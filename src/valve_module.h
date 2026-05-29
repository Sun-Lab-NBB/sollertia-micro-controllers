/**
 * @file
 *
 * @brief Provides the ValveModule class that controls a solenoid valve and, optionally, a piezoelectric tone buzzer.
 */

#ifndef AXMC_VALVE_MODULE_H
#define AXMC_VALVE_MODULE_H

#include <Arduino.h>
#include <digitalWriteFast.h>
#include <module.h>

/// The kTonePin template parameter value used to indicate that no piezoelectric tone buzzer is connected.
static constexpr uint8_t kUnusedTonePin = 255;

/**
 * @brief Dispenses precise volumes of fluid or gas by sending digital currents through a solenoid valve and optionally
 * emits tones by sending digital currents through a piezoelectric buzzer.
 *
 * @tparam kValvePin the digital pin connected to the logic terminal of the managed solenoid valve's FET-gated power
 * relay.
 * @tparam kNormallyClosed determines whether the managed solenoid valve is opened (allows the fluid or gas to flow) or
 * closed (prevents the fluid or gas from flowing) when unpowered.
 * @tparam kStartClosed determines the initial state of the managed solenoid valve during class initialization.
 * @tparam kTonePin the digital pin connected to the logic terminal of the managed piezoelectric buzzer's FET-gated
 * power relay.
 * @tparam kNormallyOff determines whether the FET relay used to control the piezoelectric buzzer is closed
 * (on / conducting) or opened (off / not conducting) when unpowered.
 * @tparam kStartOff determines the initial state of the managed piezoelectric buzzer during class initialization.
 */
template <
    const uint8_t kValvePin,
    const bool kNormallyClosed,
    const bool kStartClosed = true,
    const uint8_t kTonePin  = kUnusedTonePin,
    const bool kNormallyOff = true,
    const bool kStartOff    = true>
class ValveModule final : public Module
{
        static_assert(
            kValvePin != LED_BUILTIN,
            "The LED-connected pin is reserved for LED manipulation. Select a different valve pin for the ValveModule "
            "instance."
        );
        static_assert(
            kTonePin != LED_BUILTIN,
            "The LED-connected pin is reserved for LED manipulation. Select a different tone pin for the ValveModule "
            "instance."
        );

    public:
        /// Defines the codes used by each module instance to communicate its runtime state to the PC.
        enum class kCustomStatusCodes : uint8_t
        {
            kOpen                     = 51,  ///< The valve is open.
            kClosed                   = 52,  ///< The valve is closed.
            kCalibrated               = 53,  ///< The valve has completed a calibration cycle.
            kToneOn                   = 54,  ///< The tone is played.
            kToneOff                  = 55,  ///< The tone is silenced.
            kInvalidToneConfiguration = 56,  ///< The instance is not configured to emit audible tones.
        };

        /// Defines the codes for the commands supported by the module's instance.
        enum class kModuleCommands : uint8_t
        {
            kSendPulse = 1,  ///< Opens the valve for the requested period of time and then closes it.
            kToggleOn  = 2,  ///< Opens the managed valve.
            kToggleOff = 3,  ///< Closes the managed valve.
            kCalibrate = 4,  ///< Repeatedly opens the valve for the requested period of time.
            kTonePulse = 5,  ///< Activates the managed buzzer to play the tone for the requested period of time.
        };

        /// Initializes the base Module class.
        ValveModule(const uint8_t module_type, const uint8_t module_id, Communication& communication) :
            Module(module_type, module_id, communication)
        {}

        /// Overwrites the module's runtime parameters structure with the data received from the PC.
        bool SetCustomParameters() override
        {
            if (ExtractParameters(_custom_parameters))
            {
                // When the tone buzzer is unused, force the tone duration to 0 so downstream stage logic can branch on
                // a single field instead of also checking kTonePin.
                if (kTonePin == kUnusedTonePin) _custom_parameters.tone_duration = 0;

                // _tone_time_delta captures the extra time the tone must outlast the valve pulse. Setting it to 0 skips
                // the extended-tone stage of Pulse() entirely.
                if (_custom_parameters.tone_duration > _custom_parameters.pulse_duration)
                    _tone_time_delta = _custom_parameters.tone_duration - _custom_parameters.pulse_duration;
                else _tone_time_delta = 0;
                return true;
            }
            return false;
        }

