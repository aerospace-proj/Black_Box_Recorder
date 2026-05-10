#include "flight_phase.h"
#include <stdio.h>

/* ---------------------------------------------------------------
 * Phase proportions (must sum to 1.0):
 *   ON_GROUND :  4%
 *   TAKEOFF   :  5%
 *   CLIMB     : 21%
 *   CRUISE    : 40%
 *   DESCENT   : 18%
 *   LANDING   : 12%
 * --------------------------------------------------------------- */
#define P_GROUND   0.04f
#define P_TAKEOFF  0.05f
#define P_CLIMB    0.21f
#define P_CRUISE   0.40f
#define P_DESCENT  0.18f
/* LANDING = remaining time */

static float T_GROUND  = 15.0f;
static float T_TAKEOFF = 35.0f;
static float T_CLIMB   = 120.0f;
static float T_CRUISE  = 280.0f;
static float T_DESCENT = 350.0f;
/* landing ends at total_duration */

static int total_dur = 380;

/**
 * @brief Call once with chosen flight duration before simulation loop.
 *        Recalculates all phase boundary times.
 */
void set_flight_duration(int total_seconds) {
    total_dur  = total_seconds;
    float D    = (float)total_seconds;

    T_GROUND   = D * P_GROUND;                              /* end of ON_GROUND  */
    T_TAKEOFF  = T_GROUND  + D * P_TAKEOFF;                 /* end of TAKEOFF    */
    T_CLIMB    = T_TAKEOFF + D * P_CLIMB;                   /* end of CLIMB      */
    T_CRUISE   = T_CLIMB   + D * P_CRUISE;                  /* end of CRUISE     */
    T_DESCENT  = T_CRUISE  + D * P_DESCENT;                 /* end of DESCENT    */
    /* LANDING: T_DESCENT -> total_seconds */

    printf("[PHASES] Duration=%ds | ON_GROUND:0-%.0fs | TAKEOFF:%.0f-%.0fs"
           " | CLIMB:%.0f-%.0fs | CRUISE:%.0f-%.0fs"
           " | DESCENT:%.0f-%.0fs | LANDING:%.0f-%ds\n",
           total_seconds,
           T_GROUND,
           T_GROUND, T_TAKEOFF,
           T_TAKEOFF, T_CLIMB,
           T_CLIMB, T_CRUISE,
           T_CRUISE, T_DESCENT,
           T_DESCENT, total_seconds);
}

/* --------------------------------------------------------------- */
const char* get_phase_name(FlightPhase phase) {
    switch (phase) {
    case PHASE_ON_GROUND:  return "ON GROUND";
    case PHASE_TAKEOFF:    return "TAKEOFF";
    case PHASE_CLIMB:      return "CLIMB";
    case PHASE_CRUISE:     return "CRUISE";
    case PHASE_DESCENT:    return "DESCENT";
    case PHASE_LANDING:    return "LANDING";
    case PHASE_CRASH:      return "CRASH";
    case PHASE_POST_CRASH: return "POST-CRASH";
    default:               return "UNKNOWN";
    }
}

/* --------------------------------------------------------------- */
FlightPhase update_flight_phase(float time_elapsed, int crashed) {
    if (crashed) return PHASE_CRASH;

    if      (time_elapsed < T_GROUND)  return PHASE_ON_GROUND;
    else if (time_elapsed < T_TAKEOFF) return PHASE_TAKEOFF;
    else if (time_elapsed < T_CLIMB)   return PHASE_CLIMB;
    else if (time_elapsed < T_CRUISE)  return PHASE_CRUISE;
    else if (time_elapsed < T_DESCENT) return PHASE_DESCENT;
    else                               return PHASE_LANDING;
}

/* --------------------------------------------------------------- */
float get_target_altitude(FlightPhase phase, float time_elapsed) {
    float alt = 0.0f;

    switch (phase) {
    case PHASE_ON_GROUND:
        alt = 0.0f;
        break;

    case PHASE_TAKEOFF: {
        float dur = T_TAKEOFF - T_GROUND;
        float prog = (time_elapsed - T_GROUND) / dur;
        alt = prog * 500.0f;
        if (alt < 0)   alt = 0;
        if (alt > 500) alt = 500;
        break;
    }

    case PHASE_CLIMB: {
        float dur = T_CLIMB - T_TAKEOFF;
        float prog = (time_elapsed - T_TAKEOFF) / dur;
        alt = 500.0f + prog * 8500.0f;
        if (alt > 9000) alt = 9000;
        break;
    }

    case PHASE_CRUISE:
        alt = 9000.0f;
        break;

    case PHASE_DESCENT: {
        float dur = T_DESCENT - T_CRUISE;
        float prog = (time_elapsed - T_CRUISE) / dur;
        alt = 9000.0f - prog * 8500.0f;
        if (alt < 500) alt = 500;
        break;
    }

    case PHASE_LANDING: {
        float dur = (float)total_dur - T_DESCENT;
        float prog = (time_elapsed - T_DESCENT) / dur;
        alt = 500.0f - prog * 500.0f;
        if (alt < 0) alt = 0;
        break;
    }

    default:
        alt = 0.0f;
        break;
    }

    return alt;
}

/* --------------------------------------------------------------- */
float get_target_speed(FlightPhase phase, float altitude) {
    float spd = 0.0f;

    switch (phase) {
    case PHASE_ON_GROUND: spd = 0.0f;   break;
    case PHASE_TAKEOFF:   spd = 75.0f;  break;
    case PHASE_CLIMB:
        spd = 75.0f + (altitude / 9000.0f) * 157.0f;
        if (spd > 232) spd = 232; /* 232 m/s = 451 kts cruise speed */
        break;
    case PHASE_CRUISE:    spd = 232.0f; break; /* 232 m/s = 451 kts */
    case PHASE_DESCENT:   spd = 160.0f; break; /* 160 m/s = 311 kts */
    case PHASE_LANDING:
        spd = 72.0f * (altitude / 500.0f);
        if (spd > 72) spd = 72; /* 72 m/s = 140 kts Vref */
        if (spd < 0)  spd = 0;
        break;
    default: spd = 0.0f; break;
    }

    return spd;
}

/* --------------------------------------------------------------- */
float get_target_pitch(FlightPhase phase) {
    switch (phase) {
    case PHASE_ON_GROUND: return  0.0f;
    case PHASE_TAKEOFF:   return 12.0f;
    case PHASE_CLIMB:     return  8.0f;
    case PHASE_CRUISE:    return  2.0f;
    case PHASE_DESCENT:   return -3.0f;
    case PHASE_LANDING:   return  5.0f;
    default:              return  0.0f;
    }
}

/* --------------------------------------------------------------- */
float get_target_roll(FlightPhase phase) {
    switch (phase) {
    case PHASE_TAKEOFF:
    case PHASE_LANDING:
        return 0.0f;
    default:
        return 2.0f;
    }
}
