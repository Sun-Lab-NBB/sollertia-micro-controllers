/**
 * @file
 *
 * @brief Provides the entrypoint and per-target module configuration for the Sollertia microcontroller firmware.
 *
 * @note To upload the firmware to a target microcontroller, select the PlatformIO environment that matches the target
 * and use it to compile and upload the project. Only one Teensy 4.1 must be connected to the host PC during upload.
 *
 * @note Designed to work with the Python interfaces from the sollertia-experiment project
 * (https://github.com/Sun-Lab-NBB/sollertia-experiment). See https://github.com/Sun-Lab-NBB/sollertia-micro-controllers
 * for hardware-assembly and installation details. API documentation:
 * https://sollertia-micro-controllers-api-docs.netlify.app/.
 * Author: Ivan Kondratyev (Inkaros).
 */

#include <Arduino.h>
#include <communication.h>
#include <kernel.h>
#include <module.h>

Communication axmc_communication(Serial);  // NOLINT(*-interfaces-global-init)

/// Stores the interval, in milliseconds, at which the host PC has to send keepalive messages to the controller.
static constexpr uint32_t kKeepaliveInterval = 500;

/// Stores the baud rate of the serial connection with the host PC. Teensy boards ignore this value.
static constexpr uint32_t kSerialBaudRate = 115200;

/// Stores the Analog-to-Digital Converter (ADC) resolution, in bits. The 12-bit setting produces the 0 to 4095
/// readout range that keeps the analog sensor readouts clean.
static constexpr uint8_t kAnalogReadResolution = 12;

// Each PlatformIO environment defines exactly one of the target macros below through its build flags, so the build
// environment selects the target microcontroller. The reference VR system defines ACTOR, SENSOR, and ENCODER.

// The literals in the target blocks below are hardware assignments: digital and analog pin numbers, module type
// codes, per-controller instance IDs, and the torque sensor's ADC baseline. A named constant would restate the
// number without adding meaning, so the magic-number check is suppressed across the whole selection block.
// NOLINTBEGIN(*-magic-numbers)
#ifdef ACTOR
#include "brake_module.h"
#include "screen_module.h"
#include "valve_module.h"

/// Stores the unique identifier of this controller, which the host PC uses to address the board.
static constexpr uint8_t kControllerID = 101;
BrakeModule<33, false, true> wheel_brake(3, 1, axmc_communication);
ValveModule<35, true, true, 34> reward_valve(5, 1, axmc_communication);
ValveModule<32, true, true> gas_puff_valve(5, 2, axmc_communication);
ScreenModule<36, false> screen_trigger(7, 1, axmc_communication);
Module* modules[] = {&wheel_brake, &reward_valve, &gas_puff_valve, &screen_trigger};

#elif defined SENSOR
#include "lick_module.h"
#include "torque_module.h"
#include "ttl_module.h"

/// Stores the unique identifier of this controller, which the host PC uses to address the board.
static constexpr uint8_t kControllerID = 152;
TTLModule<34, false, false> mesoscope_frame(1, 1, axmc_communication);
LickModule<41> lick_sensor(4, 1, axmc_communication);
TorqueModule<40, 2048, true> torque_sensor(6, 1, axmc_communication);
Module* modules[] = {&mesoscope_frame, &lick_sensor, &torque_sensor};

#elif defined ENCODER
#include "encoder_module.h"

/// Stores the unique identifier of this controller, which the host PC uses to address the board.
static constexpr uint8_t kControllerID = 203;
EncoderModule<33, 34, 35, true> wheel_encoder(2, 1, axmc_communication);
Module* modules[] = {&wheel_encoder};
#else
static_assert(
    false,
    "Unable to resolve the target microcontroller. Build with a PlatformIO environment that defines one of the "
    "supported target macros (ACTOR, SENSOR, ENCODER)."
);
#endif
// NOLINTEND(*-magic-numbers)

Kernel axmc_kernel(kControllerID, axmc_communication, modules, kKeepaliveInterval);

void setup()
{
    Serial.begin(kSerialBaudRate);

    analogReadResolution(kAnalogReadResolution);

    axmc_kernel.Setup();
}

void loop()
{
    axmc_kernel.RuntimeCycle();
}