        /// Resolves and executes the currently active command.
        bool RunActiveCommand() override
        {
            switch (static_cast<kModuleCommands>(get_active_command()))
            {
                // Opens the valve and re-closes it after the configured pulse duration.
                case kModuleCommands::kSendPulse: Pulse(); return true;
                // Holds the valve open.
                case kModuleCommands::kToggleOn: Open(); return true;
                // Closes the valve.
                case kModuleCommands::kToggleOff: Close(); return true;
                // Runs a blocking calibration burst of `calibration_count` valve pulses.
                case kModuleCommands::kCalibrate: Calibrate(); return true;
                // Activates the buzzer for the configured tone duration.
                case kModuleCommands::kTonePulse: Tone(); return true;
                // Unrecognized command.
                default: return false;
            }
        }

        /// Sets the module instance's software and hardware parameters to the default values.
        bool SetupModule() override
        {
            if (kTonePin != kUnusedTonePin)
            {
                pinModeFast(kTonePin, OUTPUT);
                if (kStartOff)
                {
                    digitalWriteFast(kTonePin, kInactivate);
                    SendData(static_cast<uint8_t>(kCustomStatusCodes::kToneOff));
                }
                else
                {
                    digitalWriteFast(kTonePin, kActivate);
                    SendData(static_cast<uint8_t>(kCustomStatusCodes::kToneOn));
                }
            }
            else
            {
                // No tone pin configured: report tone-off so the PC sees a consistent initial state.
                SendData(static_cast<uint8_t>(kCustomStatusCodes::kToneOff));
            }

            pinModeFast(kValvePin, OUTPUT);
            if (kStartClosed)
            {
                digitalWriteFast(kValvePin, kClose);
                SendData(static_cast<uint8_t>(kCustomStatusCodes::kClosed));
            }
            else
            {
                digitalWriteFast(kValvePin, kOpen);
                SendData(static_cast<uint8_t>(kCustomStatusCodes::kOpen));
            }

            _custom_parameters.pulse_duration    = 35000;  // 35000 microseconds == ~5.0 uL in the reference rig.
            _custom_parameters.calibration_count = 500;    // 500 pulses per calibration burst.

            // Tone duration is only meaningful when the tone pin is configured.
            // 300000 microseconds == 300 milliseconds.
            if (kTonePin != kUnusedTonePin) _custom_parameters.tone_duration = 300000;
            else _custom_parameters.tone_duration = 0;

            return true;
        }

        ~ValveModule() override = default;

    private:
        /// Stores the instance's addressable runtime parameters.
        struct CustomRuntimeParameters
        {
                uint32_t pulse_duration    = 35000;   ///< The time, in microseconds, to keep the valve open.
                uint16_t calibration_count = 500;     ///< The number of times to pulse the valve during calibration.
                uint32_t tone_duration     = 300000;  ///< The time, in microseconds, to keep playing the tone.
        } PACKED_STRUCT _custom_parameters;

        /// Stores the digital signal that needs to be sent to the valve pin to open the valve.
        static constexpr bool kOpen = kNormallyClosed ? HIGH : LOW;  // NOLINT(*-dynamic-static-initializers)

        /// Stores the digital signal that needs to be sent to the valve pin to close the valve.
        static constexpr bool kClose = kNormallyClosed ? LOW : HIGH;  // NOLINT(*-dynamic-static-initializers)

        /// Stores the digital signal that needs to be sent to the tone pin to activate the tone buzzer.
        static constexpr bool kActivate = kNormallyOff ? HIGH : LOW;  // NOLINT(*-dynamic-static-initializers)

        /// Stores the digital signal that needs to be sent to the tone pin to deactivate the tone buzzer.
        static constexpr bool kInactivate = kNormallyOff ? LOW : HIGH;  // NOLINT(*-dynamic-static-initializers)

        /// Stores the time, in microseconds, that must separate any two consecutive pulses during the valve
        /// calibration. The value for this attribute is hardcoded for the system's safety, as pulsing the
        /// valve too fast may generate undue stress in the calibrated hydraulic system.
        static constexpr uint32_t kCalibrationDelay = 300000;

        /// Stores the difference, in microseconds, between the valve's pulse duration and the buzzer's tone duration,
        /// if both are used during valve pulsing.
        uint32_t _tone_time_delta = 0;

