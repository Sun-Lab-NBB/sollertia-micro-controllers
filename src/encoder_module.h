/**
 * @file
 *
 * @brief Provides the EncoderModule class that monitors and records the data produced by a quadrature encoder.
 *
 * @warning This file is written in a way that is @b NOT compatible with any other library or class that uses
 * AttachInterrupt(). Disable the 'ENCODER_USE_INTERRUPTS' macro defined at the top of the file to make this file
 * compatible with other interrupt libraries.
 */

#ifndef SLMC_ENCODER_MODULE_H
#define SLMC_ENCODER_MODULE_H

// This definition must precede Encoder.h inclusion. It increases the resolution of the encoder, but interferes with
// any other library that makes use of AttachInterrupt().
#define ENCODER_USE_INTERRUPTS

#include <Arduino.h>
#include <Encoder.h>
#include <module.h>

/**
 * @brief Wraps an Encoder class instance and provides access to its pulse counter to monitor the direction and
 * magnitude of the rotation measured by the managed quadrature encoder.
 *
 * @warning Both Pin A and Pin B must be hardware interrupt pins to achieve the maximum encoder state readout
 * resolution.
 *
 * @note By default, the clockwise (CW) movement of the encoder is measured as a negative displacement, while the
 * counterclockwise (CCW) movement of the encoder is measured as a positive displacement.
 *
 * @tparam kPinA the digital interrupt pin connected to the 'A' channel of the quadrature encoder.
 * @tparam kPinB the digital interrupt pin connected to the 'B' channel of the quadrature encoder.
 * @tparam kPinX the digital pin connected to the 'X' (index) channel of the quadrature encoder.
 * @tparam kInvertDirection determines whether to invert the sign of the value returned by the encoder.
 */
template <const uint8_t kPinA, const uint8_t kPinB, const uint8_t kPinX, const bool kInvertDirection = false>
class EncoderModule final : public Module
{
        static_assert(kPinA != kPinB, "EncoderModule PinA and PinB cannot be the same!");
        static_assert(kPinA != kPinX, "EncoderModule PinA and PinX cannot be the same!");
        static_assert(kPinB != kPinX, "EncoderModule PinB and PinX cannot be the same!");

        static_assert(
            kPinA != LED_BUILTIN,
            "The LED-connected pin is reserved for LED manipulation. Select a different Channel A pin for the "
            "EncoderModule instance."
        );
        static_assert(
            kPinB != LED_BUILTIN,
            "The LED-connected pin is reserved for LED manipulation. Select a different Channel B pin for the "
            "EncoderModule instance."
        );
        static_assert(
            kPinX != LED_BUILTIN,
            "The LED-connected pin is reserved for LED manipulation. Select a different Index (X) pin for the "
            "EncoderModule instance."
        );

    public:
        /// Defines the codes used by each module instance to communicate its runtime state to the PC.
        enum class kCustomStatusCodes : uint8_t
        {
            kRotatedCCW = 51,  ///< The encoder was rotated in the counterclockwise (CCW) direction.
            kRotatedCW  = 52,  ///< The encoder was rotated in the clockwise (CW) direction.
            kPPR        = 53,  ///< Communicates the estimated Pulse-Per-Revolution (PPR) value of the encoder.
        };

        /// Defines the codes for the commands supported by the module's instance.
        enum class kModuleCommands : uint8_t
        {
            kCheckState = 1,  ///< Determines the direction and magnitude of the encoder's motion since the last check.
            kReset      = 2,  ///< Resets the encoder's pulse tracker to 0.
            kGetPPR     = 3,  ///< Estimates the encoder's Pulse-Per-Revolution (PPR) value.
        };

        /// Initializes the base Module class.
        EncoderModule(const uint8_t module_type, const uint8_t module_id, Communication& communication) :
            Module(module_type, module_id, communication)
        {}

        /// Overwrites the module's runtime parameters structure with the data received from the PC.
        bool SetCustomParameters() override
        {
            if (ExtractParameters(_custom_parameters))
            {
                ResolveAmortizationCaps();
                return true;
            }
            return false;
        }

