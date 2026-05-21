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
developed in the Sun (NeuroAI) lab at Cornell University. It specializes the general microcontroller framework provided
by the [ataraxis-micro-controller](https://github.com/Sun-Lab-NBB/ataraxis-micro-controller) library into the concrete
hardware modules consumed by Sollertia platform data acquisition systems and exposed to the host PC through the
[sollertia-experiment](https://github.com/Sun-Lab-NBB/sollertia-experiment) runtime.

The firmware is partitioned across microcontroller boards via preprocessor target macros in `main.cpp`; each board
runs one firmware binary corresponding to one target. The current Mesoscope-VR acquisition system (the only consumer
this project currently supports) uses three target classes: AMC-ACTOR, AMC-SENSOR, and AMC-ENCODER. The Actor
interfaces with the hardware modules that control the experiment environment, for example, to deliver water, lock
the running wheel, and activate Virtual Reality screens. The Sensor monitors most data-acquisition devices, such as
the torque sensor, lick sensor, and Mesoscope frame timestamp sensor. The Encoder uses hardware interrupt logic to
monitor the animal's movement using a rotary encoder and, due to interrupt logic constraints, is segmented into its
own class of microcontrollers. This combination maximizes data acquisition speed while avoiding communication channel
overloading. Future acquisition systems can define any other set of targets with any partitioning of the available
modules across boards.

This project contains both the schematics for assembling the microcontrollers used by the Sollertia platform and the
firmware that runs on those microcontrollers. The hardware created and programmed as part of this project is designed
to be interfaced through the bindings available from the
[sollertia-experiment](https://github.com/Sun-Lab-NBB/sollertia-experiment) library, which is a core dependency of every
Sollertia platform acquisition system.

___

## Table of Contents

- [Dependencies](#dependencies)
- [Hardware Assembly](#hardware-assembly)
- [Software Installation](#software-installation)
- [Per-Target Configuration](#per-target-configuration)
- [Usage](#usage)
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

- [digitalWriteFast](https://github.com/ArminJo/digitalWriteFast).
- [Encoder](https://github.com/PaulStoffregen/Encoder).
- [ataraxis-micro-controller](https://github.com/Sun-Lab-NBB/ataraxis-micro-controller).
- [ataraxis-transport-layer-mc](https://github.com/Sun-Lab-NBB/ataraxis-transport-layer-mc).

___

## Hardware Assembly

To assemble the microcontroller hardware, consult the
[schematics and instructions](https://drive.google.com/drive/folders/12gDWwI_88usMgt7qVo7e83FKYo45KZwz?usp=drive_link)
reflecting the latest state of the Sollertia platform microcontroller hardware.

***Note,*** the provided link only covers the microcontrollers and does not discuss the assembly of other
experiment-facilitating devices used by each data acquisition system. Consult the
[sollertia-experiment](https://github.com/Sun-Lab-NBB/sollertia-experiment) library for details on assembling the other
Sollertia platform data acquisition system components.

___

## Software Installation

1. Download this repository to a local PC with direct USB access to the microcontrollers. Use the latest
   stable release from [GitHub](https://github.com/Sun-Lab-NBB/sollertia-micro-controllers/releases), as it always
   reflects the current state of the Sollertia platform data acquisition hardware.
2. Open the project in the 'PlatformIO' IDE.
3. Optionally disable all hardware modules not used by the target acquisition system. This project is intended to be
   reused by all Sollertia platform acquisition systems, so it contains all hardware modules the platform supports.
4. Connect a ***single*** microcontroller to the host PC and select the microcontroller type by modifying the
   preprocessor directive on line 26 of the [main.cpp](src/main.cpp). Do ***NOT*** connect more than a single controller
   at a time, as some systems have issues selecting the correct upload target otherwise.
5. After uploading the firmware, disconnect the microcontroller from the host PC and connect the next microcontroller.
6. Repeat steps 4 and 5 until all microcontrollers are configured.
7. Connect all microcontrollers to the PC that will manage the data acquisition runtime (the main data-acquisition PC).

___

## Per-Target Configuration

The firmware exposes a small set of compile-time identifiers that the companion host-PC runtime
([sollertia-experiment](https://github.com/Sun-Lab-NBB/sollertia-experiment)) must match. The defaults
shipped with this project are:

- **Controller IDs** (set in [main.cpp](src/main.cpp)): `ACTOR = 101`, `SENSOR = 152`, `ENCODER = 203`.
- **Keepalive interval**: `500` ms. The Kernel expects the host PC to send a keepalive message at least this often;
  if it does not, the microcontroller resets to abort runtime.

Adjust these values directly in `main.cpp` if a deployment needs different IDs or a different keepalive cadence,
and make sure the host-PC configuration is updated to match.

___

## Usage

Once the microcontrollers are assembled, configured, and connected to the main data acquisition PC, they are
accessed via the [sollertia-experiment](https://github.com/Sun-Lab-NBB/sollertia-experiment) library.

___

## API Documentation

See the [API documentation](https://sollertia-micro-controllers-api-docs.netlify.app/) for the detailed description of
the methods and classes exposed by this project.

___

## AI-Assisted Development

Claude Code skills and AI development assets for this project are distributed through two marketplaces:

- [sollertia](https://github.com/Sun-Lab-NBB/sollertia) marketplace: Provides the firmware-aware Sollertia skills
  via the **experiment** plugin. The most relevant skills for this firmware are:
  - `/microcontroller-interface` — registry of paired firmware Module + Python `ModuleInterface` classes plus the
    cross-side conventions and contract they share
  - `/acquisition-system-design` — platform-general pattern for how a consuming acquisition system composes the
    firmware's wrappers into binding classes and a system configuration
  - `/mesoscope-vr` — current Mesoscope-VR consumer's hardware composition and configuration surface (the only
    acquisition system this firmware currently feeds)
  - `/mesoscope-vr-runtime` — Mesoscope-VR runtime behavior (state machine, training modes, CLI commands)
  - `/zaber-interface` — Zaber motor interface mechanics (shared across acquisition systems)
  - `/pipeline` — end-to-end Sollertia experiment lifecycle orchestration context
  - `/acquisition-system-setup` — post-flash hardware enumeration and verification
- [ataraxis](https://github.com/Sun-Lab-NBB/ataraxis) marketplace: Provides the low-level firmware module mechanics
  via the **microcontroller** plugin (`/firmware-module` for the base `Module` subclass implementation), the
  host-PC communication side via the **communication** plugin (`/microcontroller-interface` for the base
  `ModuleInterface` API, `/microcontroller-setup` for discovery and MQTT verification), and shared development
  skills that enforce Sun Lab coding conventions (C++ style, README style, commit messages, Sphinx documentation,
  tox configuration) and general-purpose codebase exploration tools via the **automation** plugin.

Install both marketplace plugins to make all associated skills and development tools available to compatible AI
coding agents. See [CLAUDE.md](CLAUDE.md) for the full session-start workflow and the canonical reading order when
adding or modifying a firmware module.

___

## Versioning

This project uses [semantic versioning](https://semver.org/). See the
[tags on this repository](https://github.com/Sun-Lab-NBB/sollertia-micro-controllers/tags) for the available project
releases.

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
