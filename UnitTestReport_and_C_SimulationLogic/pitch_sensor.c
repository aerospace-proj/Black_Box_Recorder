/**
 * @author Mohammed
 */

#include "sensors.h"
#include <stdlib.h>

static float current_pitch = 0.0f;

void reset_pitch(void) {
    current_pitch = 0.0f;
}

float generate_pitch(FlightPhase phase, float target_pitch, int crashed,
    CrashType crash_type, float time_elapsed, float crash_trigger_time) {
    int random_int = rand() % 100;
    float noise = (float)(random_int - 50) / 50.0f;
    float response_rate = 4.0f;

    if (crashed && crash_type != CRASH_NONE) {
        current_pitch = apply_crash_pitch(crash_type, time_elapsed, crash_trigger_time);
        return current_pitch;
    }

    if (current_pitch < target_pitch) {
        current_pitch += response_rate;
        if (current_pitch > target_pitch) current_pitch = target_pitch;
    }
    else if (current_pitch > target_pitch) {
        current_pitch -= response_rate;
        if (current_pitch < target_pitch) current_pitch = target_pitch;
    }

    float final_pitch = current_pitch + noise;

    if (final_pitch > 15.0f) final_pitch = 15.0f;
    if (final_pitch < -10.0f) final_pitch = -10.0f;

    return final_pitch;
}