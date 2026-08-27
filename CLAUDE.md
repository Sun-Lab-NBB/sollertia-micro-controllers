# Claude Code Instructions

## Session start behavior

At the beginning of each coding session, before making any code changes, you should build a comprehensive
understanding of the codebase by invoking the `automation:explore-codebase` skill.

This ensures you:
- Understand the project architecture before modifying code
- Follow existing patterns and conventions
- Do not introduce inconsistencies or break integrations with downstream consumers of this firmware library
  (currently the Mesoscope-VR acquisition system in sollertia-experiment, with future acquisition systems consuming
  the same firmware library)

## Style guide compliance

You MUST invoke the appropriate skill before performing ANY of the following tasks:

| Task                                       | Skill to invoke                |
|--------------------------------------------|--------------------------------|
| Writing or modifying C++ code              | `automation:cpp-style`         |
| Writing or modifying README files          | `automation:readme-style`      |
| Writing or modifying Sphinx docs files     | `automation:api-docs`          |
| Writing or modifying platformio.ini        | `automation:platformio-config` |
| Writing or modifying tox.ini               | `automation:tox-config`        |
| Writing git commit messages                | `automation:commit`            |
| Writing or modifying skill files / this MD | `automation:skill-design`      |
| Auditing for style compliance              | `automation:audit-style`       |
| Auditing for factual accuracy              | `automation:audit-facts`       |
| Auditing for bugs and edge cases           | `automation:audit-correctness` |
| Auditing for speed and memory use          | `automation:audit-performance` |

Each skill contains a verification checklist that you MUST complete before submitting any work. Failure to invoke the
appropriate skill results in style violations that block release.

## Cross-referenced library verification

This firmware depends on two ataraxis C++ libraries and is consumed by one Sollertia Python library. Local clones
of all three typically live alongside this repository, in its parent directory.

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

| Skill                                  | Purpose                                                                                            |
|----------------------------------------|----------------------------------------------------------------------------------------------------|
| `microcontroller:firmware-module`      | Base C++ `Module` subclass mechanics (ataraxis base template)                                      |
| `experiment:microcontroller-interface` | Paired firmware Module + host-PC `ModuleInterface` contract registry                               |
| `experiment:library-extension`         | Firmware module, controller target, and board family extension seams, and the cross-repo constants |

The Sollertia platform's development and style skills required for routine changes ship in the ataraxis marketplace's
`automation` plugin. Invoke them as directed by the "Session start behavior" and "Style guide compliance" sections
above.

