/**
 * @file
 *
 * @brief Provides the TorqueModule class that monitors and records the data produced by a reaction torque sensor.
 */

#ifndef AXMC_TORQUE_MODULE_H
#define AXMC_TORQUE_MODULE_H

#include <Arduino.h>
#include <digitalWriteFast.h>
#include <module.h>

/**
 * @brief Monitors the amplified voltage signal sent through an AD620 instrumentation amplifier to detect the direction
 * and the magnitude of the rotational force measured by the managed torque sensor.
 *
 * @note By default, the sensor interprets torque in the counterclockwise (CCW) direction as positive and torque in the
 * clockwise (CW) direction as negative.
 *
 * @tparam kPin the analog pin connected to the signal terminal of the AD620 instrumentation amplifier.
 * @tparam kBaseline the value, in 12-bit Analog-to-Digital Converter (ADC) units, of the received signal interpreted
 * as 0 torque.
 * @tparam kInvertDirection determines whether to invert the sign of the value returned by the sensor.
 */
template <const uint8_t kPin, const uint16_t kBaseline, const bool kInvertDirection = false>
class TorqueModule final : public Module
{
        static_assert(
            kPin != LED_BUILTIN,
            "The LED-connected pin is reserved for LED manipulation. Select a different pin for the TorqueModule "
            "instance."
        );

    public:
        /// Defines the codes used by each module instance to communicate its runtime state to the PC.
        enum class kCustomStatusCodes : uint8_t
        {
            kCCWTorque = 51,  ///< The sensor detects torque in the counterclockwise (CCW) direction.
            kCWTorque  = 52,  ///< The sensor detects torque in the clockwise (CW) direction.
        };

        /// Defines the codes for the commands supported by the module's instance.
        enum class kModuleCommands : uint8_t
        {
            kCheckState = 1,  ///< Determines the direction and magnitude of the detected torque since the last check.
        };

        /// Initializes the base Module class.
        TorqueModule(const uint8_t module_type, const uint8_t module_id, Communication& communication) :
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
                // Reports torque magnitude and direction since the previous check.
                case kModuleCommands::kCheckState: CheckState(); return true;
                // Unrecognized command.
                default: return false;
            }
        }

        /// Sets the module instance's software and hardware parameters to the default values.
        bool SetupModule() override
        {
            pinModeFast(kPin, INPUT);

            _custom_parameters.report_ccw        = true;
            _custom_parameters.report_cw         = true;
            _custom_parameters.signal_threshold  = 100;
            _custom_parameters.delta_threshold   = 70;
            _custom_parameters.average_pool_size = 5;

            // Notifies the PC about the initial sensor state. Direction is arbitrary for the zero-value baseline.
            SendData(
                static_cast<uint8_t>(kCustomStatusCodes::kCCWTorque),
                static_cast<uint16_t>(0)
            );

            return true;
        }

        ~TorqueModule() override = default;

    private:
        /// Stores the instance's addressable runtime parameters.
        struct CustomRuntimeParameters
        {
                bool report_ccw           = true;  ///< Determines whether to report changes in the CCW direction.
                bool report_cw            = true;  ///< Determines whether to report changes in the CW direction.
                uint16_t signal_threshold = 100;   ///< The minimum rescaled torque magnitude reported to the PC.
                uint16_t delta_threshold  = 70;    ///< The minimum signal difference to report torque changes.
                uint8_t average_pool_size = 5;     ///< The number of readouts to average when computing torque.
        } PACKED_STRUCT _custom_parameters;

        /// Reads the current direction and magnitude of the torque recorded by the sensor and sends it to the PC if it
        /// is significantly different from the previous readout.
        void CheckState()
        {
            static uint16_t previous_readout = kBaseline;  // NOLINT(*-dynamic-static-initializers)
            // A zero-baseline message is emitted during SetupModule(), so the initial previous_zero is true.
            static bool previous_zero        = true;

            uint16_t signal = AnalogRead(kPin, _custom_parameters.average_pool_size);

            const auto delta =
                static_cast<uint16_t>(abs(static_cast<int32_t>(signal) - static_cast<int32_t>(previous_readout)));

            // Suppresses readouts that are not significantly different from the previous value.
            if (delta <= _custom_parameters.delta_threshold)
            {
                CompleteCommand();
                return;
            }

            previous_readout = signal;

            // Rescales the signal so that 0 always means no torque and `kBaseline` always means maximum torque,
            // regardless of direction. Signals above baseline encode CCW, signals below baseline encode CW.
            bool is_clockwise = false;
            if (signal > kBaseline)
            {
                // CCW torque grows as the signal grows; rebase to [0, kBaseline].
                is_clockwise = false ^ kInvertDirection;
                signal       = signal - kBaseline;
            }
            else if (signal < kBaseline)
            {
                // CW torque grows as the signal shrinks; flip and rebase to [0, kBaseline] so larger values still mean
                // larger torque.
                is_clockwise = true ^ kInvertDirection;
                signal       = kBaseline - signal;
            }
            else
            {
                // Zero torque: direction is irrelevant. CCW chosen arbitrarily for reporting.
                is_clockwise = false;
                signal       = 0;
            }

            // Sub-threshold torque: emits a single zero-pull message if the previously reported value was not already
            // zero, to mark the end of an above-threshold event without spamming the PC. Direction is irrelevant.
            if (signal < _custom_parameters.signal_threshold)
            {
                if (!previous_zero)
                {
                    SendData(
                        static_cast<uint8_t>(kCustomStatusCodes::kCCWTorque),
                        static_cast<uint16_t>(0)
                    );
                    previous_zero = true;
                }
            }
            else
            {
                // Reports the absolute directional torque value in raw ADC units, using the event code to encode the
                // direction. Skips reporting if the class is configured to ignore changes in that direction.
                if (!is_clockwise && _custom_parameters.report_ccw)
                {
                    SendData(static_cast<uint8_t>(kCustomStatusCodes::kCCWTorque), signal);
                }
                else if (is_clockwise && _custom_parameters.report_cw)
                {
                    SendData(static_cast<uint8_t>(kCustomStatusCodes::kCWTorque), signal);
                }

                previous_zero = false;
            }

            CompleteCommand();
        }
};

#endif  //AXMC_TORQUE_MODULE_H
