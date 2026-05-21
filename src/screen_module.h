/**
 * @file
 *
 * @brief Provides the ScreenModule class that controls the screen power boards used in the Sollertia platform's
 * Virtual Reality system.
 */

#ifndef AXMC_SCREEN_MODULE_H
#define AXMC_SCREEN_MODULE_H

#include <Arduino.h>
#include <digitalWriteFast.h>
#include <module.h>

/**
 * @brief Switches the screen power state by sending digital currents to the FET gate that shorts the power board's
 * button terminals.
 *
 * @tparam kPin the digital pin connected to the logic terminals of the VR screen's power board FET gates.
 * @tparam kNormallyClosed determines whether the FET relays used to control the screens' power are closed
 * (On / conducting) or opened (Off / not conducting) when unpowered.
 */
template <const uint8_t kPin, const bool kNormallyClosed = false>
class ScreenModule final : public Module
{
        static_assert(
            kPin != LED_BUILTIN,
            "The LED-connected pin is reserved for LED manipulation. Select a different pin for the ScreenModule "
            "instance."
        );

    public:
        /// Defines the codes used by each module instance to communicate its runtime state to the PC.
        enum class kCustomStatusCodes : uint8_t
        {
            kOn  = 51,  ///< The screen power board relay is receiving a HIGH signal.
            kOff = 52,  ///< The screen power board relay is receiving a LOW signal.
        };

        /// Defines the codes for the commands supported by the module's instance.
        enum class kModuleCommands : uint8_t
        {
            kToggle = 1,  ///< Pulses the screens' power board FET gates to simulate pressing the screens' power button.
        };

        /// Initializes the base Module class.
        ScreenModule(const uint8_t module_type, const uint8_t module_id, Communication& communication) :
            Module(module_type, module_id, communication)
        {}

        /// Overwrites the module's runtime parameters structure with the data received from the PC.
        bool SetCustomParameters() override
        {
            return ExtractParameters(_custom_parameters);
        }

        /// Resolves and executes the currently active command.
        bool RunActiveCommand() override
        {
            switch (static_cast<kModuleCommands>(get_active_command()))
            {
                // Simulates a button press on the screens' power board.
                case kModuleCommands::kToggle: Toggle(); return true;
                // Unrecognized command.
                default: return false;
            }
        }

        /// Sets the module instance's software and hardware parameters to the default values.
        bool SetupModule() override
        {
            pinModeFast(kPin, OUTPUT);

            digitalWriteFast(kPin, kOff);

            // Notifies the PC about the initial state of the FET gates.
            SendData(static_cast<uint8_t>(kCustomStatusCodes::kOff));

            _custom_parameters.pulse_duration = 1000000;  // 1000000 microseconds == 1 second.

            return true;
        }

        ~ScreenModule() override = default;

    private:
        /// Stores the instance's addressable runtime parameters.
        struct CustomRuntimeParameters
        {
                uint32_t pulse_duration = 1000000;  ///< The time of the toggling pulse's HIGH phase, in microseconds.
        } PACKED_STRUCT _custom_parameters;

        /// Stores the digital signal that needs to be sent to the output pin to simulate pressing the screens' power
        /// button.
        static constexpr bool kOn = kNormallyClosed ? LOW : HIGH;  // NOLINT(*-dynamic-static-initializers)

        /// Stores the digital signal that needs to be sent to the output pin to simulate releasing the screens' power
        /// button.
        static constexpr bool kOff = kNormallyClosed ? HIGH : LOW;  // NOLINT(*-dynamic-static-initializers)

        /// Sends a digital pulse through all output pins, using the preconfigured pulse_duration of microseconds, to
        /// simulate pressing and releasing the screens' power button.
        void Toggle()
        {
            switch (get_command_stage())
            {
                // Simulates pressing the screens' power button.
                case 1:
                    digitalWriteFast(kPin, kOn);
                    SendData(static_cast<uint8_t>(kCustomStatusCodes::kOn));
                    AdvanceCommandStage();
                    return;

                // Simulates holding the button down so the power board's debounce logic can detect the press.
                case 2:
                    if (!WaitForMicros(_custom_parameters.pulse_duration)) return;
                    AdvanceCommandStage();
                    return;

                // Simulates releasing the screens' power button.
                case 3:
                    digitalWriteFast(kPin, kOff);
                    SendData(static_cast<uint8_t>(kCustomStatusCodes::kOff));
                    CompleteCommand();
                    return;

                default: AbortCommand();
            }
        }
};

#endif  //AXMC_SCREEN_MODULE_H