Everything else this firmware touches lives on the consumer side. The sollertia marketplace's `experiment` plugin and
the downstream [sollertia-experiment](https://github.com/Sun-Lab-NBB/sollertia-experiment) (sle) library own the
host-PC interface wrappers, the acquisition-system binding classes, and the per-system configuration and runtime
surface. The `experiment:microcontroller-interface` skill links out to the ataraxis `communication` plugin for the
base host-PC `ModuleInterface` API. This consumer surface changes per acquisition system (Mesoscope-VR is the only
current consumer), so this file does not enumerate it. When a change reaches the consumer side, inspect the
`experiment` plugin's skills and the `sollertia-experiment` library to determine which are currently relevant.

**Canonical reading order when adding or modifying a firmware module:**
1. `experiment:microcontroller-interface` covers the cross-repo paired Module + Interface contract. Allocate the new
   module type code and follow the slmc firmware + sle wrapper conventions it documents.
2. `experiment:library-extension` covers the seam view. Read it when the change adds a controller target or a board
   family rather than a module, because those two seams carry different sollertia-experiment mirrors than a new module
   does, and read it first when the driver of the change is a new acquisition system consuming this firmware rather
   than new hardware on an existing one.
3. `microcontroller:firmware-module` covers the base C++ `Module` subclass mechanics that the skill above extends.
4. For consumer-side changes (binding classes, system configuration, post-flash hardware setup), consult the
   `experiment` plugin and the `sollertia-experiment` library for the consuming acquisition system's current surface,
   which is Mesoscope-VR today. A future consumer would expose its own skills.

***Note,*** the sollertia `experiment` and `mesoscope` plugins may be unavailable on hosts where the sollertia
marketplace is not installed (the live `available-skills` list will not include any `experiment:*` or `mesoscope:*`
entries). The source for each skill lives at `sollertia/plugins/<plugin>/skills/<skill>/SKILL.md`. Consult these
files directly when the slash-command form is not available.

## Companion library synchronization

This firmware lives on one end of a two-repository contract with `sollertia-experiment`, which owns the host-PC
`ModuleInterface` wrappers (system-agnostic, in `src/sollertia_experiment/cross_system/module_interfaces.py`)
and the per-acquisition-system binding classes and configuration dataclasses. The current consumer is the
Mesoscope-VR system. Its binding classes and `MesoscopeMicroControllers` configuration dataclass live in
`src/sollertia_experiment/mesoscope_vr/`.

Any change to a `Module` subclass's parameter structure, status codes, command codes, controller IDs, keepalive
interval, or per-target module layout MUST be synchronized with the corresponding changes in sollertia-experiment.

`experiment:microcontroller-interface` owns this synchronization list. It pairs every firmware constant below with the
exact sollertia-experiment symbol that must move with it, and it carries the per-module conventions and the catalog of
modules that currently exist. `experiment:library-extension` catalogues the three extension seams, a new firmware
module, a new controller target, and a new board family, with the sollertia-experiment mirror each one obliges.

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
- Controller IDs (currently `ACTOR = 101`, `SENSOR = 152`, `ENCODER = 203` for the Mesoscope-VR consumer, with a
  future consumer having its own assignments)
- Keepalive interval (`kKeepaliveInterval`, currently 500 ms, which is Mesoscope-VR's chosen cadence)
- Per-target module layout (which `Module` subclass instances live on which controller for each acquisition
  system) and module `(type, id)` assignments

**What does NOT require synchronization:**
- Internal `Module` implementation details (stage-based command execution, intermediate state variables)
- Build configuration (`platformio.ini`, `tox.ini`, `Doxyfile`, `.clang-format`, `.clang-tidy`)
- Doxygen comments, inline comments, file-level docstrings
- LED error indication, ADC resolution settings, baud rate

## Distribution model

This repository ships firmware source and its own project instructions, and it publishes no skills of its own. The
skills that cover this firmware are distributed separately, through two marketplaces: the ataraxis marketplace's
`automation` plugin carries the style, audit, and workflow skills and its `microcontroller` plugin carries
`firmware-module`, while the sollertia marketplace's `experiment` plugin carries `microcontroller-interface` and
`library-extension`. A skill edit or a skill defect report lands in the owning marketplace repository rather than here.

## Project context

This is **sollertia-micro-controllers**, a C++17 PlatformIO firmware library that specializes the general
microcontroller framework provided by `ataraxis-micro-controller` into the concrete hardware modules used by
Sollertia platform data acquisition systems. The firmware is Arduino-compatible at the framework level and is
not locked to any single board family. The current deployment targets Teensy 4.1 boards because that is the
hardware the only currently-supported consumer (the Mesoscope-VR acquisition system) uses. Modules are exposed
to the host PC through the [sollertia-experiment](https://github.com/Sun-Lab-NBB/sollertia-experiment) runtime
within the [Sollertia](https://github.com/Sun-Lab-NBB/sollertia) platform, which is built on the
[Ataraxis](https://github.com/Sun-Lab-NBB/ataraxis) framework.

`.claude/rules/firmware-workflows.md` autoloads alongside this file and carries the development commands and the
module-addition, parameter-change, controller-ID, and build-configuration workflows.

### Key areas

| Directory  | Purpose                                                                              |
|------------|--------------------------------------------------------------------------------------|
| `src/`     | Firmware source: per-module headers and `main.cpp` per-target entry point            |
| `docs/`    | Sphinx + Breathe documentation source (consumes Doxygen XML)                         |

### Architecture

- **Multi-target firmware**: `main.cpp` uses preprocessor `#ifdef` blocks to select which `Module` subclass
  instances are compiled into the firmware. Each PlatformIO environment defines exactly one target macro through its
  build flags. The current Mesoscope-VR deployment defines three targets (`ACTOR`, `SENSOR`, `ENCODER`) and requires
  all three target firmwares running on three separate boards. A different acquisition system could define any other
  set of targets with any partitioning of the available modules across boards.
- **Per-target controller IDs**: assigned per acquisition system in `main.cpp`. The current Mesoscope-VR
  deployment uses `ACTOR = 101`, `SENSOR = 152`, `ENCODER = 203`. These IDs are part of the contract with the
  consuming acquisition system's binding classes in `sollertia-experiment`.
- **Keepalive watchdog**: `kKeepaliveInterval = 500` ms in the current deployment. The Kernel expects the host PC
  to send a keepalive message at least this often. If it does not, the microcontroller emergency-resets to abort
  runtime. The interval is doubled internally by the Kernel to tolerate brief communication lapses. The value is
  shared across all targets, and a future consumer would pick its own value.
- **One `Module` subclass per hardware role**: Seven headers under `src/` (brake, encoder, lick, screen, torque,
  ttl, valve) each declare a `final` class that inherits from `ataraxis-micro-controller`'s `Module` base. Each
  implements three pure virtual methods (`SetupModule`, `SetCustomParameters`, `RunActiveCommand`) and exposes
  per-module `kCustomStatusCodes` and `kModuleCommands` enums.

### Core components

The Mesoscope-VR column below shows where each module is instantiated under the current Mesoscope-VR deployment.
A future acquisition system could partition these modules differently, because every module in this table is
platform-general and consumable by any target.

| Component       | File               | Purpose                                                          | Mesoscope-VR target |
|-----------------|--------------------|------------------------------------------------------------------|---------------------|
| `BrakeModule`   | `brake_module.h`   | Controls electromagnetic particle brake on the running wheel     | ACTOR               |
| `ValveModule`   | `valve_module.h`   | Drives solenoid valve (water reward + tone buzzer, gas puff)     | ACTOR               |
| `ScreenModule`  | `screen_module.h`  | Pulses VR screen power-board FET gates                           | ACTOR               |
| `LickModule`    | `lick_module.h`    | Monitors conductive lick sensor voltage                          | SENSOR              |
| `TorqueModule`  | `torque_module.h`  | Monitors AD620-amplified torque sensor on the running wheel      | SENSOR              |
| `TTLModule`     | `ttl_module.h`     | Emits or reads TTL pulses for external hardware synchronization  | SENSOR              |
| `EncoderModule` | `encoder_module.h` | Monitors quadrature encoder with hardware-interrupt pulse count  | ENCODER             |
| `main.cpp`      | `main.cpp`         | Per-target module instantiation, `setup()` and `loop()` entry    | All                 |

Module type codes (`module_type` argument to each `Module` constructor) are assigned per hardware role, and they MUST
not be reused across slmc. The README's "Per-Target Configuration" section lists the current Mesoscope-VR deployment's
type-code and instance-ID assignments.

### Key patterns

- **Header-only modules**: All `Module` subclasses live in `.h` files under `src/`. There are no `.cpp` files for
  modules, and everything is template-instantiated at compile time from `main.cpp`.
- **Template-parameterized pin assignments**: Each module class is a template with pin and behavior parameters
  (e.g., `BrakeModule<kPin, kNormallyEngaged, kStartEngaged>`). `main.cpp` instantiates each module with
  target-specific values.
- **Stage-based command execution**: Multi-step commands (e.g., `ValveModule::Pulse`, `BrakeModule::SendPulse`) use
  `AdvanceCommandStage()` + `WaitForMicros()` for non-blocking execution across `RuntimeCycle()` iterations. The
  blocking exception is calibration commands (`ValveModule::Calibrate`, `EncoderModule::GetPPR`), which run as
  intentional in-place loops with `@warning` annotations on their Doxygen blocks. Both warnings mark the command
  offline-only, because a block that outlasts `kKeepaliveInterval` trips the Kernel's emergency reset. Calibration is
  experimenter-operated from the consumer's maintenance runtime.
- **PACKED_STRUCT serialization**: Each module's `CustomRuntimeParameters` struct uses `PACKED_STRUCT` for byte-level
  binary compatibility with the companion Python `ModuleInterface`.
- **Status code returns**: All operations return boolean / enum status codes rather than throwing exceptions,
  consistent with embedded C++ patterns.
- **Custom status codes 51-250**: Module-specific `kCustomStatusCodes` use the 51-250 range reserved for module
  subclasses by `ataraxis-micro-controller`.
- **LED-pin static_assert**: Every module class opens with `static_assert` blocks that reject `LED_BUILTIN` for each
  of its pin template parameters, preventing accidental reuse of the LED pin for hardware control.
- **Library-prefixed include guards**: All headers use `SLMC_<NAME>_MODULE_H` include guards.
- **`get_active_command()` and `get_command_stage()` accessors**: Base-class accessors (snake_case) are called from
  each module's `RunActiveCommand()` dispatch switch and from stage-based command implementations.

### Dependencies

| Library                       | Purpose                                                   | Source             |
|-------------------------------|-----------------------------------------------------------|--------------------|
| `Arduino.h`                   | Core Arduino framework (Serial, Stream, types, fast GPIO) | platform-bundled   |
| `Encoder`                     | Quadrature encoder pulse counting (Paul Stoffregen)       | PlatformIO library |
| `ataraxis-transport-layer-mc` | CRC-16 checksummed serial communication with COBS         | PlatformIO library |
| `ataraxis-micro-controller`   | `Kernel`, `Communication`, `Module` base class            | PlatformIO library |

### Build system

This is a PlatformIO firmware project. `platformio.ini` defines one environment per controller target, all currently
targeting the Teensy 4.1 board used by the Mesoscope-VR deployment:

| Environment        | Board      | Platform | Target macro | Monitor speed |
|--------------------|------------|----------|--------------|---------------|
| `teensy41_actor`   | Teensy 4.1 | teensy   | `ACTOR`      | 115200        |
| `teensy41_sensor`  | Teensy 4.1 | teensy   | `SENSOR`     | 115200        |
| `teensy41_encoder` | Teensy 4.1 | teensy   | `ENCODER`    | 115200        |

Each environment extends the shared `[teensy41_base]` template and appends its target macro to `build_flags`. Running
`pio run` without `-e` compiles all three targets, so a break in a target other than the one being flashed fails the
build. Only one board must be connected to the host PC at upload time. An upload MUST name its environment, because
an upload command without `-e` flashes the connected board once per environment and leaves it running the last one.

***Exemption from `automation:platformio-config`:*** that skill mandates one `[env:<board>]` section named for the
board, which assumes the library archetype where the board is the only build axis. This firmware carries a second axis,
the controller target, so its environments are named `<board>_<target>` and share the non-buildable `[teensy41_base]`
template that holds every field common to them. Preserve this layout when editing `platformio.ini`.

Adding support for a new board family is a `platformio.ini` change (a new base template mirroring `[teensy41_base]`,
plus one environment per controller target) and any board-specific adjustments to the pin literals in this
repository's own `src/main.cpp` target blocks. It also requires re-checking the per-module `LED_BUILTIN` asserts, that
the board supports `analogReadResolution(12)`, and that the `Encoder` library supports the new architecture's
interrupt pins. `experiment:library-extension` carries the full step list for this seam and for the seam that adds a
new controller target.

### Issue templates

The `.github/ISSUE_TEMPLATE/` forms carry no fields beyond the general ataraxis templates that
`automation:project-layout` prescribes. `bug_report.yml` differs from the general form only by the sanctioned
`{environment_example}` substitution, which fills the existing Environment field with the `OS:`, `PlatformIO:`, and
`Board:` lines plus a `Firmware target:` line naming the target the affected board ran. `feature_request.yml` is
verbatim. `config.yml` carries the `{project}` substitution and a second AI development assets link, because this
repository's skills are split between the ataraxis and sollertia marketplaces. Audit the corpus by diffing it against
the `automation:project-layout` assets and requiring an exact match outside those substitutions.
