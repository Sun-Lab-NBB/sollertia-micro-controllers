/**
 * @file
 *
 * @brief Provides the TTLModule class that supports Transistor-to-Transistor Logic (TTL) communication with other
 * hardware systems by sending or receiving TTL logic pulses.
 */

#ifndef AXMC_TTL_MODULE_H
#define AXMC_TTL_MODULE_H

#include <Arduino.h>
#include <digitalWriteFast.h>
#include <module.h>

/**
 * @brief Sends or receives Transistor-to-Transistor Logic (TTL) signals using the specified digital pin.
 *
 * @warning Each class instance can either receive or output TTL signals, it cannot do both at the same time!
 *
 * @tparam kPin the digital pin used to output or receive TTL signals.
 * @tparam kOutput determines whether the instance is used to send TTL signals (if true) or receive TTL signals
 * (if false).
 * @tparam kStartOn determines the initial state of the output pin when the class is configured to output TTL signals.
 */
template <const uint8_t kPin, const bool kOutput = true, const bool kStartOn = false>
class TTLModule final : public Module
{
        static_assert(
            kPin != LED_BUILTIN,
            "The LED-connected pin is reserved for LED manipulation. Select a different pin for the TTLModule "
            "instance."
        );

    public:
        /// Defines the codes used by each module instance to communicate its runtime state to the PC.
        enum class kCustomStatusCodes : uint8_t
        {
            kInputOn        = 51,  ///< The input TTL pin is receiving a HIGH signal.
            kInputOff       = 52,  ///< The input TTL pin is receiving a LOW signal.
            kInvalidPinMode = 53,  ///< The instance's pin mode is not valid for the requested command.
            kOutputOn       = 54,  ///< The output TTL pin is set to HIGH.
            kOutputOff      = 55,  ///< The output TTL pin is set to LOW.
        };

        /// Defines the codes for the commands supported by the module's instance.
        enum class kModuleCommands : uint8_t
        {
            kSendPulse  = 1,  ///< Sends a TTL pulse through the output pin.
            kToggleOn   = 2,  ///< Sets the output pin to HIGH.
            kToggleOff  = 3,  ///< Sets the output pin to LOW.
            kCheckState = 4,  ///< Checks the state of the input pin.
        };

        /// Initializes the base Module class.
        TTLModule(const uint8_t module_type, const uint8_t module_id, Communication& communication) :
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
                // Emits a single TTL pulse through the output pin.
                case kModuleCommands::kSendPulse: SendPulse(); return true;
                // Holds the output pin HIGH.
                case kModuleCommands::kToggleOn: ToggleOn(); return true;
                // Holds the output pin LOW.
                case kModuleCommands::kToggleOff: ToggleOff(); return true;
                // Reports the input-pin state when it changes.
                case kModuleCommands::kCheckState: CheckState(); return true;
                // Unrecognized command.
                default: return false;
            }
        }

        /// Sets the module instance's software and hardware parameters to the default values.
        bool SetupModule() override
        {
            if (kOutput)
            {
                pinModeFast(kPin, OUTPUT);

                if (!kStartOn)
                {
                    digitalWriteFast(kPin, LOW);
                    SendData(static_cast<uint8_t>(kCustomStatusCodes::kOutputOff));
                }
                else
                {
                    digitalWriteFast(kPin, HIGH);
                    SendData(static_cast<uint8_t>(kCustomStatusCodes::kOutputOn));
                }
            }
            else
            {
                pinModeFast(kPin, INPUT);

                // Notifies the PC about the initial sensor state, which is always reported as a zero-value baseline.
                SendData(static_cast<uint8_t>(kCustomStatusCodes::kInputOff));
            }

            _custom_parameters.pulse_duration    = 10000;  // 10000 microseconds == 10 milliseconds.
            _custom_parameters.average_pool_size = 0;      // 0 or 1 disables averaging.

            return true;
        }

        ~TTLModule() override = default;

    private:
        /// Stores the instance's addressable runtime parameters.
        struct CustomRuntimeParameters
        {
                uint32_t pulse_duration   = 10000;  ///< The time, in microseconds, the pin outputs HIGH during pulses.
                uint8_t average_pool_size = 0;  ///< The number of digital readouts to average when checking pin state.
        } PACKED_STRUCT _custom_parameters;

        /// Sets the output pin to HIGH for the requested pulse_duration of microseconds and then sets it LOW.
        void SendPulse()
        {
            // Receive-only instance: report invalid pin mode and abort.
            if (!kOutput)
            {
                SendData(static_cast<uint8_t>(kCustomStatusCodes::kInvalidPinMode));
                AbortCommand();
                return;
            }

            switch (get_command_stage())
            {
                // Initializes the pulse.
                case 1:
                    digitalWriteFast(kPin, HIGH);
                    SendData(static_cast<uint8_t>(kCustomStatusCodes::kOutputOn));
                    AdvanceCommandStage();
                    return;

                // Delays for the requested number of microseconds.
                case 2:
                    if (!WaitForMicros(_custom_parameters.pulse_duration)) return;
                    AdvanceCommandStage();
                    return;

                // Ends the pulse.
                case 3:
                    digitalWriteFast(kPin, LOW);
                    SendData(static_cast<uint8_t>(kCustomStatusCodes::kOutputOff));
                    CompleteCommand();
                    return;

                default: AbortCommand();
            }
        }

        /// Sets the output pin to emit the HIGH signal.
        void ToggleOn()
        {
            // Receive-only instance: report invalid pin mode and abort.
            if (!kOutput)
            {
                SendData(static_cast<uint8_t>(kCustomStatusCodes::kInvalidPinMode));
                AbortCommand();
                return;
            }

            digitalWriteFast(kPin, HIGH);
            SendData(static_cast<uint8_t>(kCustomStatusCodes::kOutputOn));
            CompleteCommand();
        }

        /// Sets the output pin to emit the LOW signal.
        void ToggleOff()
        {
            // Receive-only instance: report invalid pin mode and abort.
            if (!kOutput)
            {
                SendData(static_cast<uint8_t>(kCustomStatusCodes::kInvalidPinMode));
                AbortCommand();
                return;
            }

            digitalWriteFast(kPin, LOW);
            SendData(static_cast<uint8_t>(kCustomStatusCodes::kOutputOff));
            CompleteCommand();
        }

        /// Checks the state of the input pin and sends it to the PC if it is significantly different from the
        /// previous readout.
        void CheckState()
        {
            static bool previous_input_status = false;

            // Output-mode instance: report invalid pin mode and abort future executions.
            if (kOutput)
            {
                SendData(static_cast<uint8_t>(kCustomStatusCodes::kInvalidPinMode));
                AbortCommand();
                return;
            }

            // Only emits when the state has changed since the previous evaluation, to reduce traffic to the PC.
            if (const bool current_state = DigitalRead(kPin, _custom_parameters.average_pool_size);
                previous_input_status != current_state)
            {
                previous_input_status = current_state;

                if (current_state) SendData(static_cast<uint8_t>(kCustomStatusCodes::kInputOn));
                else SendData(static_cast<uint8_t>(kCustomStatusCodes::kInputOff));
            }

            CompleteCommand();
        }
};

#endif  //AXMC_TTL_MODULE_H
