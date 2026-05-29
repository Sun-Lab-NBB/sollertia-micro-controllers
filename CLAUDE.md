# Claude Code Instructions

## Session start behavior

At the beginning of each coding session, before making any code changes, you should build a comprehensive
understanding of the codebase by invoking the `/explore-codebase` skill.

This ensures you:
- Understand the project architecture before modifying code
- Follow existing patterns and conventions
- Do not introduce inconsistencies or break integrations with downstream consumers of this firmware library
  (currently the Mesoscope-VR acquisition system in sollertia-experiment; future acquisition systems will consume
  the same firmware library)

## Style guide compliance

You MUST invoke the appropriate skill before performing ANY of the following tasks:

| Task                                       | Skill to invoke    |
|--------------------------------------------|--------------------|
| Writing or modifying C++ code              | `/cpp-style`       |
| Writing or modifying README files          | `/readme-style`    |
| Writing or modifying Sphinx docs files     | `/api-docs`        |
| Writing or modifying tox.ini               | `/tox-config`      |
| Writing git commit messages                | `/commit`          |
| Writing or modifying skill files / this MD | `/skill-design`    |
| Auditing for style compliance              | `/audit-style`     |
| Auditing for factual accuracy              | `/audit-facts`     |

Each skill contains a verification checklist that you MUST complete before submitting any work. Failure to invoke the
appropriate skill results in style violations that block release.

## Cross-referenced library verification

This firmware depends on two ataraxis C++ libraries and is consumed by one Sollertia Python library. Local clones
of all three typically live alongside this repository under `/home/cyberaxolotl/Desktop/GitHubRepos/`.

| Library                       | Direction       | Role                                                                                                                                                                                                                                |
|-------------------------------|-----------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `ataraxis-micro-controller`   | Upstream        | Provides `Kernel`, `Communication`, `Module` base class                                                                                                                                                                             |
| `ataraxis-transport-layer-mc` | Upstream        | Bidirectional serial communication with CRC and COBS                                                                                                                                                                                |
| `sollertia-experiment`        | Downstream (PC) | Owns the host-PC `ModuleInterface` wrappers (system-agnostic, in `cross_system/module_interfaces.py`) and the per-acquisition-system binding classes and configuration dataclasses (currently only Mesoscope-VR in `mesoscope_vr/`) |

**Before writing code that interacts with a cross-referenced library, you MUST:**

1. **Check for local version**: Look for the library in the parent directory (e.g.,
   `../ataraxis-micro-controller/`, `../sollertia-experiment/`).

2. **Compare versions**: If a local copy exists, compare its version against the latest release or main branch on
   GitHub:
   - Read the local `library.json` (ataraxis C++ libraries) or `pyproject.toml` (Sollertia Python libraries) to get
     the current version
   - Use `gh api repos/Sun-Lab-NBB/{repo-name}/releases/latest` to check the latest release

3. **Handle version mismatches**: If the local version differs from the latest release or main branch, notify the user
   with the following options:
   - **Use online version**: Fetch documentation and API details from the GitHub repository
   - **Update local copy**: The user will pull the latest changes locally before proceeding

4. **Proceed with correct source**: Use whichever version the user selects as the authoritative reference for API
   usage, patterns, and documentation.

## Available skills

The skills directly relevant to firmware work in this repository, and guaranteed to remain relevant regardless of
which acquisition system consumes the firmware, are:

| Skill                                  | Purpose                                                              |
|----------------------------------------|----------------------------------------------------------------------|
| `microcontroller:firmware-module`      | Base C++ `Module` subclass mechanics (ataraxis base template)        |
| `experiment:microcontroller-interface` | Paired firmware Module + host-PC `ModuleInterface` contract registry |

The Sollertia platform's development and style skills required for routine changes ship in the ataraxis marketplace's
`automation` plugin; invoke them as directed by the "Session start behavior" and "Style guide compliance" sections
above.

