/**
 * @file
 *
 * @brief Provides the BrakeModule class that controls an electromagnetic particle brake.
 */

#ifndef AXMC_BRAKE_MODULE_H
#define AXMC_BRAKE_MODULE_H

#include <Arduino.h>
#include <digitalWriteFast.h>
#include <module.h>

/**
 * @brief Controls the electromagnetic brake by sending digital or analog Pulse-Width-Modulated (PWM) currents through
 * the brake.
 *
 * @tparam kPin the digital output pin connected to the logic terminal of the managed brake's FET-gated power relay.
 * @tparam kNormallyEngaged determines whether the brake is engaged (active) or disengaged (inactive) when unpowered.
 * @tparam kStartEngaged determines the initial state of the brake during class initialization.
 */
template <const uint8_t kPin, const bool kNormallyEngaged, const bool kStartEngaged = true>
class BrakeModule final : public Module
{
        static_assert(
            kPin != LED_BUILTIN,
            "The LED-connected pin is reserved for LED manipulation. Select a different pin for the BrakeModule "
            "instance."
        );

    public:
        /// Defines the codes used by each module instance to communicate its runtime state to the PC.
        enum class kCustomStatusCodes : uint8_t
        {
            kEngaged    = 51,  ///< The brake is engaged at maximum possible strength.
            kDisengaged = 52,  ///< The brake is disengaged.
            kVariable   = 53,  ///< The brake is engaged at the specified non-maximal strength.
        };

        /// Defines the codes for the commands supported by the module's instance.
        enum class kModuleCommands : uint8_t
        {
            kToggleOn        = 1,  ///< Engages the brake at maximum strength.
            kToggleOff       = 2,  ///< Disengages the brake.
            kSetBrakingPower = 3,  ///< Sets the brake to engage at the requested braking strength.
            kSendPulse       = 4,  ///< Briefly engages the brake at maximum strength for the specified duration.
        };

        /// Initializes the base Module class.
        BrakeModule(const uint8_t module_type, const uint8_t module_id, Communication& communication) :
            Module(module_type, module_id, communication)
        {}

        /// Overwrites the module's runtime parameters structure with the data received from the PC.
        bool SetCustomParameters() override
        {
            if (ExtractParameters(_custom_parameters))
            {
                // Inverts the PWM value when the brake is normally engaged, so that strength 255 always means the brake
                // is fully engaged regardless of the relay's idle state.
                if (kNormallyEngaged)
                    _custom_parameters.braking_strength = 255 - _custom_parameters.braking_strength;
                return true;
            }
            return false;
        }

        /// Resolves and executes the currently active command.
        bool RunActiveCommand() override
        {
            switch (static_cast<kModuleCommands>(get_active_command()))
            {
                // Engages the brake at maximum strength.
                case kModuleCommands::kToggleOn: EnableBrake(); return true;
                // Disengages the brake.
                case kModuleCommands::kToggleOff: DisableBrake(); return true;
                // Engages the brake at the requested non-maximal strength via PWM.
                case kModuleCommands::kSetBrakingPower: SetBrakingPower(); return true;
                // Briefly engages the brake at maximum strength for the configured pulse duration.
                case kModuleCommands::kSendPulse: SendPulse(); return true;
                // Unrecognized command.
                default: return false;
            }
        }

        /// Sets the module instance's software and hardware parameters to the default values.
        bool SetupModule() override
        {
            pinModeFast(kPin, OUTPUT);

            // Drives the brake into the configured initial state, accounting for whether the relay is normally engaged.
            if (kStartEngaged)
            {
                digitalWriteFast(kPin, kEngage);
                SendData(static_cast<uint8_t>(kCustomStatusCodes::kEngaged));
            }
            else
            {
                digitalWriteFast(kPin, kDisengage);
                SendData(static_cast<uint8_t>(kCustomStatusCodes::kDisengaged));
            }

            _custom_parameters.braking_strength = 128;      // 50% braking strength.
            _custom_parameters.pulse_duration   = 1000000;  // 1000000 microseconds == 1 second.

            return true;
        }

        ~BrakeModule() override = default;

    private:
        /// Stores the instance's addressable runtime parameters.
        struct CustomRuntimeParameters
        {
                uint8_t braking_strength = 128;      ///< Determines the strength of the brake in variable mode.
                uint32_t pulse_duration  = 1000000;  ///< The time, in microseconds, to engage the brake during pulses.
        } PACKED_STRUCT _custom_parameters;

        /// Stores the digital signal that needs to be sent to the output pin to engage the brake at maximum strength.
        static constexpr bool kEngage = kNormallyEngaged ? LOW : HIGH;  // NOLINT(*-dynamic-static-initializers)

        /// Stores the digital signal that needs to be sent to the output pin to disengage the brake.
        static constexpr bool kDisengage = kNormallyEngaged ? HIGH : LOW;  // NOLINT(*-dynamic-static-initializers)

        /// Engages the brake at the maximum strength.
        void EnableBrake()
        {
            digitalWriteFast(kPin, kEngage);
            SendData(static_cast<uint8_t>(kCustomStatusCodes::kEngaged));
            CompleteCommand();
        }

        /// Disengages the brake.
        void DisableBrake()
        {
            digitalWriteFast(kPin, kDisengage);
            SendData(static_cast<uint8_t>(kCustomStatusCodes::kDisengaged));
            CompleteCommand();
        }

        /// Engages the brake at the specified strength level.
        void SetBrakingPower()
        {
            // Drives the pin with a PWM square wave so the brake is engaged for the configured duty-cycle fraction.
            analogWrite(kPin, _custom_parameters.braking_strength);
            SendData(static_cast<uint8_t>(kCustomStatusCodes::kVariable));
            CompleteCommand();
        }

        /// Engages the brake at maximum strength for the requested pulse_duration of microseconds and then
        /// disengages it.
        void SendPulse()
        {
            switch (get_command_stage())
            {
                // Engages the brake at maximum strength.
                case 1:
                    digitalWriteFast(kPin, kEngage);
                    SendData(static_cast<uint8_t>(kCustomStatusCodes::kEngaged));
                    AdvanceCommandStage();
                    return;

                // Delays for the requested number of microseconds.
                case 2:
                    if (!WaitForMicros(_custom_parameters.pulse_duration)) return;
                    AdvanceCommandStage();
                    return;

                // Disengages the brake.
                case 3:
                    digitalWriteFast(kPin, kDisengage);
                    SendData(static_cast<uint8_t>(kCustomStatusCodes::kDisengaged));
                    CompleteCommand();
                    return;

                default: AbortCommand();
            }
        }
};

#endif  //AXMC_BRAKE_MODULE_H