        /// Opens the valve to deliver a precise volume of fluid or gas and then closes it.
        void Pulse()
        {
            switch (get_command_stage())
            {
                // Opens the valve and (optionally) starts the tone buzzer.
                case 1:
                    digitalWriteFast(kValvePin, kOpen);
                    SendData(static_cast<uint8_t>(kCustomStatusCodes::kOpen));

                    if (_custom_parameters.tone_duration != 0)
                    {
                        digitalWriteFast(kTonePin, kActivate);
                        SendData(static_cast<uint8_t>(kCustomStatusCodes::kToneOn));
                    }

                    AdvanceCommandStage();
                    return;

                // Waits for the requested valve pulse duration of microseconds to pass.
                case 2:
                    if (!WaitForMicros(_custom_parameters.pulse_duration)) return;
                    AdvanceCommandStage();
                    return;

                // Closes the valve and either completes or advances to the extended-tone stage.
                case 3:
                    digitalWriteFast(kValvePin, kClose);
                    SendData(static_cast<uint8_t>(kCustomStatusCodes::kClosed));

                    if (_tone_time_delta == 0) CompleteCommand();
                    else AdvanceCommandStage();
                    return;

                // Optional stage: Waits for the remaining tone duration of microseconds to pass.
                case 4:
                    if (!WaitForMicros(_tone_time_delta)) return;

                    digitalWriteFast(kTonePin, kInactivate);
                    SendData(static_cast<uint8_t>(kCustomStatusCodes::kToneOff));
                    CompleteCommand();
                    return;

                default: AbortCommand();
            }
        }

        /// Opens the valve.
        void Open()
        {
            digitalWriteFast(kValvePin, kOpen);
            SendData(static_cast<uint8_t>(kCustomStatusCodes::kOpen));
            CompleteCommand();
        }

        /// Closes the valve.
        void Close()
        {
            digitalWriteFast(kValvePin, kClose);
            SendData(static_cast<uint8_t>(kCustomStatusCodes::kClosed));
            CompleteCommand();
        }

        /**
         * @brief Delivers `calibration_count` consecutive valve pulses of `pulse_duration` microseconds each,
         * separated by the hardcoded calibration delay.
         *
         * @warning Blocks the runtime in-place via delayMicroseconds() for the duration of the calibration burst.
         * Intended only for offline calibration, never during an active behavior session.
         */
        void Calibrate()
        {
            for (uint16_t pulse_index = 0; pulse_index < _custom_parameters.calibration_count; ++pulse_index)
            {
                digitalWriteFast(kValvePin, kOpen);

                delayMicroseconds(_custom_parameters.pulse_duration);

                digitalWriteFast(kValvePin, kClose);

                // Settles the valve before the next pulse to avoid undue hydraulic stress.
                delayMicroseconds(kCalibrationDelay);
            }

            SendData(static_cast<uint8_t>(kCustomStatusCodes::kCalibrated));
            CompleteCommand();
        }

        /// Activates the tone buzzer to deliver an audible tone for the specified duration of time and then
        /// deactivates it.
        void Tone()
        {
            // tone_duration == 0 covers both an unconfigured tone pin and an explicit zero-duration request.
            if (_custom_parameters.tone_duration == 0)
            {
                SendData(static_cast<uint8_t>(kCustomStatusCodes::kInvalidToneConfiguration));
                AbortCommand();
                return;
            }

            switch (get_command_stage())
            {
                // Activates the tone buzzer.
                case 1:
                    digitalWriteFast(kTonePin, kActivate);
                    SendData(static_cast<uint8_t>(kCustomStatusCodes::kToneOn));
                    AdvanceCommandStage();
                    return;

                // Waits for the tone duration to pass.
                case 2:
                    if (!WaitForMicros(_custom_parameters.tone_duration)) return;
                    AdvanceCommandStage();
                    return;

                // Deactivates the tone buzzer.
                case 3:
                    digitalWriteFast(kTonePin, kInactivate);
                    SendData(static_cast<uint8_t>(kCustomStatusCodes::kToneOff));
                    CompleteCommand();
                    return;

                default: AbortCommand();
            }
        }
};

#endif  //AXMC_VALVE_MODULE_H
