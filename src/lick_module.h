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

            _custom_parameters.signal_threshold  = kDefaultSignalThreshold;
            _custom_parameters.delta_threshold   = kDefaultDeltaThreshold;
            _custom_parameters.average_pool_size = kDefaultAveragePoolSize;

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
                uint16_t signal_threshold = kDefaultSignalThreshold;  ///< The minimum voltage level reported.
                uint16_t delta_threshold  = kDefaultDeltaThreshold;   ///< The minimum readout change reported.
                uint8_t average_pool_size = kDefaultAveragePoolSize;  ///< The number of readouts averaged.
        } PACKED_STRUCT _custom_parameters;

        /// Stores the default minimum voltage level to report to the PC, in 12-bit ADC units. The value sits just
        /// above the typical noise floor.
        static constexpr uint16_t kDefaultSignalThreshold = 300;

        /// Stores the default minimum readout change to report to the PC, in 12-bit ADC units. The value is at least
        /// half of the minimal signal threshold.
        static constexpr uint16_t kDefaultDeltaThreshold = 300;

        /// Stores the default number of readouts to average, which suppresses single-sample ADC noise.
        static constexpr uint8_t kDefaultAveragePoolSize = 2;

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
