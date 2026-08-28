# sollertia-micro-controllers

Aggregates the firmware and hardware documentation for all Ataraxis Micro Controllers (AXMCs) used by Sollertia
platform data acquisition systems.

![PlatformIO](https://img.shields.io/badge/PlatformIO-orange?logo=platformio&logoColor=white&labelColor=grey)
![C++](https://img.shields.io/badge/C%2B%2B-blue?logo=cplusplus&logoColor=white&labelColor=grey)
![Arduino](https://img.shields.io/badge/Arduino-blue?logo=Arduino&logoColor=white&labelColor=grey)
[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](LICENSE)

___

## Detailed Description

This project is part of the [Sollertia](https://github.com/Sun-Lab-NBB/sollertia) AI-assisted scientific data
acquisition and processing platform, built on the [Ataraxis](https://github.com/Sun-Lab-NBB/ataraxis) framework and
developed in the [Sun (NeuroAI) lab](https://neuroai.github.io/sunlab/) at Cornell University. It specializes the
general microcontroller framework provided by the
[ataraxis-micro-controller](https://github.com/Sun-Lab-NBB/ataraxis-micro-controller) library into the concrete
hardware modules consumed by Sollertia platform data acquisition systems and exposed to the host PC through the
[sollertia-experiment](https://github.com/Sun-Lab-NBB/sollertia-experiment) runtime.

The firmware is partitioned across microcontroller boards via preprocessor target macros in `main.cpp`. Each board runs
one firmware binary corresponding to one target. The current Mesoscope-VR acquisition system (the only consumer this
project currently supports) uses three target classes: ACTOR, SENSOR, and ENCODER. The Actor interfaces with the
hardware modules that control the experiment environment, for example, to deliver water, lock the running wheel, and
activate Virtual Reality screens. The Sensor monitors most data-acquisition devices, such as the torque sensor, lick
sensor, and Mesoscope frame timestamp sensor. The Encoder uses hardware interrupt logic to monitor the animal's movement
using a rotary encoder and, due to interrupt logic constraints, is segmented into its own class of microcontrollers.
This combination maximizes data acquisition speed while avoiding communication channel overloading. Future acquisition
systems can define any other set of targets with any partitioning of the available modules across boards.

This repository contains the firmware that runs on the microcontrollers used by the Sollertia platform and links to the
schematics for assembling them. The hardware created and programmed as part of this project is designed to be
interfaced through the bindings available from the
[sollertia-experiment](https://github.com/Sun-Lab-NBB/sollertia-experiment) library, which is a core dependency of every
Sollertia platform acquisition system.

___

## Table of Contents

- [Dependencies](#dependencies)
- [Installation](#installation)
  - [Hardware Assembly](#hardware-assembly)
  - [Software Installation](#software-installation)
  - [Per-Target Configuration](#per-target-configuration)
- [Usage](#usage)
- [Extending the Library](#extending-the-library)
- [API Documentation](#api-documentation)
- [AI-Assisted Development](#ai-assisted-development)
- [Versioning](#versioning)
- [Authors](#authors)
- [License](#license)
- [Acknowledgments](#acknowledgments)

___

## Dependencies

### Main Dependency
- [PlatformIO](https://platformio.org/install) IDE to upload the firmware to each microcontroller.

### Additional Dependencies
These dependencies are automatically resolved whenever the project is installed via PlatformIO.

- [Encoder](https://github.com/PaulStoffregen/Encoder).
- [ataraxis-micro-controller](https://github.com/Sun-Lab-NBB/ataraxis-micro-controller).
- [ataraxis-transport-layer-mc](https://github.com/Sun-Lab-NBB/ataraxis-transport-layer-mc).

___

## Installation

### Hardware Assembly

To assemble the microcontroller hardware, consult the [schematics and
instructions](https://drive.google.com/drive/folders/12gDWwI_88usMgt7qVo7e83FKYo45KZwz?usp=drive_link) reflecting the
latest state of the Sollertia platform microcontroller hardware.

***Note,*** the provided link only covers the microcontrollers and does not discuss the assembly of other
experiment-facilitating devices used by each data acquisition system. Consult the
[sollertia-experiment](https://github.com/Sun-Lab-NBB/sollertia-experiment) library for details on assembling the other
Sollertia platform data acquisition system components.

### Software Installation

1. Download this repository to a local PC with direct USB access to the microcontrollers. Use the latest
   stable release from [GitHub](https://github.com/Sun-Lab-NBB/sollertia-micro-controllers/releases), as it always
   reflects the current state of the Sollertia platform data acquisition hardware.
2. Open the project in the 'PlatformIO' IDE.
3. Optionally disable all hardware modules not used by the target acquisition system. This project is intended to be
   reused by all Sollertia platform acquisition systems, so it contains all hardware modules the platform supports.
4. Connect a ***single*** microcontroller to the host PC and upload the PlatformIO environment matching that
   controller's target, one of `teensy41_actor`, `teensy41_sensor`, or `teensy41_encoder`. Do ***NOT*** connect more
   than a single controller at a time, as some systems have issues selecting the correct upload target otherwise.
5. After uploading the firmware, disconnect the microcontroller from the host PC and connect the next microcontroller.
6. Repeat steps 4 and 5 until all microcontrollers are configured.
7. Connect all microcontrollers to the PC that will manage the data acquisition runtime (the main data-acquisition PC).

***Warning!*** Always name the environment when uploading, as in `pio run -e teensy41_actor -t upload`. An upload
command that omits the environment processes every environment in turn, flashing the connected board with each target
firmware and leaving it running the last one.

### Per-Target Configuration

The firmware exposes a small set of compile-time identifiers that the companion host-PC runtime
([sollertia-experiment](https://github.com/Sun-Lab-NBB/sollertia-experiment)) must match. The defaults
shipped with this project are:

- **Controller IDs** (set in [main.cpp](src/main.cpp)): `ACTOR = 101`, `SENSOR = 152`, `ENCODER = 203`.
- **Keepalive interval**: `500` ms. The Kernel expects the host PC to send a keepalive message at least this often. If
  it does not, the microcontroller resets to abort runtime. The Kernel doubles this value internally, so the emergency
  reset fires after roughly twice the interval without a keepalive command.
- **Serial baud rate**: `115200`. Teensy boards ignore the value, but the host-PC runtime opens the port with it, and
  it matches the `monitor_speed` set in [platformio.ini](platformio.ini).
- **ADC resolution**: `12` bits, giving the 0 to 4095 readout range. Every ADC-unit value on both sides is scaled to
  it, so a one-sided change leaves each message parseable while its numbers mean something else.
- **Module `(type, id)` pairs**: each module instance is constructed with a module type code and a per-controller
  instance ID. The current deployment assigns TTL `1`, encoder `2`, brake `3`, lick `4`, valve `5`, torque `6`, and
  screen `7`, with instance ID `1` everywhere except the second valve (the gas-puff valve), which uses ID `2`. The pair
  must be unique across every module on one controller, which is why the two valves share type `5` and differ by ID.
  The firmware accepts a repeated pair without complaint. The host-PC runtime reports it when it connects, after the
  controller has already set up its hardware, so check the pairs by hand whenever this list changes.

Adjust these values directly in `main.cpp` if a deployment needs different IDs, a different keepalive cadence, or a
different module layout, and make sure the host-PC configuration is updated to match. When a deployment needs a new
controller target or a new board family rather than new values for the existing ones, the experiment plugin's
`/library-extension` skill carries the full seam list and names the sollertia-experiment mirror each change obliges.

___

## Usage

Once the microcontrollers are assembled, configured, and connected to the main data acquisition PC, they are
accessed via the [sollertia-experiment](https://github.com/Sun-Lab-NBB/sollertia-experiment) library.

___

## Extending the Library

The firmware exposes three extension seams, and each one carries a matching obligation in the
[sollertia-experiment](https://github.com/Sun-Lab-NBB/sollertia-experiment) library that drives it.

- **A new hardware module.** Add `src/<name>_module.h` declaring a `Module` subclass, then wire it into the target's
  `#ifdef` block in `src/main.cpp` as an include, an instantiation carrying a unique `(module_type, module_id)` pair,
  and an entry in that target's `modules[]` array. The module stays unusable until a matching `ModuleInterface`
  subclass exists in sollertia-experiment's `cross_system/module_interfaces.py`, carrying the same type, id, command
  codes, status codes, and parameter field order.
- **A new controller target.** Add an `[env:teensy41_<target>]` environment to `platformio.ini` inheriting
  `[teensy41_base]`, then add an `#elif defined <TARGET>` branch to `src/main.cpp` declaring its `kControllerID`, its
  module instances, and its `modules[]` array. Name the target in the `static_assert` of the `#else` branch, and
  mirror it with a `MicroControllerInterface` carrying the same controller id.
- **A new board family.** Add a second non-`env:` template mirroring `[teensy41_base]` in `platformio.ini`, plus one
  `[env:<board>_<target>]` environment per target. Teensy 4.1 is currently the only family, because every shipped
  environment inherits its `board` and `monitor_speed` from that one template.

Module type codes 1 through 7 are in use, and 8 is the next unused code. The `(module_type, module_id)` pair must be
unique within its controller, and no build step checks it, so a collision surfaces only at the identification
handshake the acquisition runtime performs when a session starts.

The library version is declared in two places that must move together, `PROJECT_NUMBER` in `Doxyfile` and `release`
in `docs/source/conf.py`.

For the ordered step lists, the roster of constants that must move across repositories, and the paired-class
contract, use the **experiment** plugin skills described under
[AI-Assisted Development](#ai-assisted-development).

___

## API Documentation

See the [API documentation](https://sollertia-micro-controllers-api-docs.netlify.app/) for the detailed description of
the methods and classes exposed by components of this library.

___

## AI-Assisted Development

Claude Code skills and AI development assets for this project are distributed through two marketplaces:

- [sollertia](https://github.com/Sun-Lab-NBB/sollertia) marketplace:
  - **experiment** plugin: two firmware-aware skills. `/microcontroller-interface` is a registry of the paired firmware
    Module and host-PC `ModuleInterface` classes and the cross-side contract they share, and it is the entry point for
    any change that spans this firmware and its
    [sollertia-experiment](https://github.com/Sun-Lab-NBB/sollertia-experiment) consumer. It also owns the roster of
    cross-repo constants that must move together, pairing each one with the sollertia-experiment symbol that mirrors
    it. `/library-extension` owns the three extension seams, a new firmware module, a new controller target, and a
    new board family. Both link out to the ataraxis plugins below for the underlying mechanics. The host-PC interface
    and configuration skills they reference belong to the consumer and are documented there.
- [ataraxis](https://github.com/Sun-Lab-NBB/ataraxis) marketplace:
  - **microcontroller** plugin: the foundational C++ firmware mechanics via the `microcontroller:firmware-module` skill
    (base `Module` subclass implementation: template parameters, parameter structs, status and command codes, and
    stage-based command execution).
  - **automation** plugin: shared development skills that enforce Sollertia platform coding conventions (C++ style,
    README style, commit messages, Sphinx documentation, tox configuration) and general-purpose codebase exploration
    tools.

Install all three plugins to make the full skill set available to compatible AI coding agents. See
[CLAUDE.md](CLAUDE.md) for the full session-start workflow and the canonical reading order when adding or modifying
a firmware module.

___

## Versioning

This project uses [semantic versioning](https://semver.org/). See the
[tags on this repository](https://github.com/Sun-Lab-NBB/sollertia-micro-controllers/tags) for the available project
releases. This project is a firmware application rather than a PlatformIO library, so it ships no `library.json`. The
release version is declared in two in-repository files, `PROJECT_NUMBER` in [Doxyfile](Doxyfile) and `release` in
[docs/source/conf.py](docs/source/conf.py), and both must be updated to match the tag.

___

## Authors

- Ivan Kondratyev ([Inkaros](https://github.com/Inkaros))

___

## License

This project is licensed under the Apache 2.0 License: see the [LICENSE](LICENSE) file for details.

___

## Acknowledgments

- All Sun lab [members](https://neuroai.github.io/sunlab/people) for providing the inspiration and comments during the
  development of this project.
- The creators of all other dependencies and projects listed in the [platformio.ini](platformio.ini) file.

___