        /// Resolves and executes the currently active command.
        bool RunActiveCommand() override
        {
            switch (static_cast<kModuleCommands>(get_active_command()))
            {
                // Reports rotation accumulated since the previous check.
                case kModuleCommands::kCheckState: ReadEncoder(); return true;
                // Resets the pulse counter to 0.
                case kModuleCommands::kReset: ResetEncoder(); return true;
                // Estimates the encoder's pulses-per-revolution.
                case kModuleCommands::kGetPPR: GetPPR(); return true;
                // Unrecognized command.
                default: return false;
            }
        }

        /// Sets the module instance's software and hardware parameters to the default values.
        bool SetupModule() override
        {
            // The wrapped Encoder class does not manage the Index pin, so configure it explicitly as an input.
            pinMode(kPinX, INPUT);

            // The Encoder's quadrature pins are statically configured during Encoder construction.
            _encoder.write(0);

            _overflow = 0;

            _custom_parameters.report_ccw      = kDefaultReportCcw;
            _custom_parameters.report_cw       = kDefaultReportCw;
            _custom_parameters.delta_threshold = kDefaultDeltaThreshold;

            // Realigns the amortization caps with the threshold assigned above. The Kernel re-runs this method on
            // every controller reset and keepalive timeout, so the caps have to be restored alongside it.
            ResolveAmortizationCaps();

            // Notifies the PC about the initial sensor state. Direction is arbitrary for the zero-value baseline.
            SendData(static_cast<uint8_t>(kCustomStatusCodes::kRotatedCW), static_cast<uint32_t>(0));

            return true;
        }

        ~EncoderModule() override = default;

    private:
        /// Stores the instance's addressable runtime parameters.
        struct CustomRuntimeParameters
        {
                bool report_ccw          = kDefaultReportCcw;       ///< Determines whether to report CCW rotation.
                bool report_cw           = kDefaultReportCw;        ///< Determines whether to report CW rotation.
                uint32_t delta_threshold = kDefaultDeltaThreshold;  ///< The minimum displacement change reported.
        } PACKED_STRUCT _custom_parameters;

        /// Determines whether the module reports counterclockwise (CCW) rotation by default.
        static constexpr bool kDefaultReportCcw = true;

        /// Determines whether the module reports clockwise (CW) rotation by default.
        static constexpr bool kDefaultReportCw = false;

        /// Stores the default minimum displacement change (delta), in encoder pulses, for reporting rotation.
        static constexpr uint32_t kDefaultDeltaThreshold = 15;

        /// Stores the multiplier used to optionally invert the pulse counter sign to virtually flip the direction of
        /// encoder readouts.
        static constexpr int32_t kMultiplier = kInvertDirection ? -1 : 1;  // NOLINT(*-dynamic-static-initializers)

        /// Stores the number of full encoder rotations measured when estimating the Pulse-Per-Revolution (PPR) value.
        static constexpr uint8_t kPPRMeasuredRotations = 10;

        /// Stores the delay, in milliseconds, that lets the index-pin trigger window elapse between rotations.
        static constexpr uint16_t kIndexSettleDelay = 100;

        /// Wraps the Encoder instance that monitors the encoder's rotation. The member is initialized statically,
        /// because deferred initialization causes a runtime crash.
        Encoder _encoder = Encoder(kPinA, kPinB);

        /// Accumulates insignificant encoder readouts to be reused during future encoder state evaluation calls.
        int32_t _overflow = 0;

        /// Determines the maximum encoder displacement (rotation) in the counterclockwise (CCW) direction that can be
        /// accumulated in the _overflow attribute during runtime when reporting the CCW rotation is disabled.
        int32_t _positive_amortization = 0;

        /// Determines the maximum encoder displacement (rotation) in the clockwise (CW) direction that can be
        /// accumulated in the _overflow attribute during runtime when reporting the CW rotation is disabled.
        int32_t _negative_amortization = 0;

