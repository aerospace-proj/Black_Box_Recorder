/**
 * @author Yasmine
 */

#include "sensors.h"
#include <stdlib.h>
#include <math.h>

static float previous_altitude = 0.0f;

void reset_altitude(void) {
    previous_altitude = 0.0f;
}

float generate_altitude(FlightPhase phase, float target_altitude,
    float time_elapsed, int crashed,
    CrashType crash_type, float crash_trigger_time) {
    float new_altitude;

    /* The Limit target altitude to valid range is :  0 <= Altitude <= 10000  */
    
    if (target_altitude < 0) target_altitude = 0;
    if (target_altitude > 10000) target_altitude = 10000;

    // CRASH EFFECTS
    if (crashed && crash_type != CRASH_NONE) {
        /*
         * Progressive drop: each second we subtract a small amount
         * that grows over time (acceleration effect).
         * time_since_crash = seconds elapsed since crash triggered.
         * drop_per_second = base_rate + acceleration * time_since_crash
         *
         * This makes the fall last ~30 seconds from any altitude,
         * because we subtract from previous_altitude each tick.
         */
        float time_since_crash = time_elapsed - crash_trigger_time;
        if (time_since_crash < 0) time_since_crash = 0;

        float base_rate = 0.0f;
        float accel     = 0.0f;

        /*
         * Values calibrated by simulation to give exact fall duration from 9000m:
         *   STALL          30s  |  OVERSPEED   25s  |  NOSE_DIVE  27s
         *   SPIRAL         28s  |  ENGINE      35s  |  CONTROL    29s
         *   TERRAIN        32s
         * Formula: drop_this_second = base_rate + accel * time_since_crash
         * Applied to previous_altitude each tick (not to initial altitude).
         */
        switch (crash_type) {
        case CRASH_STALL:
            base_rate = 99.0f;  accel = 14.0f; break;  /* 30s to impact */
        case CRASH_OVERSPEED:
            base_rate = 99.0f;  accel = 22.0f; break;  /* 25s to impact */
        case CRASH_NOSE_DIVE:
            base_rate = 99.0f;  accel = 19.0f; break;  /* 27s to impact */
        case CRASH_SPIRAL:
            base_rate = 99.0f;  accel = 17.0f; break;  /* 28s to impact */
        case CRASH_ENGINE_FAILURE:
            base_rate = 99.0f;  accel = 10.0f; break;  /* 35s to impact */
        case CRASH_CONTROL_FAILURE:
            base_rate = 99.0f;  accel = 16.0f; break;  /* 29s to impact */
        case CRASH_TERRAIN:
            base_rate = 99.0f;  accel = 12.0f; break;  /* 32s to impact */
        default:
            base_rate = 99.0f;  accel = 14.0f; break;
        }

        float drop = base_rate + accel * time_since_crash;
        new_altitude = previous_altitude - drop;

        if (new_altitude < 0)     new_altitude = 0;
        if (new_altitude > 10000) new_altitude = 10000;
        previous_altitude = new_altitude;
        return new_altitude;
    }

    // NORMAL FLIGHT - with boundary protection
    int random_int = rand() % 100;
    float noise = (float)(random_int - 50) / 10.0f;

    // Protect against invalid phase values
    if (phase == PHASE_TAKEOFF || phase == PHASE_CLIMB) {
        new_altitude = target_altitude + noise;
        if (new_altitude < previous_altitude) new_altitude = previous_altitude;
        if (new_altitude < 0) new_altitude = 0;
        if (new_altitude > 10000) new_altitude = 10000;
    }
    else if (phase == PHASE_DESCENT || phase == PHASE_LANDING) {
        new_altitude = target_altitude + noise;
        if (new_altitude > previous_altitude && previous_altitude > 0) {
            new_altitude = previous_altitude;
        }
        if (new_altitude < 0) new_altitude = 0;
    }
    else if (phase == PHASE_ON_GROUND) {
        new_altitude = 0.0f;
    }
    else {
        new_altitude = target_altitude + noise * 0.3f;
        if (new_altitude < 0) new_altitude = 0;
        if (new_altitude > 10000) new_altitude = 10000;
    }

    previous_altitude = new_altitude;
    return new_altitude;
}