Everything else this firmware touches lives on the consumer side. The sollertia marketplace's `experiment` plugin and
the downstream [sollertia-experiment](https://github.com/Sun-Lab-NBB/sollertia-experiment) (sle) library own the
host-PC interface wrappers, the acquisition-system binding classes, and the per-system configuration and runtime
surface; the `experiment:microcontroller-interface` skill links out to the ataraxis `communication` plugin for the
base host-PC `ModuleInterface` API. This consumer surface changes per acquisition system (Mesoscope-VR is the only
current consumer), so this file does not enumerate it. When a change reaches the consumer side, inspect the
`experiment` plugin's skills and the `sollertia-experiment` library to determine which are currently relevant.

**Canonical reading order when adding or modifying a firmware module:**
1. `experiment:microcontroller-interface` — the cross-repo paired Module + Interface contract; allocate the new
   module type code and follow the slmc firmware + sle wrapper conventions it documents.
2. `microcontroller:firmware-module` — the base C++ `Module` subclass mechanics that the skill above extends.
3. For consumer-side changes (binding classes, system configuration, post-flash hardware setup), consult the
   `experiment` plugin and the `sollertia-experiment` library for the consuming acquisition system's current surface
   — Mesoscope-VR today, but a future consumer would expose its own skills.

***Note,*** the sollertia `experiment` plugin may be unavailable on hosts where the sollertia marketplace is not
installed (the live `available-skills` list will not include any `experiment:*` entries). The source for each skill
lives at `sollertia/plugins/experiment/skills/<skill>/SKILL.md`; consult these files directly when the slash-command
form is not available.

## Downstream library integration

This firmware lives on one end of a two-repository contract with `sollertia-experiment`, which owns the host-PC
`ModuleInterface` wrappers (system-agnostic, in `src/sollertia_experiment/cross_system/module_interfaces.py`)
and the per-acquisition-system binding classes and configuration dataclasses. The current consumer is the
Mesoscope-VR system; its binding classes and `MesoscopeMicroControllers` configuration dataclass live in
`src/sollertia_experiment/mesoscope_vr/`.

Any change to a `Module` subclass's parameter structure, status codes, command codes, controller IDs, keepalive
interval, or per-target module layout MUST be synchronized with the corresponding changes in sollertia-experiment.

**Before modifying any cross-repository contract, you MUST:**

1. **Identify the companion repository**: Check for a local copy at `../sollertia-experiment/`. If unavailable, use
   `gh api repos/Sun-Lab-NBB/sollertia-experiment` to access the remote repository.

2. **Review the corresponding implementation**: Read the host-PC `ModuleInterface` subclass in
   `sollertia-experiment/src/sollertia_experiment/cross_system/module_interfaces.py` that consumes the firmware
   module you are modifying, and the matching per-system calibration fields. For the current Mesoscope-VR consumer,
   the calibration lives in `MesoscopeMicroControllers`
   (`sollertia-experiment/src/sollertia_experiment/mesoscope_vr/system.py`). Verify that both repositories
   are currently in sync before making changes.

3. **Plan synchronized changes**: Document what must change in each repository. Notify the user of the required
   companion changes so they can be applied together.

4. **Never modify a contract field unilaterally**: A change applied to only one side will cause runtime parameter
   mismatches, dropped commands, or silent miscalibration.

**What requires synchronization with sollertia-experiment:**
- `kCustomStatusCodes` enum values (per-module, range 51-250 per the base `Module` class)
- `kModuleCommands` enum values (per-module, unique within the module class). The underlying `uint8_t`
  allows 1-255 with 0 reserved by the runtime to signal "no active command".
- `CustomRuntimeParameters` struct layout, field names, and units (one struct per module)
- Module template parameters (e.g., `EncoderModule<kPinA, kPinB, kPinX, kInvertDirection>`)
- Controller IDs (currently `ACTOR = 101`, `SENSOR = 152`, `ENCODER = 203` for the Mesoscope-VR consumer; a future
  consumer would have its own assignments)
- Keepalive interval (`kKeepaliveInterval`, currently 500 ms — Mesoscope-VR's chosen cadence)
- Per-target module layout (which `Module` subclass instances live on which controller for each acquisition
  system) and module `(type, id)` assignments

**What does NOT require synchronization:**
- Internal `Module` implementation details (stage-based command execution, intermediate state variables)
- Build configuration (`platformio.ini`, `tox.ini`, `Doxyfile`, `.clang-format`, `.clang-tidy`)
- Doxygen comments, inline comments, file-level docstrings
- LED error indication, ADC resolution settings, baud rate

## Project context

This is **sollertia-micro-controllers**, a C++17 PlatformIO firmware library that specializes the general
microcontroller framework provided by `ataraxis-micro-controller` into the concrete hardware modules used by
Sollertia platform data acquisition systems. The firmware is Arduino-compatible at the framework level and is
not locked to any single board family; the current deployment targets Teensy 4.1 boards because that is the
hardware the only currently-supported consumer (the Mesoscope-VR acquisition system) uses. Modules are exposed
to the host PC through the [sollertia-experiment](https://github.com/Sun-Lab-NBB/sollertia-experiment) runtime
within the [Sollertia](https://github.com/Sun-Lab-NBB/sollertia) platform, which is built on the
[Ataraxis](https://github.com/Sun-Lab-NBB/ataraxis) framework.

### Key areas

| Directory  | Purpose                                                                              |
|------------|--------------------------------------------------------------------------------------|
| `src/`     | Firmware source: per-module headers and `main.cpp` per-target entry point            |
| `docs/`    | Sphinx + Breathe documentation source (consumes Doxygen XML)                         |

### Architecture

- **Multi-target firmware**: `main.cpp` uses preprocessor `#ifdef` blocks to select which `Module` subclass
  instances are compiled into the firmware. Exactly one target macro is defined at upload time. The current
  Mesoscope-VR deployment defines three targets (`ACTOR`, `SENSOR`, `ENCODER`) and requires all three target
  firmwares running on three separate boards; a different acquisition system could define any other set of targets
  with any partitioning of the available modules across boards.
- **Per-target controller IDs**: assigned per acquisition system in `main.cpp`. The current Mesoscope-VR
  deployment uses `ACTOR = 101`, `SENSOR = 152`, `ENCODER = 203`. These IDs are part of the contract with the
  consuming acquisition system's binding classes in `sollertia-experiment`.
- **Keepalive watchdog**: `kKeepaliveInterval = 500` ms in the current deployment. The Kernel expects the host PC
  to send a keepalive message at least this often; if it does not, the microcontroller emergency-resets to abort
  runtime. The interval is doubled internally by the Kernel to tolerate brief communication lapses. The value is
  shared across all targets; a future consumer would pick its own value.
- **One `Module` subclass per hardware role**: Seven headers under `src/` (brake, encoder, lick, screen, torque,
  ttl, valve) each declare a `final` class that inherits from `ataraxis-micro-controller`'s `Module` base. Each
  implements three pure virtual methods (`SetupModule`, `SetCustomParameters`, `RunActiveCommand`) and exposes
  per-module `kCustomStatusCodes` and `kModuleCommands` enums.

### Core components

The Mesoscope-VR column below shows where each module is instantiated under the current Mesoscope-VR deployment.
A future acquisition system could partition these modules differently — every module in this table is
platform-general and consumable by any target.

| Component       | File               | Purpose                                                          | Mesoscope-VR target |
|-----------------|--------------------|------------------------------------------------------------------|---------------------|
| `BrakeModule`   | `brake_module.h`   | Controls electromagnetic particle brake on the running wheel     | ACTOR               |
| `ValveModule`   | `valve_module.h`   | Drives solenoid valve (water reward + tone buzzer; gas puff)     | ACTOR               |
| `ScreenModule`  | `screen_module.h`  | Pulses VR screen power-board FET gates                           | ACTOR               |
| `LickModule`    | `lick_module.h`    | Monitors conductive lick sensor voltage                          | SENSOR              |
| `TorqueModule`  | `torque_module.h`  | Monitors AD620-amplified torque sensor on the running wheel      | SENSOR              |
| `TTLModule`     | `ttl_module.h`     | Emits or reads TTL pulses for external hardware synchronization  | SENSOR              |
| `EncoderModule` | `encoder_module.h` | Monitors quadrature encoder with hardware-interrupt pulse count  | ENCODER             |
| `main.cpp`      | `main.cpp`         | Per-target module instantiation, `setup()` and `loop()` entry    | All                 |

### Module type and ID assignments

Module type codes (`module_type` argument to each `Module` constructor) are assigned per hardware role; they MUST
not be reused across slmc. Each module also carries a `module_id` to disambiguate multiple instances of the same
type on the same controller (currently used only for the two `ValveModule` instances on Mesoscope-VR's ACTOR
target). The table below shows the current Mesoscope-VR deployment's assignments.

| Type | Module           | Mesoscope-VR controller | Instances                                      |
|------|------------------|-------------------------|------------------------------------------------|
| 1    | `TTLModule`      | SENSOR                  | `mesoscope_frame` (id 1)                       |
| 2    | `EncoderModule`  | ENCODER                 | `wheel_encoder` (id 1)                         |
| 3    | `BrakeModule`    | ACTOR                   | `wheel_brake` (id 1)                           |
| 4    | `LickModule`     | SENSOR                  | `lick_sensor` (id 1)                           |
| 5    | `ValveModule`    | ACTOR                   | `reward_valve` (id 1), `gas_puff_valve` (id 2) |
| 6    | `TorqueModule`   | SENSOR                  | `torque_sensor` (id 1)                         |
| 7    | `ScreenModule`   | ACTOR                   | `screen_trigger` (id 1)                        |

### Key patterns

- **Header-only modules**: All `Module` subclasses live in `.h` files under `src/`. There are no `.cpp` files for
  modules; everything is template-instantiated at compile time from `main.cpp`.
- **Template-parameterized pin assignments**: Each module class is a template with pin and behavior parameters
  (e.g., `BrakeModule<kPin, kNormallyEngaged, kStartEngaged>`). `main.cpp` instantiates each module with
  target-specific values.
- **Stage-based command execution**: Multi-step commands (e.g., `ValveModule::Pulse`, `BrakeModule::SendPulse`) use
  `AdvanceCommandStage()` + `WaitForMicros()` for non-blocking execution across `RuntimeCycle()` iterations. The
  blocking exception is calibration commands (`ValveModule::Calibrate`, `EncoderModule::GetPPR`), which run as
  intentional in-place loops with `@warning` annotations on their Doxygen blocks.
- **PACKED_STRUCT serialization**: Each module's `CustomRuntimeParameters` struct uses `PACKED_STRUCT` for byte-level
  binary compatibility with the companion Python `ModuleInterface`.
- **Status code returns**: All operations return boolean / enum status codes rather than throwing exceptions,
  consistent with embedded C++ patterns.
- **Custom status codes 51-250**: Module-specific `kCustomStatusCodes` use the 51-250 range reserved for module
  subclasses by `ataraxis-micro-controller`.
- **LED-pin static_assert**: Every module class begins with a `static_assert(kPin != LED_BUILTIN, ...)` to prevent
  accidental reuse of the LED pin for hardware control.
- **Library-prefixed include guards**: All headers use `AXMC_<NAME>_MODULE_H` include guards.
- **`get_active_command()` and `get_command_stage()` accessors**: Base-class accessors (snake_case) are called from
  each module's `RunActiveCommand()` dispatch switch and from stage-based command implementations.

### Dependencies

| Library                       | Purpose                                              | Source             |
|-------------------------------|------------------------------------------------------|--------------------|
| `Arduino.h`                   | Core Arduino framework (Serial, Stream, types)       | platform-bundled   |
| `digitalWriteFast`            | Fast GPIO read/write operations                      | PlatformIO library |
| `Encoder`                     | Quadrature encoder pulse counting (Paul Stoffregen)  | PlatformIO library |
| `ataraxis-transport-layer-mc` | CRC-16 checksummed serial communication with COBS    | PlatformIO library |
| `ataraxis-micro-controller`   | `Kernel`, `Communication`, `Module` base class       | PlatformIO library |

### Build system

This is a PlatformIO firmware project. `platformio.ini` defines a single environment, currently targeting the
Teensy 4.1 board used by the Mesoscope-VR deployment:

| Environment | Board      | Platform | Monitor speed |
|-------------|------------|----------|---------------|
| `teensy41`  | Teensy 4.1 | teensy   | 115200        |

The target controller is selected at compile time by modifying the `#define` macro on line 26 of `src/main.cpp`
before each upload. Only one board must be connected to the host PC at upload time.

Adding support for a new board family is a `platformio.ini` change (new environment) plus any board-specific
adjustments to pin assignments in the consumer's `main.cpp` target block.

### Development commands

```bash
pio run                          # Compile firmware for the active target macro in main.cpp
pio run -t upload                # Compile and flash to the connected board
pio check                        # Run static analysis (cppcheck)
tox -e docs                      # Build Sphinx + Doxygen API documentation
```

### Workflow guidance

**Adding a new hardware module to the firmware:**

1. Invoke `experiment:microcontroller-interface` first to understand the cross-repo paired Module + Interface
   contract (slmc firmware conventions + sle Python wrapper conventions + the cross-side agreement they must
   honor). Allocate a new module type code from the registry in
   `experiment:microcontroller-interface`'s `references/module-catalog.md`.
2. Invoke `microcontroller:firmware-module` for the base C++ Module subclass mechanics (template parameter
   conventions, `CustomRuntimeParameters` struct, `kCustomStatusCodes` / `kModuleCommands` enums, stage-based
   command execution, `SendData` patterns) that `experiment:microcontroller-interface` extends.
3. Write the new header in `src/<module>_module.h` following the slmc conventions documented in
   `experiment:microcontroller-interface`'s "slmc firmware conventions" section.
4. Add the module's `#include` and instantiation block to the appropriate target in `src/main.cpp` (for the
   current Mesoscope-VR deployment, this means choosing one of ACTOR / SENSOR / ENCODER; for a different
   consumer, the choice depends on that system's target layout). Add the new instance to the
   `Module* modules[]` array for that target.
5. Add the new header to `Doxyfile`'s `INPUT` list and to `docs/source/api.rst` for documentation coverage.
6. Update `experiment:microcontroller-interface`'s `references/module-catalog.md` with the new entry.
7. Bump the slmc version (git tag) and flash to the affected board(s).
8. Hand off to sollertia-experiment for the host-PC side. The handoff splits in two:
   - **Python wrapper** (system-agnostic): author the new `ModuleInterface` subclass in
     `sollertia-experiment/src/sollertia_experiment/cross_system/module_interfaces.py` following
     `experiment:microcontroller-interface`'s "sle Python wrapper conventions" section.
   - **Binding-class integration** (consumer-specific): for the current Mesoscope-VR consumer, hand off to
     `experiment:mesoscope-vr` to add calibration fields to `MesoscopeMicroControllers`, extend
     `MicroControllerInterfaces` to instantiate the new wrapper, and regenerate the system YAML. Bump the
     sollertia-experiment version so older deployments refuse to load against the new schema.

**Modifying an existing module's parameter structure or status codes:**

1. Read the relevant header in `src/` to understand the current `CustomRuntimeParameters` and `kCustomStatusCodes`.
2. Verify the corresponding host-PC `ModuleInterface` (in
   `sollertia-experiment/src/sollertia_experiment/cross_system/module_interfaces.py`) and the consuming
   system's calibration fields (for Mesoscope-VR: `MesoscopeMicroControllers` in
   `sollertia-experiment/src/sollertia_experiment/mesoscope_vr/system.py`) before making changes.
   Cross-repository drift is a runtime hazard.
3. Make the firmware change, bump the slmc version, and coordinate companion changes via
   `experiment:microcontroller-interface` (wrapper-side) and the consumer's instance skill (binding-side; for
   Mesoscope-VR, `experiment:mesoscope-vr`).
4. Re-flash all affected boards (a parameter-struct change typically affects only the one target that hosts the
   module, but a status-code change may ripple across PC-side log processing).

**Modifying controller IDs, keepalive interval, or per-target module layout:**

1. These are top-level cross-repository contracts. Coordinate with the consuming acquisition system's maintainers
   before changing. For the current Mesoscope-VR consumer, this means coordinating with
   `experiment:mesoscope-vr`'s maintenance contract.
2. Update `main.cpp` (controller IDs, keepalive interval, module instantiation order) and propagate the changes
   to the matching constants in the consumer's binding class (for Mesoscope-VR: `MicroControllerInterfaces` in
   `sollertia-experiment/src/sollertia_experiment/mesoscope_vr/binding_classes.py`).
3. Update the README's "Per-Target Configuration" section to reflect the new values.

**Modifying build configuration, documentation, or style:**

1. C++ style changes: invoke `/cpp-style` and run `pio check` to catch shadowing, unused variables, etc.
2. README / Sphinx docs changes: invoke `/readme-style` or `/api-docs` as appropriate. `tox -e docs` must succeed.
3. tox.ini changes: invoke `/tox-config`.
4. Build flag, clang-format, or clang-tidy changes: verify against the canonical assets shipped with `/cpp-style`.

**Important considerations:**

- Module type codes are `uint8_t`; the `(type, id)` pair must be unique across all modules on a single controller.
- Controller IDs are `uint8_t`; the current Mesoscope-VR deployment uses 101 / 152 / 203. The firmware `Kernel`
  requires uniqueness across concurrently-connected microcontrollers but does not document a reserved value at
  the source layer. Cross-reference the PC-side `ataraxis-communication-interface` conventions before reusing any
  low values, and coordinate any new ID choice with the consuming acquisition system.
- Pin selection must avoid `LED_BUILTIN`; the per-module `static_assert` enforces this at compile time.
- Calibration commands (`ValveModule::Calibrate`, `EncoderModule::GetPPR`) intentionally block the runtime; they
  must never run during an active acquisition session. Their Doxygen `@warning` blocks document this.
