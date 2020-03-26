// Copyright 2020 The Pigweed Authors
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy of
// the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.

#include <cinttypes>

#include "pw_boot_armv7m/boot.h"
#include "pw_preprocessor/compiler.h"
#include "pw_sys_io/sys_io.h"

namespace {

// Default core clock. This is technically not a constant, but since this app
// doesn't change the system clock a constant will suffice.
constexpr uint32_t kSystemCoreClock = 8000000;

// Base address for everything peripheral-related on the STM32F3xx.
constexpr uint32_t kPeripheralBaseAddr = 0x40000000u;
// Base address for everything AHB1-related on the STM32F3xx.
constexpr uint32_t kAhb1PeripheralBase = kPeripheralBaseAddr + 0x00020000U;
// Base address for everything AHB2-related on the STM32F3xx.
constexpr uint32_t kAhb2PeripheralBase = kPeripheralBaseAddr + 0x08000000U;
// Base address for everything APB2-related on the STM32F3xx.
constexpr uint32_t kApb2PeripheralBase = kPeripheralBaseAddr + 0x00010000U;

// Reset/clock configuration block (RCC).
// `reserved` fields are unimplemented features, and are present to ensure
// proper alignment of registers that are in use.
PW_PACKED(struct) RccBlock {
  uint32_t reserved1[5];
  uint32_t ahb_config;
  uint32_t apb2_config;
};

// Mask for ahb_config (AHBENR) to enable the "C" GPIO pins.
constexpr uint32_t kGpioCEnable = 0x1u << 19;

// Mask for apb2_config (APB2ENR) to enable USART1.
constexpr uint32_t kUsart1Enable = 0x1u << 14;

// GPIO register block definition.
PW_PACKED(struct) GpioBlock {
  uint32_t modes;
  uint32_t out_type;
  uint32_t out_speed;
  uint32_t pull_up_down;
  uint32_t input_data;
  uint32_t output_data;
  uint32_t gpio_bit_set;
  uint32_t port_config_lock;
  uint32_t alt_low;
  uint32_t alt_high;
  uint32_t gpio_bit_reset;
};

// Constants related to GPIO mode register masks.
constexpr uint32_t kGpioPortModeMask = 0x3u;
constexpr uint32_t kGpio4PortModePos = 8;
constexpr uint32_t kGpio5PortModePos = 10;
constexpr uint32_t kGpioPortModeAlternate = 2;

// Constants related to GPIO port speed register masks.
constexpr uint32_t kGpioPortSpeedMask = 0x3u;
constexpr uint32_t kGpio4PortSpeedPos = 8;
constexpr uint32_t kGpio5PortSpeedPos = 10;
constexpr uint32_t kGpioSpeedHigh = 3;

// // Constants related to GPIO pull up/down resistor type masks.
constexpr uint32_t kGpioPullTypeMask = 0x3u;
constexpr uint32_t kGpio4PullTypePos = 8;
constexpr uint32_t kGpio5PullTypePos = 10;
constexpr uint32_t kPullTypeNone = 0;

// Constants related to GPIO alternate function register masks.
constexpr uint32_t kGpioAltModeMask = 0xFu;
constexpr uint32_t kGpio4AltModeLowPos = 16;
constexpr uint32_t kGpio5AltModeLowPos = 20;

// Alternate function for pins C4 and C5 that enable USART1.
constexpr uint8_t kGpioAlternateFunctionUsart1 = 0x07u;

// USART status flags.
constexpr uint32_t kTxRegisterEmpty = 0x1u << 7;

// USART configuration flags for control1 register.
// Note: a large number of configuration flags have been omitted as they default
// to sane values and we don't need to change them.
constexpr uint32_t kReceiveEnable = 0x1u << 2;
constexpr uint32_t kTransmitEnable = 0x1u << 3;
constexpr uint32_t kReadDataReady = 0x1u << 5;
constexpr uint32_t kEnableUsart = 0x1u;

// Layout of memory mapped registers for USART blocks.
PW_PACKED(struct) UsartBlock {
  uint32_t control1;
  uint32_t control2;
  uint32_t control3;
  uint32_t baud_rate;
  uint32_t prescaler;
  uint32_t rx_timeout;
  uint32_t request;
  uint32_t status;
  uint32_t interrupt_flag_clear;
  // Only the lower 9 bits are valid
  uint32_t rx_data_register;
  // Only the lower 9 bits are valid
  uint32_t tx_data_register;
};

// Sets the UART baud register using the peripheral clock and target baud rate.
// These calculations are specific to the default oversample by 16 mode.
// TODO(amontanez): Document magic calculations in full UART implementation.
uint32_t CalcBaudRegister(uint32_t clock, uint32_t target_baud) {
  return clock / target_baud;
}

// Declare a reference to the memory mapped RCC block.
volatile RccBlock& platform_rcc =
    *reinterpret_cast<volatile RccBlock*>(kAhb1PeripheralBase + 0x1000U);

// Declare a reference to the 'C' GPIO memory mapped block.
volatile GpioBlock& gpio_c =
    *reinterpret_cast<volatile GpioBlock*>(kAhb2PeripheralBase + 0x0800U);

// Declare a reference to the memory mapped block for USART1.
volatile UsartBlock& usart1 =
    *reinterpret_cast<volatile UsartBlock*>(kApb2PeripheralBase + 0x3800U);

// Default handler to insert into the ARMv7-M vector table (below).
// This function exists for convenience. If a device isn't doing what you
// expect, it might have hit a fault and ended up here.
void DefaultFaultHandler(void) {
  while (true) {
    // Wait for debugger to attach.
  }
}

// This is the device's interrupt vector table. It's not referenced in any
// code because the platform (STM32F3xx) expects this table to be present at the
// beginning of flash. The exact address is specified in the pw_boot_armv7m
// configuration as part of the target config.
//
// For more information, see ARMv7-M Architecture Reference Manual DDI 0403E.b
// section B1.5.3.

// This typedef is for convenience when building the vector table. With the
// exception of SP_main (0th entry in the vector table), all the entries of the
// vector table are function pointers.
typedef void (*InterruptHandler)();

PW_KEEP_IN_SECTION(".vector_table")
const InterruptHandler vector_table[] = {
    // The starting location of the stack pointer.
    // This address is NOT an interrupt handler/function pointer, it is simply
    // the address that the main stack pointer should be initialized to. The
    // value is reinterpret casted because it needs to be in the vector table.
    [0] = reinterpret_cast<InterruptHandler>(&pw_stack_high_addr),

    // Reset handler, dictates how to handle reset interrupt. This is the
    // address that the Program Counter (PC) is initialized to at boot.
    [1] = pw_BootEntry,

    // NMI handler.
    [2] = DefaultFaultHandler,
    // HardFault handler.
    [3] = DefaultFaultHandler,
};

}  // namespace

