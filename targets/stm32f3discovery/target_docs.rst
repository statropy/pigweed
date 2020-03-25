.. _chapter-stm32f3discovery:

.. default-domain:: cpp

.. highlight:: sh

----------------
stm32f3discovery
----------------
The STMicroelectronics STM32F3DISCOVERY development board features the STM32F303VCT6 MCU 
with256KB of flash and 48KB of RAM, a user button, 8 user LEDs, a 3-axis gyroscope and a 3D
linear acceleration/magnetic sensor.

Building
========
To build for this target, change the ``pw_target_config`` GN build arg to point
to this target's configuration file.

.. code:: sh

  $ gn gen --args='pw_target_config = "//targets/stm32f3discovery/target_config.gni"' out/f3disco
  $ ninja -C out/f3disco

or

.. code:: sh

  $ gn gen out/f3disco
  $ gn args
  # Modify and save the args file to update the pw_target_config.
  pw_target_config = "//targets/stm32f3discovery/target_config.gni"
  $ ninja -C out/f3disco

Testing
=======
The default Pigweed build target will build all Pigweed modules' unit tests.
These tests can be run on-device in a few different ways.

Run a unit test
---------------
If using ``out/f3disco`` as a build directory, tests will be located in
``out/f3disco/obj/[module name]/[test_name].elf``. To run these on device, the
stm32f3discovery target provides a helper script that flashes the test to a
device and then runs it.

.. code:: sh

  # Setup pigweed environment.
  $ . pw_env_setup/.env_setup.sh 
  # Run test.
  $ stm32f3discovery_unit_test_runner /path/to/binary

Run multiple tests
------------------
Running all tests one-by-one is rather tedious. To make running multiple
tests easier, use Pigweed's ``pw test`` command and pass it a path to the build
directory and the name of the test runner. By default, ``pw test`` will run all
tests, but it can be restricted it to specific ``pw_test_group`` targets using
the ``--group`` argument. Alternatively, individual test binaries can be
specified with the ``--test`` option.

.. code:: sh

  # Setup Pigweed environment.
  $ . pw_env_setup/setup.sh
  # Run test.
  $ pw test --root out/f3disco/ --runner stm32f3discovery_unit_test_runner

Run tests affected by code changes
----------------------------------
When writing code that will impact multiple modules, it's helpful to only run
all tests that are affected by a given code change. Thanks to the GN/Ninja
build, this is possible! This is done by using a ``pw_target_runner_server``
that Ninja can send the tests to as it rebuilds affected targets.

Additionally, this method enables distributed testing. If multiple devices are
connected, the tests will be run across all attached devices to further speed up
testing.

Step 1: Start test server
^^^^^^^^^^^^^^^^^^^^^^^^^
To allow Ninja to properly serialize tests to run on an arbitrary number of
devices, Ninja will send test requests to a server running in the background.
The first step is to launch this server. By default, the script will attempt
to automatically detect all attached stm32f3discovery boards and use them for
testing. To override this behavior, provide a custom server configuration file
with ``--server-config``.

.. tip::

  If you unplug or plug in any boards, you'll need to restart the test server
  for hardware changes to properly be detected.

.. code:: sh

  $ stm32f3discovery_test_server

Step 2: Configure GN
^^^^^^^^^^^^^^^^^^^^
By default, this hardware target has incremental testing via
``pw_target_runner`` disabled. Enabling the ``pw_use_test_server`` build arg
tells GN to send requests to a running ``stm32f3discovery_test_server``.

.. code:: sh

  $ gn args out/f3disco
  # Modify and save the args file to use pw_target_runner.
  pw_use_test_server = true

Step 3: Build changes
^^^^^^^^^^^^^^^^^^^^^
Whenever you run ``ninja -C out/f3disco``, affected tests will be built and run on
the attached device(s). Alternatively, you may use ``pw watch`` to set up
Pigweed to build/test whenever it sees changes to source files.
