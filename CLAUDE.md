# Claude Code Instructions

## Session start behavior

At the beginning of each coding session, before making any code changes, you should build a comprehensive
understanding of the codebase by invoking the `/explore-codebase` skill.

This ensures you:
- Understand the project architecture before modifying code
- Follow existing patterns and conventions
- Do not introduce inconsistencies or break integrations with the downstream Sollertia library that consumes this
  firmware (sollertia-experiment)

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

| Library                       | Direction       | Role                                                                                                                                            |
|-------------------------------|-----------------|-------------------------------------------------------------------------------------------------------------------------------------------------|
| `ataraxis-micro-controller`   | Upstream        | Provides `Kernel`, `Communication`, `Module` base class                                                                                         |
| `ataraxis-transport-layer-mc` | Upstream        | Bidirectional serial communication with CRC and COBS                                                                                            |
| `sollertia-experiment`        | Downstream (PC) | Owns the host-PC binding classes that wrap each `Module` plus the `MesoscopeMicroControllers` calibration dataclass (`mesoscope_vr/configuration.py`) |

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

The sollertia marketplace ships an `experiment` plugin with skills that target this firmware directly via
cross-plugin handoffs. The ataraxis marketplace ships several plugins used across all Sun Lab repositories, of which
the `microcontroller`, `communication`, and `automation` plugins are relevant to this firmware.

**Canonical reading order when adding or modifying a firmware module:**
1. `experiment:microcontroller-interface` — Sollertia binding contract (calibration field naming, version-bump rule)
2. `microcontroller:firmware-module` — C++ Module subclass mechanics in this repository
3. `communication:microcontroller-interface` — host-PC `ModuleInterface` wiring in sollertia-experiment
4. `communication:microcontroller-setup` — post-flash hardware enumeration / verification

All skills below ship as part of the marketplace / plugin pair shown in the **Plugin** column. The marketplace is
`ataraxis` for the first three plugins (`automation`, `microcontroller`, `communication`) and `sollertia` for the
`experiment` plugin.

| Skill                                      | Plugin            | Purpose                                                          |
|--------------------------------------------|-------------------|------------------------------------------------------------------|
| `/explore-codebase`                        | `automation`      | Perform in-depth codebase exploration at session start           |
| `/cpp-style`                               | `automation`      | Apply Sun Lab C++ conventions (REQUIRED for C++ changes)         |
| `/readme-style`                            | `automation`      | Apply Sun Lab README conventions (REQUIRED for README changes)   |
| `/api-docs`                                | `automation`      | Apply Sun Lab Sphinx documentation conventions                   |
| `/tox-config`                              | `automation`      | Apply Sun Lab tox.ini conventions                                |
| `/commit`                                  | `automation`      | Draft Sun Lab style-compliant git commit messages                |
| `/skill-design`                            | `automation`      | Generate, update, and verify skill files and this CLAUDE.md      |
| `/audit-style`                             | `automation`      | Audit files against applicable style skill checklists            |
| `/audit-facts`                             | `automation`      | Audit documentation against source code for factual accuracy     |
| `microcontroller:firmware-module`          | `microcontroller` | Guide creation of custom hardware `Module` subclasses            |
| `communication:microcontroller-interface`  | `communication`   | Write host-PC `ModuleInterface` code that consumes this firmware |
| `communication:microcontroller-setup`      | `communication`   | Discover microcontrollers, verify MQTT, inspect manifests        |
| `experiment:microcontroller-interface`     | `experiment`      | Sollertia binding contract for `MesoscopeMicroControllers`       |
| `experiment:modifying-mesoscope-vr-system` | `experiment`      | Entry router for hardware changes that span firmware + binding   |
| `experiment:system-configuration`          | `experiment`      | Source of truth for `MesoscopeMicroControllers` YAML writes      |
| `experiment:pipeline`                      | `experiment`      | End-to-end Sollertia experiment lifecycle orchestration          |
| `experiment:acquisition-system-setup`      | `experiment`      | Post-flash hardware enumeration and configuration verification   |

***Note,*** the sollertia `experiment` plugin may be unavailable on hosts where the sollertia marketplace is not
installed (the live `available-skills` list will not include any `experiment:*` entries). The source for each skill
lives at `sollertia/plugins/experiment/skills/<skill>/SKILL.md`; consult these files directly when the slash-command
form is not available.

## Downstream library integration

This firmware lives on one end of a two-repository contract with sollertia-experiment, which owns both the host-PC
binding classes (`MicroControllerInterfaces` and per-module `ModuleInterface` subclasses) and the
`MesoscopeMicroControllers` configuration dataclass (`src/sl_experiment/mesoscope_vr/configuration.py`). Any change
to a `Module` subclass's parameter structure, status codes, command codes, controller IDs, keepalive interval, or
per-target module layout MUST be synchronized with the corresponding changes in sollertia-experiment.

**Before modifying any cross-repository contract, you MUST:**

1. **Identify the companion repository**: Check for a local copy at `../sollertia-experiment/`. If unavailable, use
   `gh api repos/Sun-Lab-NBB/sollertia-experiment` to access the remote repository.

