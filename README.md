# sollertia-micro-controllers

Aggregates the firmware and hardware documentation for all Ataraxis Micro Controllers (AXMCs) used by Sollertia
platform data acquisition systems.

![c++](https://img.shields.io/badge/C++-00599C?style=flat-square&logo=C%2B%2B&logoColor=white)
![license](https://img.shields.io/badge/license-GPLv3-blue)
---

## Detailed Description

This project is part of the [Sollertia](https://github.com/Sun-Lab-NBB/sollertia) AI-assisted scientific data
acquisition and processing platform, built on the [Ataraxis](https://github.com/Sun-Lab-NBB/ataraxis) framework and
developed in the Sun (NeuroAI) lab at Cornell University. It specializes the general microcontroller framework provided
by the [ataraxis-micro-controller](https://github.com/Sun-Lab-NBB/ataraxis-micro-controller) library into the concrete
hardware modules consumed by Sollertia platform data acquisition systems and exposed to the host PC through the
[sollertia-experiment](https://github.com/Sun-Lab-NBB/sollertia-experiment) runtime.

Currently, all Sollertia platform data acquisition systems can use up to three distinct classes of microcontrollers:
AMC-ACTOR, AMC-SENSOR, and AMC-ENCODER. The Actor interfaces with the hardware modules that control the experiment
environment, for example, to deliver water, lock the running wheel, and activate Virtual Reality screens. The Sensor
monitors most data-acquisition devices, such as the torque sensor, lick sensor, and Mesoscope frame timestamp sensor.
The Encoder uses hardware interrupt logic to monitor the animal's movement using a rotary encoder and, due to interrupt
logic constraints, is segmented into its own class of microcontrollers. Using this combination of microcontrollers
maximizes data acquisition speed while avoiding communication channel overloading. Typically, each acquisition system
uses at most a single instance of each microcontroller type.

This project contains both the schematics for assembling the microcontrollers used by the Sollertia platform and the
firmware that runs on those microcontrollers. The hardware created and programmed as part of this project is designed
to be interfaced through the bindings available from the
[sollertia-experiment](https://github.com/Sun-Lab-NBB/sollertia-experiment) library, which is a core dependency of every
Sollertia platform acquisition system.

---

## Table of Contents

- [Dependencies](#dependencies)
- [Hardware Assembly](#hardware-assembly)
- [Software Installation](#software-installation)
- [Usage](#usage)
- [API Documentation](#api-documentation)
- [Versioning](#versioning)
- [Authors](#authors)
- [License](#license)
- [Acknowledgments](#acknowledgments)

---

## Dependencies

### Main Dependency
- [PlatformIO](https://platformio.org/install) IDE to upload the firmware to each microcontroller.

### Additional Dependencies
These dependencies are automatically resolved whenever the project is installed via PlatformIO.

- [digitalWriteFast](https://github.com/ArminJo/digitalWriteFast).
- [Encoder](https://github.com/PaulStoffregen/Encoder).
- [ataraxis-micro-controller](https://github.com/Sun-Lab-NBB/ataraxis-micro-controller).
- [ataraxis-transport-layer-mc](https://github.com/Sun-Lab-NBB/ataraxis-transport-layer-mc).

---

## Hardware Assembly

To assemble the microcontroller hardware, consult the
[schematics and instructions](https://drive.google.com/drive/folders/12gDWwI_88usMgt7qVo7e83FKYo45KZwz?usp=drive_link)
reflecting the latest state of the Sollertia platform microcontroller hardware.

Note, the provided link only covers the microcontrollers and does not discuss the assembly of other
experiment-facilitating devices used by each data acquisition system. Consult the
[sollertia-experiment](https://github.com/Sun-Lab-NBB/sollertia-experiment) library for details on assembling the other
Sollertia platform data acquisition system components.

---

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

---

## Usage

Once the microcontrollers are assembled, configured, and connected to the main data acquisition PC, they are
accessed via the [sollertia-experiment](https://github.com/Sun-Lab-NBB/sollertia-experiment) library.

---

## API Documentation

See the [API documentation](https://sollertia-micro-controllers-api-docs.netlify.app/) for the detailed description of
the methods and classes exposed by this project.

---

## Versioning

This project uses [semantic versioning](https://semver.org/). See the
[tags on this repository](https://github.com/Sun-Lab-NBB/sollertia-micro-controllers/tags) for the available releases.

---

## Authors

- Ivan Kondratyev ([Inkaros](https://github.com/Inkaros))

---

## License

This project is licensed under the GPL3 License: see the [LICENSE](LICENSE) file for details.

---

## Acknowledgments

- All Sun lab [members](https://neuroai.github.io/sunlab/people) for providing the inspiration and comments during the
  development of this project.
- The creators of all other dependencies and projects listed in the [platformio.ini](platformio.ini) file.

---
