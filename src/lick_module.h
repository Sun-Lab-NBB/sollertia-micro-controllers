/**
 * @file
 *
 * @brief Provides the LickModule class that monitors and records the data produced by a conductive lick sensor.
 */

#ifndef SLMC_LICK_MODULE_H
#define SLMC_LICK_MODULE_H

#include <Arduino.h>
#include <module.h>

/**
 * @brief Monitors the voltage fluctuations measured by a conductive lick sensor to detect interactions with the
 * sensor's circuitry.
 *
 * @tparam kPin the analog pin connected to the output terminal of the lick sensor.
 */
template <const uint8_t kPin>
class LickModule final : public Module
{
        static_assert(
            kPin != LED_BUILTIN,
            "The LED-connected pin is reserved for LED manipulation. Select a different pin for the "
            "LickModule instance."
        );

    public:
        /// Defines the codes used by each module instance to communicate its runtime state to the PC.
        enum class kCustomStatusCodes : uint8_t
        {
            kChanged = 51,  ///< The sensor has experienced a significant change in the conducted voltage level.
        };

        /// Defines the codes for the commands supported by the module's instance.
        enum class kModuleCommands : uint8_t
        {
            kCheckState = 1,  ///< Checks the voltage level across the sensor.
        };

        /// Initializes the base Module class.
        LickModule(const uint8_t module_type, const uint8_t module_id, Communication& communication) :
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
                // Reports lick-sensor voltage when it changes significantly.
                case kModuleCommands::kCheckState: CheckState(); return true;
                // Unrecognized command.
                default: return false;
            }
        }

        /// Sets the module instance's software and hardware parameters to the default values.
        bool SetupModule() override
        {
            // Pull-down mode: the sensor spends most of its runtime in an uncompleted-circuit state, so the pin must
            // be pulled to 0 at rest.
            pinMode(kPin, INPUT_PULLDOWN);

            // Assumes 12-bit ADC resolution.
            _custom_parameters.signal_threshold  = 300;  // Just above the typical noise floor.
            _custom_parameters.delta_threshold   = 300;  // At least half of the minimal signal_threshold.
            _custom_parameters.average_pool_size = 2;    // Averages two readouts to suppress single-sample ADC noise.

            // Realigns the change-detection state with the zero baseline reported below. The Kernel re-runs this
            // method on every controller reset and keepalive timeout, so the state has to be restored alongside it.
            _previous_readout = 0;
            _previous_zero    = true;

            // Notifies the PC about the initial sensor state.
            SendData(static_cast<uint8_t>(kCustomStatusCodes::kChanged), static_cast<uint16_t>(0));

            return true;
        }

        ~LickModule() override = default;

    private:
        /// Stores the instance's addressable runtime parameters.
        struct CustomRuntimeParameters
        {
                uint16_t signal_threshold = 300;  ///< The minimum voltage level to report to the PC.
                uint16_t delta_threshold  = 300;  ///< The minimum change in voltage level readouts to report to the PC.
                uint8_t average_pool_size = 2;    ///< The number of readouts to average to determine the voltage level.
        } PACKED_STRUCT _custom_parameters;

        /// Stores the voltage level readout that the next delta comparison measures against, updated only when a
        /// readout clears the delta threshold.
        uint16_t _previous_readout = 0;

        /// Determines whether the most recent readout reported to the PC was the zero-value baseline.
        bool _previous_zero = true;

        /// Checks the voltage level across the sensor's circuitry and sends it to the PC if it is significantly
        /// different from the previous readout.
        void CheckState()
        {
            const uint16_t signal = AnalogRead<kPin>(_custom_parameters.average_pool_size);

            const auto delta =
                static_cast<uint16_t>(abs(static_cast<int32_t>(signal) - static_cast<int32_t>(_previous_readout)));

            // Suppresses readouts that are not significantly different from the previous value.
            if (delta <= _custom_parameters.delta_threshold)
            {
                CompleteCommand();
                return;
            }

            _previous_readout = signal;

            if (signal >= _custom_parameters.signal_threshold)
            {
                SendData(static_cast<uint8_t>(kCustomStatusCodes::kChanged), signal);
                _previous_zero = false;
            }

            // Sub-threshold signal: emits a single zero-pull message if the previously reported value was not already
            // zero, to mark the end of an above-threshold event without spamming the PC.
            else if (!_previous_zero)
            {
                SendData(static_cast<uint8_t>(kCustomStatusCodes::kChanged), static_cast<uint16_t>(0));
                _previous_zero = true;
            }

            CompleteCommand();
        }
};

#endif  // SLMC_LICK_MODULE_H
