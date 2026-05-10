#include "crash_scenario.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

const char* get_crash_name(CrashType type) {
    switch (type) {
    case CRASH_NONE:           return "NORMAL FLIGHT";
    case CRASH_STALL:          return "AERODYNAMIC STALL";
    case CRASH_OVERSPEED:      return "OVERSPEED STRUCTURAL FAILURE";
    case CRASH_NOSE_DIVE:      return "UNCONTROLLED NOSE DIVE";
    case CRASH_SPIRAL:         return "SPIRAL DIVE";
    case CRASH_ENGINE_FAILURE: return "ENGINE FAILURE";
    case CRASH_CONTROL_FAILURE:return "CONTROL SYSTEM FAILURE";
    case CRASH_TERRAIN:        return "CFIT (Controlled Flight Into Terrain)";
    default:                   return "UNKNOWN";
    }
}

CrashType get_random_crash(void) {
    int crash_roll = rand() % 100;

    // 50% chance of crash for better testing
    if (crash_roll < 50) {
        return (CrashType)((rand() % 7) + 1);
    }

    return CRASH_NONE;
}

CrashInfo get_crash_info(CrashType type, float trigger_time) {
    CrashInfo info;
    info.type = type;
    info.trigger_time = trigger_time;
    info.impact_time = trigger_time + 30.0f;  /* ~30s to impact */

    // Use snprintf instead of sprintf (safe for Visual Studio)
    switch (type) {
    case CRASH_STALL:
        snprintf(info.description, sizeof(info.description),
            "Aircraft lost lift due to excessive angle of attack");
        snprintf(info.cause, sizeof(info.cause),
            "Low airspeed, high pitch attitude, possible icing");
        break;
    case CRASH_OVERSPEED:
        snprintf(info.description, sizeof(info.description),
            "Aircraft exceeded maximum design speed");
        snprintf(info.cause, sizeof(info.cause),
            "Uncontrolled descent, pilot error, turbulence");
        break;
    case CRASH_NOSE_DIVE:
        snprintf(info.description, sizeof(info.description),
            "Aircraft pitched down violently");
        snprintf(info.cause, sizeof(info.cause),
            "Control system malfunction, trim runaway, pilot disorientation");
        break;
    case CRASH_SPIRAL:
        snprintf(info.description, sizeof(info.description),
            "Aircraft entered uncontrolled spiral descent");
        snprintf(info.cause, sizeof(info.cause),
            "Spatial disorientation, instrument failure, IMC conditions");
        break;
    case CRASH_ENGINE_FAILURE:
        snprintf(info.description, sizeof(info.description),
            "Complete loss of engine power");
        snprintf(info.cause, sizeof(info.cause),
            "Fuel starvation, mechanical failure, bird strike");
        break;
    case CRASH_CONTROL_FAILURE:
        snprintf(info.description, sizeof(info.description),
            "Loss of pitch/roll control authority");
        snprintf(info.cause, sizeof(info.cause),
            "Hydraulic failure, control cable break, actuator malfunction");
        break;
    case CRASH_TERRAIN:
        snprintf(info.description, sizeof(info.description),
            "Controlled flight into terrain");
        snprintf(info.cause, sizeof(info.cause),
            "Poor visibility, navigation error, altimeter setting error");
        break;
    default:
        snprintf(info.description, sizeof(info.description), "Unknown crash cause");
        snprintf(info.cause, sizeof(info.cause), "Further investigation required");
        break;
    }

    return info;
}

float apply_crash_altitude(CrashType crash_type, float time_elapsed,
    float trigger_time, float current_altitude) {
    float time_since_crash = time_elapsed - trigger_time;
    if (time_since_crash < 0) time_since_crash = 0;

    float new_altitude = current_altitude;

    /*
     * Realistic fall rates using power curve: altitude = A - rate * t^1.4
     * Slow start (confusion/recovery attempt) then rapid acceleration.
     * Approximate time to impact from 9000m cruise:
     *   STALL          ~60s  |  OVERSPEED    ~45s  |  NOSE_DIVE    ~50s
     *   SPIRAL         ~55s  |  ENGINE       ~90s  |  CONTROL      ~65s
     *   TERRAIN        ~70s
     */
    /*
     * Power curve: altitude = current - rate * t^1.4
     * All scenarios reach 0m from 9000m in approximately 30 seconds.
     * Slight variation per crash type for realism:
     *   STALL         ~32s  |  OVERSPEED   ~26s  |  NOSE_DIVE   ~28s
     *   SPIRAL        ~30s  |  ENGINE      ~35s  |  CONTROL     ~30s
     *   TERRAIN       ~33s
     */
    float t_pow = powf(time_since_crash, 1.4f);

    switch (crash_type) {
    case CRASH_STALL:
        new_altitude = current_altitude - (t_pow * 70.3f);
        break;
    case CRASH_OVERSPEED:
        new_altitude = current_altitude - (t_pow * 94.0f);
        break;
    case CRASH_NOSE_DIVE:
        new_altitude = current_altitude - (t_pow * 84.8f);
        break;
    case CRASH_SPIRAL:
        new_altitude = current_altitude - (t_pow * 77.0f);
        break;
    case CRASH_ENGINE_FAILURE:
        new_altitude = current_altitude - (t_pow * 62.0f);
        break;
    case CRASH_CONTROL_FAILURE:
        new_altitude = current_altitude - (t_pow * 77.0f);
        break;
    case CRASH_TERRAIN:
        new_altitude = current_altitude - (t_pow * 67.3f);
        break;
    default:
        break;
    }

    if (new_altitude < 0) new_altitude = 0;
    return new_altitude;
}

float apply_crash_pitch(CrashType crash_type, float time_elapsed, float trigger_time) {
    float time_since_crash = time_elapsed - trigger_time;
    if (time_since_crash < 0) time_since_crash = 0;

    switch (crash_type) {
    case CRASH_STALL:
    {
        float pitch = 25.0f - (time_since_crash * 8.0f);
        if (pitch < -45.0f) pitch = -45.0f;
        return pitch;
    }
    case CRASH_OVERSPEED:
    {
        float pitch = -10.0f - (time_since_crash * 5.0f);
        if (pitch < -45.0f) pitch = -45.0f;
        return pitch;
    }
    case CRASH_NOSE_DIVE:
        return -45.0f;
    case CRASH_SPIRAL:
        return -30.0f;
    case CRASH_ENGINE_FAILURE:
        return -5.0f;
    case CRASH_CONTROL_FAILURE:
        return 15.0f;
    case CRASH_TERRAIN:
        return 0.0f;
    default:
        return 0.0f;
    }
}