extern "C" void pw_PreMainInit() {
  // Enable 'C' GIPO clocks.
  platform_rcc.ahb_config |= kGpioCEnable;

  // Enable Uart TX pin.
  // Output type defaults to push-pull (rather than open/drain).
  gpio_c.modes |= kGpioPortModeAlternate << kGpio4PortModePos;
  gpio_c.out_speed |= kGpioSpeedHigh << kGpio4PortSpeedPos;
  gpio_c.pull_up_down |= kPullTypeNone << kGpio4PullTypePos;
  gpio_c.alt_low |= kGpioAlternateFunctionUsart1 << kGpio4AltModeLowPos;

  // Enable Uart RX pin.
  // Output type defaults to push-pull (rather than open/drain).
  gpio_c.modes |= kGpioPortModeAlternate << kGpio5PortModePos;
  gpio_c.out_speed |= kGpioSpeedHigh << kGpio5PortSpeedPos;
  gpio_c.pull_up_down |= kPullTypeNone << kGpio5PullTypePos;
  gpio_c.alt_low |= kGpioAlternateFunctionUsart1 << kGpio5AltModeLowPos;

  // Initialize USART1. Initialized to 8N1 at the specified baud rate.
  platform_rcc.apb2_config |= kUsart1Enable;

  // Warning: Normally the baud rate register calculation is based off
  // peripheral 2 clock. For this code, the peripheral clock defaults to
  // the system core clock so it can be used directly.
  usart1.baud_rate = CalcBaudRegister(kSystemCoreClock, /*target_baud=*/115200);

  usart1.control1 = kEnableUsart | kReceiveEnable | kTransmitEnable;

// TODO(pwbug/17): Replace when Pigweed config system is added.
#if defined(PW_ARMV7M_ENABLE_FPU) && PW_ARMV7M_ENABLE_FPU == 1
  // Enable FPU if built using hardware FPU instructions.
  // CPCAR mask that enables FPU. (ARMv7-M Section B3.2.20)
  constexpr uint32_t kFpuEnableMask = (0xFu << 20);

  // Memory mapped register to enable FPU. (ARMv7-M Section B3.2.2, Table B3-4)
  volatile uint32_t& arm_v7m_cpacr =
      *reinterpret_cast<volatile uint32_t*>(0xE000ED88u);

  arm_v7m_cpacr |= kFpuEnableMask;
#endif  // defined(PW_ARMV7M_ENABLE_FPU) && PW_ARMV7M_ENABLE_FPU == 1
}

namespace pw::sys_io {

// Wait for a byte to read on USART1. This blocks until a byte is read. This is
// extremely inefficient as it requires the target to burn CPU cycles polling to
// see if a byte is ready yet.
Status ReadByte(std::byte* dest) {
  while (true) {
    if (usart1.status & kReadDataReady) {
      *dest = static_cast<std::byte>(usart1.rx_data_register);
    }
  }
  return Status::OK;
}

// Send a byte over USART1. Since this blocks on every byte, it's rather
// inefficient. At the default baud rate of 115200, one byte blocks the CPU for
// ~87 micro seconds. This means it takes only 10 bytes to block the CPU for
// 1ms!
Status WriteByte(std::byte b) {
  // Wait for TX buffer to be empty. When the buffer is empty, we can write
  // a value to be dumped out of UART.
  while (!(usart1.status & kTxRegisterEmpty)) {
  }
  usart1.tx_data_register = static_cast<uint32_t>(b);
  return Status::OK;
}

// Writes a string using pw::sys_io, and add newline characters at the end.
StatusWithSize WriteLine(const std::string_view& s) {
  size_t chars_written = 0;
  StatusWithSize result = WriteBytes(as_bytes(span(s)));
  if (!result.ok()) {
    return result;
  }
  chars_written += result.size();

  // Write trailing newline ("\n\r").
  result = WriteBytes(as_bytes(span("\n\r", 2)));
  chars_written += result.size();

  return StatusWithSize(result.status(), chars_written);
}

}  // namespace pw::sys_io