2. **Review the corresponding implementation**: Read the host-PC `ModuleInterface` subclass in sollertia-experiment
   that consumes the firmware module you are modifying, and the matching `MesoscopeMicroControllers` calibration
   fields in `sollertia-experiment/src/sl_experiment/mesoscope_vr/configuration.py` that parameterize it. Verify
   that both repositories are currently in sync before making changes.

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
- Controller IDs (`ACTOR = 101`, `SENSOR = 152`, `ENCODER = 203`)
- Keepalive interval (`kKeepaliveInterval`, currently 500 ms)
- Per-target module layout (which `Module` subclass instances live on which controller) and module `(type, id)`
  assignments

**What does NOT require synchronization:**
- Internal `Module` implementation details (stage-based command execution, intermediate state variables)
- Build configuration (`platformio.ini`, `tox.ini`, `Doxyfile`, `.clang-format`, `.clang-tidy`)
- Doxygen comments, inline comments, file-level docstrings
- LED error indication, ADC resolution settings, baud rate

## Project context

This is **sollertia-micro-controllers**, a C++17 PlatformIO firmware project that runs on the three Teensy 4.1
microcontrollers used by Sollertia platform data acquisition systems. It specializes the general microcontroller
framework provided by `ataraxis-micro-controller` into concrete hardware modules consumed by the
[sollertia-experiment](https://github.com/Sun-Lab-NBB/sollertia-experiment) host-PC runtime. The firmware targets
Teensy 4.1 boards within the [Sollertia](https://github.com/Sun-Lab-NBB/sollertia) platform, which is built on the
[Ataraxis](https://github.com/Sun-Lab-NBB/ataraxis) framework.

### Key areas

| Directory  | Purpose                                                                  |
|------------|--------------------------------------------------------------------------|
| `src/`     | Firmware source: 7 module headers and `main.cpp` per-target entry point  |
| `docs/`    | Sphinx + Breathe documentation source (consumes Doxygen XML)             |

### Architecture

- **Three-target firmware**: `main.cpp` uses `#ifdef ACTOR / #elif defined SENSOR / #elif defined ENCODER` to select
  which `Module` subclass instances are compiled into the firmware. Exactly one target macro is defined at upload
  time. The Mesoscope-VR system requires all three target firmwares running on three separate Teensy 4.1 boards.
- **Per-target controller IDs**: `ACTOR = 101`, `SENSOR = 152`, `ENCODER = 203`. These IDs are part of the contract
  with `sollertia-experiment`'s `MicroControllerInterfaces` binding class.
- **Keepalive watchdog**: `kKeepaliveInterval = 500` ms. The Kernel expects the host PC to send a keepalive message
  at least this often; if it does not, the microcontroller emergency-resets to abort runtime. The interval is doubled
  internally by the Kernel to tolerate brief communication lapses.
- **One `Module` subclass per hardware role**: Seven headers under `src/` (brake, encoder, lick, screen, torque, ttl,
  valve) each declare a `final` class that inherits from `ataraxis-micro-controller`'s `Module` base. Each implements
  three pure virtual methods (`SetupModule`, `SetCustomParameters`, `RunActiveCommand`) and exposes per-module
  `kCustomStatusCodes` and `kModuleCommands` enums.

### Core components

| Component       | File               | Purpose                                                          | Target  |
|-----------------|--------------------|------------------------------------------------------------------|---------|
| `BrakeModule`   | `brake_module.h`   | Controls electromagnetic particle brake on the running wheel     | ACTOR   |
| `ValveModule`   | `valve_module.h`   | Drives solenoid valve (water reward + tone buzzer; gas puff)     | ACTOR   |
| `ScreenModule`  | `screen_module.h`  | Pulses VR screen power-board FET gates                           | ACTOR   |
| `LickModule`    | `lick_module.h`    | Monitors conductive lick sensor voltage                          | SENSOR  |
| `TorqueModule`  | `torque_module.h`  | Monitors AD620-amplified torque sensor on the running wheel      | SENSOR  |
| `TTLModule`     | `ttl_module.h`     | Emits or reads TTL pulses for external hardware synchronization  | SENSOR  |
| `EncoderModule` | `encoder_module.h` | Monitors quadrature encoder with hardware-interrupt pulse count  | ENCODER |
| `main.cpp`      | `main.cpp`         | Per-target module instantiation, `setup()` and `loop()` entry    | All     |

### Module type and ID assignments

Module type codes (`module_type` argument to each `Module` constructor) are assigned per hardware role; they MUST
not be reused across slmc. Each module also carries a `module_id` to disambiguate multiple instances of the same
type on the same controller (currently used only for the two `ValveModule` instances on ACTOR).

| Type | Module           | Controller | Instances                                      |
|------|------------------|------------|------------------------------------------------|
| 1    | `TTLModule`      | SENSOR     | `mesoscope_frame` (id 1)                       |
| 2    | `EncoderModule`  | ENCODER    | `wheel_encoder` (id 1)                         |
| 3    | `BrakeModule`    | ACTOR      | `wheel_brake` (id 1)                           |
| 4    | `LickModule`     | SENSOR     | `lick_sensor` (id 1)                           |
| 5    | `ValveModule`    | ACTOR      | `reward_valve` (id 1), `gas_puff_valve` (id 2) |
| 6    | `TorqueModule`   | SENSOR     | `torque_sensor` (id 1)                         |
| 7    | `ScreenModule`   | ACTOR      | `screen_trigger` (id 1)                        |

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

This is a PlatformIO firmware project targeting Teensy 4.1. `platformio.ini` defines a single environment:

| Environment | Board      | Platform | Monitor speed |
|-------------|------------|----------|---------------|
| `teensy41`  | Teensy 4.1 | teensy   | 115200        |

The target microcontroller (ACTOR / SENSOR / ENCODER) is selected at compile time by modifying the `#define` macro
on line 26 of `src/main.cpp` before each upload. Only one Teensy 4.1 must be connected to the host PC at upload time.

### Development commands

```bash
pio run                          # Compile firmware for the active target macro in main.cpp
pio run -t upload                # Compile and flash to the connected Teensy 4.1
pio check                        # Run static analysis (cppcheck)
tox -e docs                      # Build Sphinx + Doxygen API documentation
```

### Workflow guidance

**Adding a new hardware module to the firmware:**

1. Invoke `experiment:microcontroller-interface` first to understand the cross-repo binding contract — which
   controller the module belongs on, what calibration fields it needs in `MesoscopeMicroControllers` (defined in
   `sollertia-experiment/src/sl_experiment/mesoscope_vr/configuration.py`), and how the host-PC `ModuleInterface`
   will consume its parameters.
2. Invoke `microcontroller:firmware-module` for the C++ Module subclass mechanics (template parameter conventions,
   `CustomRuntimeParameters` struct, `kCustomStatusCodes` / `kModuleCommands` enums, stage-based command execution,
   `SendData` patterns).
3. Allocate a new module type code in the 1-255 range (current highest is 7); confirm the new code does not clash
   with any existing module type on the same controller.
4. Add the module's `#include` and instantiation block to the appropriate target in `src/main.cpp`. Add the new
   instance to the `Module* modules[]` array for that target.
5. Add the new header to `Doxyfile`'s `INPUT` list and to `docs/source/api.rst` for documentation coverage.
6. Bump the slmc version, build, and flash to the affected Teensy.
7. Hand off to sollertia-experiment for both the host-PC `ModuleInterface` implementation / binding-class
   integration and the `MesoscopeMicroControllers` calibration field addition (the dataclass lives in
   `sollertia-experiment/src/sl_experiment/mesoscope_vr/configuration.py`). Bump the sollertia-experiment version so
   older deployments refuse to load against the new schema.

**Modifying an existing module's parameter structure or status codes:**

1. Read the relevant header in `src/` to understand the current `CustomRuntimeParameters` and `kCustomStatusCodes`.
2. Verify the corresponding host-PC `ModuleInterface` and `MesoscopeMicroControllers` calibration fields in
   sollertia-experiment before making changes. Cross-repository drift is a runtime hazard.
3. Make the firmware change, bump the slmc version, and coordinate companion changes in both Sollertia repositories.
4. Re-flash all affected Teensy boards (a parameter-struct change typically affects only one of ACTOR / SENSOR /
   ENCODER, but a status-code change may ripple across PC-side log processing).

**Modifying controller IDs, keepalive interval, or per-target module layout:**

1. These are top-level cross-repository contracts. Coordinate with sollertia-experiment maintainers before changing.
2. Update `main.cpp` (controller IDs, keepalive interval, module instantiation order) and propagate the changes to
   the matching constants in sollertia-experiment's `MicroControllerInterfaces` binding class.
3. Update the README's "Per-Target Configuration" section to reflect the new values.

**Modifying build configuration, documentation, or style:**

1. C++ style changes: invoke `/cpp-style` and run `pio check` to catch shadowing, unused variables, etc.
2. README / Sphinx docs changes: invoke `/readme-style` or `/api-docs` as appropriate. `tox -e docs` must succeed.
3. tox.ini changes: invoke `/tox-config`.
4. Build flag, clang-format, or clang-tidy changes: verify against the canonical assets shipped with `/cpp-style`.

**Important considerations:**

- Module type codes are `uint8_t`; the `(type, id)` pair must be unique across all modules on a single controller.
- Controller IDs are `uint8_t`; slmc uses 101 / 152 / 203. The firmware `Kernel` requires uniqueness across
  concurrently-connected microcontrollers but does not document a reserved value at the source layer. Cross-reference
  the PC-side `ataraxis-communication-interface` conventions before reusing any low values, and coordinate any new
  ID choice with sollertia-experiment.
- Pin selection must avoid `LED_BUILTIN`; the per-module `static_assert` enforces this at compile time.
- Calibration commands (`ValveModule::Calibrate`, `EncoderModule::GetPPR`) intentionally block the runtime; they
  must never run during an active acquisition session. Their Doxygen `@warning` blocks document this.