        /**
         * @brief Derives the positive and negative amortization caps from the instance's current delta threshold.
         *
         * Amortization permits the overflow accumulator to store pulses in the non-reported direction up to the
         * threshold, suppressing small jitter (for example, a locked running wheel) so that opposing micro-motions
         * cancel out instead of accumulating into a spurious directional event.
         */
        void ResolveAmortizationCaps()
        {
            _positive_amortization = static_cast<int32_t>(_custom_parameters.delta_threshold);
            _negative_amortization = -_positive_amortization;
        }

        /// Reads the direction and magnitude of the encoder's rotation since the previous check and sends it to the PC
        /// when the accumulated displacement exceeds the configured delta threshold.
        void ReadEncoder()
        {
            const int32_t new_motion = _encoder.readAndReset() * kMultiplier;

            if (new_motion == 0)
            {
                CompleteCommand();
                return;
            }

            _overflow += new_motion;

            if (new_motion < 0 && !_custom_parameters.report_cw)
            {
                // Caps the accumulated CW motion (negative) to the amortization threshold when CW reporting is off.
                _overflow = max(_overflow, _negative_amortization);
            }
            else if (new_motion > 0 && !_custom_parameters.report_ccw)
            {
                // Caps the accumulated CCW motion (positive) to the amortization threshold when CCW reporting is off.
                _overflow = min(_overflow, _positive_amortization);
            }

            const auto delta = static_cast<uint32_t>(abs(_overflow));

            // Negative overflow encodes CW displacement, and positive overflow encodes CCW. Only reports magnitudes
            // that exceed the configured delta threshold, then drains the accumulator.
            if (_overflow < 0 && delta > _custom_parameters.delta_threshold)
            {
                SendData(static_cast<uint8_t>(kCustomStatusCodes::kRotatedCW), delta);
                _overflow = 0;
            }
            else if (_overflow > 0 && delta > _custom_parameters.delta_threshold)
            {
                SendData(static_cast<uint8_t>(kCustomStatusCodes::kRotatedCCW), delta);
                _overflow = 0;
            }

            CompleteCommand();
        }

        /// Resets the encoder pulse counter back to 0.
        void ResetEncoder()
        {
            _encoder.write(0);
            CompleteCommand();
        }

        /**
         * @brief Estimates the Pulse-Per-Revolution (PPR) value of the encoder by using the index pin to precisely
         * measure the number of pulses emitted during a full (360-degree) encoder rotation.
         *
         * @warning Blocks the runtime in-place. The method spins waiting on the index pin and delays between rotations.
         * The index waits are intentionally unbounded, so the instance stays in this mode until the operator rotates
         * the encoder through every measured revolution. Intended only for offline calibration, never during an active
         * behavior session.
         */
        void GetPPR()
        {
            // Discards an index window inside which the encoder may already be parked, so the baseline below is
            // established on a genuine low-to-high index transition. A baseline taken part-way through a window makes
            // the first measured revolution short and deflates the average.
            while (digitalReadFast(kPinX))
            {
            }

            // Waits for the first index-pin trigger to establish the rotation baseline before counting begins.
            while (!digitalReadFast(kPinX))
            {
            }
            _encoder.write(0);

            // Measures 10 full rotations (indicated by index pin pulses). Resets the pulse tracker at each measurement
            // and ignores the rotation direction.
            uint32_t total_pulses = 0;
            for (uint8_t rotation_index = 0; rotation_index < kPPRMeasuredRotations; ++rotation_index)
            {
                // Delays long enough for the index-pin trigger window to elapse before the next rotation is measured.
                delay(kIndexSettleDelay);

                while (!digitalReadFast(kPinX))
                {
                }

                total_pulses += abs(_encoder.readAndReset());
            }

            // Half-up integer division: (sum + rotations / 2) / rotations.
            const uint32_t average_ppr = (total_pulses + kPPRMeasuredRotations / 2) / kPPRMeasuredRotations;

            SendData(static_cast<uint8_t>(kCustomStatusCodes::kPPR), average_ppr);

            CompleteCommand();
        }
};

#endif  // SLMC_ENCODER_MODULE_H
