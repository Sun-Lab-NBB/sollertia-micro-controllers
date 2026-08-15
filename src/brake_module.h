/**
 * @file
 *
 * @brief Provides the BrakeModule class that controls an electromagnetic particle brake.
 */

#ifndef SLMC_BRAKE_MODULE_H
#define SLMC_BRAKE_MODULE_H

#include <Arduino.h>
#include <module.h>

/**
 * @brief Controls the electromagnetic brake by sending digital or analog Pulse-Width-Modulated (PWM) currents through
 * the brake.
 *
 * @note The default pulse duration is calibrated for non-blocking command execution. A blocking pulse command stalls
 * the controller for its full duration, which exceeds the keepalive interval the firmware declares and trips the
 * Kernel's emergency reset.
 *
 * @tparam kPin the digital output pin connected to the logic terminal of the managed brake's FET-gated power relay.
 * The pin has to support analogWrite(), as the module drives it with a PWM waveform for every braking strength
 * between the two extremes.
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
                if (kNormallyEngaged) _custom_parameters.braking_strength = 255 - _custom_parameters.braking_strength;
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
            pinMode(kPin, OUTPUT);

            // Realigns the pin-mode tracker with the GPIO mode the call above selects. The Kernel re-runs this method
            // on every controller reset and keepalive timeout, so the tracker has to be restored alongside it.
            _analog_mode = false;

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

            // Defaulting to full strength keeps the pin under GPIO control until the PC requests an intermediate
            // strength, which is the only case that needs the PWM peripheral.
            _custom_parameters.braking_strength = kFullEngageDuty;
            _custom_parameters.pulse_duration   = 1000000;  // 1000000 microseconds == 1 second.

            return true;
        }

        ~BrakeModule() override = default;

    private:
        /// Stores the instance's addressable runtime parameters.
        struct CustomRuntimeParameters
        {
                uint8_t braking_strength = kFullEngageDuty;  ///< Determines the strength of the brake in variable mode.
                uint32_t pulse_duration  = 1000000;  ///< The time, in microseconds, to engage the brake during pulses.
        } PACKED_STRUCT _custom_parameters;

        /// Stores the braking_strength value that engages the brake at maximum strength, expressed in the inverted
        /// frame SetCustomParameters() stores. Driving it as a digital level is electrically identical to driving it
        /// as a duty cycle, so the two extremes stay on the GPIO peripheral.
        static constexpr uint8_t kFullEngageDuty = kNormallyEngaged ? 0 : 255;  // NOLINT(*-dynamic-static-initializers)

        /// Stores the braking_strength value that disengages the brake, expressed in the same inverted frame.
        static constexpr uint8_t kFullDisengageDuty =
            kNormallyEngaged ? 255 : 0;  // NOLINT(*-dynamic-static-initializers)

        /// Stores the digital signal that needs to be sent to the output pin to engage the brake at maximum strength.
        static constexpr bool kEngage = kNormallyEngaged ? LOW : HIGH;  // NOLINT(*-dynamic-static-initializers)

        /// Stores the digital signal that needs to be sent to the output pin to disengage the brake.
        static constexpr bool kDisengage = kNormallyEngaged ? HIGH : LOW;  // NOLINT(*-dynamic-static-initializers)

        /// Determines whether the PWM peripheral currently drives the output pin instead of the GPIO peripheral.
        bool _analog_mode = false;

        /**
         * @brief Drives the output pin with the requested digital level, reclaiming GPIO control of the pin when the
         * PWM peripheral currently owns it.
         *
         * analogWrite() re-points the pin at the PWM peripheral, which leaves every later digitalWriteFast() writing
         * to a register the pin no longer reads. The level is written before the mode switch, as the two use separate
         * registers, so the pin never briefly emits the stale level its output register held.
         *
         * @param level the digital signal to send to the output pin.
         */
        void WriteDigital(const bool level)
        {
            digitalWriteFast(kPin, level);

            if (!_analog_mode) return;

            pinMode(kPin, OUTPUT);
            _analog_mode = false;
        }

        /// Engages the brake at the maximum strength.
        void EnableBrake()
        {
            WriteDigital(kEngage);
            SendData(static_cast<uint8_t>(kCustomStatusCodes::kEngaged));
            CompleteCommand();
        }

        /// Disengages the brake.
        void DisableBrake()
        {
            WriteDigital(kDisengage);
            SendData(static_cast<uint8_t>(kCustomStatusCodes::kDisengaged));
            CompleteCommand();
        }

        /**
         * @brief Engages the brake at the specified strength level.
         *
         * The two extremes are exactly representable as digital levels, so they take the GPIO path and leave the pin
         * under GPIO control. Only an intermediate strength hands the pin to the PWM peripheral, which keeps a system
         * that only ever toggles the brake fully on and off from ever leaving the digital mode.
         */
        void SetBrakingPower()
        {
            const uint8_t strength = _custom_parameters.braking_strength;

            if (strength == kFullEngageDuty)
            {
                WriteDigital(kEngage);
                SendData(static_cast<uint8_t>(kCustomStatusCodes::kEngaged));
            }
            else if (strength == kFullDisengageDuty)
            {
                WriteDigital(kDisengage);
                SendData(static_cast<uint8_t>(kCustomStatusCodes::kDisengaged));
            }
            else
            {
                // Drives the pin with a PWM square wave so the brake is engaged for the configured duty-cycle fraction.
                analogWrite(kPin, strength);
                _analog_mode = true;
                SendData(static_cast<uint8_t>(kCustomStatusCodes::kVariable));
            }

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
                    WriteDigital(kEngage);
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
                    WriteDigital(kDisengage);
                    SendData(static_cast<uint8_t>(kCustomStatusCodes::kDisengaged));
                    CompleteCommand();
                    return;

                default: AbortCommand();
            }
        }
};

#endif  // SLMC_BRAKE_MODULE_H
