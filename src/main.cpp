/**
 * @file
 *
 * @brief Provides the entrypoint and per-target module configuration for the Sollertia microcontroller firmware.
 *
 * @note To upload the firmware to a target microcontroller, modify the active target macro below and use PlatformIO
 * to compile and upload the project. Only one Teensy 4.1 must be connected to the host PC at the time of upload.
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

static constexpr uint32_t kKeepaliveInterval = 500;  // 500 milliseconds.

// Defines the target microcontroller. The reference VR system currently has 3 valid targets: ACTOR, SENSOR, ENCODER.
#define ACTOR

#ifdef ACTOR
#include "brake_module.h"
#include "screen_module.h"
#include "valve_module.h"

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

static constexpr uint8_t kControllerID = 152;
TTLModule<34, false, false> mesoscope_frame(1, 1, axmc_communication);
LickModule<41> lick_sensor(4, 1, axmc_communication);
TorqueModule<40, 2048, true> torque_sensor(6, 1, axmc_communication);
Module* modules[] = {&mesoscope_frame, &lick_sensor, &torque_sensor};

#elif defined ENCODER
#include "encoder_module.h"

static constexpr uint8_t kControllerID = 203;
EncoderModule<33, 34, 35, true> wheel_encoder(2, 1, axmc_communication);
Module* modules[] = {&wheel_encoder};
#else
static_assert(false, "Define one of the supported microcontroller targets (ACTOR, SENSOR, ENCODER).");
#endif

Kernel axmc_kernel(kControllerID, axmc_communication, modules, kKeepaliveInterval);

void setup()
{
    Serial.begin(115200);  // The baudrate is ignored for Teensy boards.

    // Teensy boards can support up to 16-bit ADC resolution, but 12-bit often produces cleaner readouts.
    analogReadResolution(12);

    axmc_kernel.Setup();
}

void loop()
{
    axmc_kernel.RuntimeCycle();
}
