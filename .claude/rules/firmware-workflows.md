# Firmware workflows

**Development commands:**

```bash
pio run                              # Compile every target firmware (the build regression gate)
pio run -e teensy41_actor            # Compile a single target
pio run -e teensy41_actor -t upload  # Compile and flash one target to the connected board
pio check                            # Run clang-tidy static analysis across every target
tox -e docs                          # Build Sphinx + Doxygen API documentation
tox -e deploy                        # Upload the built documentation to the project's Netlify site
```

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
   current Mesoscope-VR deployment, this means choosing one of ACTOR / SENSOR / ENCODER, and for a different
   consumer, the choice depends on that system's target layout). Add the new instance to the
   `Module* modules[]` array for that target.
5. Add the new header to `Doxyfile`'s `INPUT` list and to `docs/source/api.rst` for documentation coverage.
6. Update `experiment:microcontroller-interface`'s `references/module-catalog.md` with the new entry.
7. Bump the slmc version. This project ships no `library.json`, so the two in-repository copies of the version are
   `PROJECT_NUMBER` in `Doxyfile` and `release` in `docs/source/conf.py`. Update both to match the git tag, because a
   one-sided bump leaves the Sphinx pages stamped with the previous release. The experimenter then flashes the
   affected board(s), because firmware uploads are not agent-driven.
8. Hand off to sollertia-experiment for the host-PC side. `experiment:library-extension` holds the seam view of this
   handoff, naming the sollertia-experiment mirror each firmware constant obliges. The handoff splits in two:
   - **Python wrapper** (system-agnostic): author the new `ModuleInterface` subclass in
     `sollertia-experiment/src/sollertia_experiment/cross_system/module_interfaces.py` following
     `experiment:microcontroller-interface`'s "sle Python wrapper conventions" section.
   - **Binding-class integration** (consumer-specific): for the current Mesoscope-VR consumer, hand off to
     `mesoscope:mesoscope-vr` to add calibration fields to `MesoscopeMicroControllers`, extend
     `MicroControllerInterfaces` to instantiate the new wrapper, and regenerate the system YAML. Bump the
     sollertia-experiment version so older deployments refuse to load against the new schema.

***Note,*** this workflow covers new hardware for an acquisition system that already consumes this firmware. When the
driver is a **new** acquisition system instead, start from `experiment:system-design-pipeline`, which orders the whole
cross-repository build and routes to `assets:library-extension` for the shared-assets registry half and to
`experiment:library-extension` for the sollertia-experiment and slmc seams. Reach this workflow only if the new system
needs a module the seven existing ones do not already cover.

**Modifying an existing module's parameter structure or status codes:**

1. Read the relevant header in `src/` to understand the current `CustomRuntimeParameters` and `kCustomStatusCodes`.
2. Verify the corresponding host-PC `ModuleInterface` (in
   `sollertia-experiment/src/sollertia_experiment/cross_system/module_interfaces.py`) and the consuming
   system's calibration fields (for Mesoscope-VR: `MesoscopeMicroControllers` in
   `sollertia-experiment/src/sollertia_experiment/mesoscope_vr/system.py`) before making changes.
   Cross-repository drift is a runtime hazard.
3. Make the firmware change, bump the slmc version, and coordinate companion changes via
   `experiment:microcontroller-interface` (wrapper-side) and the consumer's instance skill (binding-side, which for
   Mesoscope-VR is `mesoscope:mesoscope-vr`).
4. The experimenter re-flashes all affected boards (a parameter-struct change typically affects only the one target
   that hosts the module, but a status-code change may ripple across PC-side log processing).

**Modifying controller IDs, keepalive interval, or per-target module layout:**

1. Read `experiment:library-extension`'s controller-target and board-family seams before editing anything. They name
   the exact sollertia-experiment mirror for each of these constants, so the companion change is identified before the
   firmware change is made rather than after.
2. These are top-level cross-repository contracts. Coordinate with the consuming acquisition system's maintainers
   before changing. For the current Mesoscope-VR consumer, this means coordinating with
   `mesoscope:mesoscope-vr`'s maintenance contract.
3. Update `main.cpp` (controller IDs, keepalive interval, module instantiation order) and propagate the changes
   to the matching constants in the consumer's binding class (for Mesoscope-VR: `MicroControllerInterfaces` in
   `sollertia-experiment/src/sollertia_experiment/mesoscope_vr/binding_classes.py`).
4. Update the README's "Per-Target Configuration" section to reflect the new values.

**Modifying build configuration, documentation, or style:**

1. C++ style changes: invoke `automation:cpp-style` and run `pio check` to catch shadowing, unused variables, etc.
2. README / Sphinx docs changes: invoke `automation:readme-style` or `automation:api-docs` as appropriate. `tox -e docs`
   must succeed.
3. tox.ini changes: invoke `automation:tox-config`.
4. Build flag, clang-format, or clang-tidy changes: verify against the canonical assets shipped with
   `automation:cpp-style`.

**Important considerations:**

- Module type codes are `uint8_t`, and the `(type, id)` pair must be unique across all modules on a single controller.
- Controller IDs are `uint8_t`. The `Kernel` accepts values 1 through 255 and reserves 0, requiring each ID to be
  unique across concurrently-connected microcontrollers. The current Mesoscope-VR deployment uses 101 / 152 / 203.
  Cross-reference the PC-side `ataraxis-communication-interface` conventions before reusing any low values, and
  coordinate any new ID choice with the consuming acquisition system.
- Pin selection must avoid `LED_BUILTIN`, and the per-module `static_assert` enforces this at compile time.
- Calibration commands (`ValveModule::Calibrate`, `EncoderModule::GetPPR`) intentionally block the runtime, and they
  must never run during an active acquisition session. Their Doxygen `@warning` blocks document this.
