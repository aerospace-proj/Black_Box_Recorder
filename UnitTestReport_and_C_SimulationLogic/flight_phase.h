/**
 * @file flight_phase.h
 * @brief Flight phase definitions and declarations
 * @author Team G1
 */

#ifndef FLIGHT_PHASE_H
#define FLIGHT_PHASE_H

/**
 * @brief All possible flight phases
 */
typedef enum {
    PHASE_ON_GROUND  = 0,
    PHASE_TAKEOFF    = 1,
    PHASE_CLIMB      = 2,
    PHASE_CRUISE     = 3,
    PHASE_DESCENT    = 4,
    PHASE_LANDING    = 5,
    PHASE_CRASH      = 6,
    PHASE_POST_CRASH = 7
} FlightPhase;

/**
 * @brief Sets all phase boundary times proportionally from total duration.
 *        Must be called once before the simulation loop.
 * @param total_seconds Total flight duration in seconds
 */
void set_flight_duration(int total_seconds);

/**
 * @brief Returns the human-readable name of a flight phase.
 */
const char* get_phase_name(FlightPhase phase);

/**
 * @brief Determines the current flight phase based on elapsed time.
 * @param time_elapsed Simulation time in seconds
 * @param crashed      Non-zero if aircraft has crashed
 * @return Current FlightPhase
 */
FlightPhase update_flight_phase(float time_elapsed, int crashed);

/**
 * @brief Returns the target altitude (m) for a given phase and time.
 */
float get_target_altitude(FlightPhase phase, float time_elapsed);

/**
 * @brief Returns the target speed (m/s) for a given phase and current altitude.
 */
float get_target_speed(FlightPhase phase, float altitude);

/**
 * @brief Returns the target pitch angle (degrees) for a given phase.
 */
float get_target_pitch(FlightPhase phase);

/**
 * @brief Returns the target roll angle (degrees) for a given phase.
 */
float get_target_roll(FlightPhase phase);

#endif /* FLIGHT_PHASE_H */
