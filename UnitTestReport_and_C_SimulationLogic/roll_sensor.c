/**
 * @author Zakaria
 */

#include "sensors.h"
#include <stdlib.h>

static float current_roll = 0.0f;

void reset_roll(void) {
    current_roll = 0.0f;
}

float generate_roll(FlightPhase phase, float target_roll, int crashed) {
    int random_int = rand() % 100;
    float noise = (float)(random_int - 50) / 100.0f;
    float response_rate = 3.0f;

    if (crashed) {
        current_roll = current_roll + noise * 10.0f;
        if (current_roll > 90.0f) current_roll = 90.0f;
        if (current_roll < -90.0f) current_roll = -90.0f;
        return current_roll;
    }

    if (phase == PHASE_TAKEOFF || phase == PHASE_LANDING) {
        target_roll = 0.0f;
    }

    if (current_roll < target_roll) {
        current_roll += response_rate;
        if (current_roll > target_roll) current_roll = target_roll;
    }
    else if (current_roll > target_roll) {
        current_roll -= response_rate;
        if (current_roll < target_roll) current_roll = target_roll;
    }

    float final_roll = current_roll + noise;

    if (final_roll > 35.0f) final_roll = 35.0f;
    if (final_roll < -35.0f) final_roll = -35.0f;

    return final_roll;
}



