// Copyright 2019 The Pigweed Authors
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
#pragma once

// This module defines a simple and unoptimized interface for byte-by-byte
// input/output. This can be done over a logging system, stdio, UART, via a
// photodiode and modulated kazoo, or basically any way to get data in and out
// of an application.
//
// This facade doesn't dictate any policies on input and output data encoding,
// format, or transmission protocol. It only requires that backends return a
// Status::OK if the operation succeeds. Backends may provide useful error
// Status types, but depending on the implementation-specific Status values is
// NOT recommended. Since this facade provides a very vague I/O interface, it
// does NOT provide tests. Backends are expected to provide their own testing to
// validate correctness.
//
// The intent of this module for simplifying bringup or otherwise getting data
// in/out of a CPU in a way that is platform-agnostic. The interface is designed
// to be easy to understand. There's no initialization as part of this
// interface, there's no configuration, and the interface is no-frills WYSIWYG
// byte-by-byte i/o.
//
//
//          PLEASE DON'T BUILD PROJECTS ON TOP OF THIS INTERFACE.


namespace pw::sys_io_baremetal_stm32f303 {

}  // namespace pw::sys_io_baremetal_stm32f303
