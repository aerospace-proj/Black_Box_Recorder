/**
 * @file sensors.h
 * @brief Sensor generation declarations (altitude, speed, pitch, roll)
 *        and the SensorData struct used for error-checking.
 * @author Team G1
 */

#ifndef SENSORS_H
#define SENSORS_H

#include "flight_phase.h"
#include "crash_scenario.h"

/* ------------------------------------------------------------------ */
/*  Sensor data structure (used by error_handler)                     */
/* ------------------------------------------------------------------ */

/**
 * @brief Container for a single sensor reading plus any error flag.
 */
typedef struct {
    char  timestamp[30];       /**< Real-wall clock string "HH:MM:SS"   */
    float altitude;            /**< Altitude in metres                   */
    float speed;               /**< Speed in m/s                         */
    float pitch;               /**< Pitch angle in degrees               */
    float roll;                /**< Roll angle in degrees                */
    int   has_error;           /**< Non-zero when out-of-range detected  */
    char  error_message[150];  /**< Human-readable error description     */
} SensorData;

/* ------------------------------------------------------------------ */
/*  Individual sensor generators                                       */
/* ------------------------------------------------------------------ */

/**
 * @brief Resets internal altitude state to 0.
 */
void reset_altitude(void);

/**
 * @brief Generates a realistic altitude reading for the current tick.
 *
 * @param phase              Current flight phase
 * @param target_altitude    Ideal altitude from get_target_altitude()
 * @param time_elapsed       Current simulation time (seconds)
 * @param crashed            Non-zero when the aircraft has crashed
 * @param crash_type         Active crash type (CRASH_NONE if none)
 * @param crash_trigger_time Time (s) at which crash was triggered
 * @return Altitude in metres (clamped 0..10000)
 */
float generate_altitude(FlightPhase phase, float target_altitude,
                        float time_elapsed, int crashed,
                        CrashType crash_type, float crash_trigger_time);

/* ------------------------------------------------------------------ */

/**
 * @brief Resets internal speed state to 0.
 */
void reset_speed(void);

/**
 * @brief Generates a realistic speed reading for the current tick.
 *
 * @param phase        Current flight phase
 * @param target_speed Ideal speed from get_target_speed()
 * @param crashed      Non-zero when the aircraft has crashed
 * @return Speed in m/s (>= 0)
 */
float generate_speed(FlightPhase phase, float target_speed, int crashed);

/**
 * @brief Converts m/s to knots.
 * @param speed_ms Speed in metres per second
 * @return Speed in knots
 */
float speed_to_knots(float speed_ms);

/* ------------------------------------------------------------------ */

/**
 * @brief Resets internal pitch state to 0.
 */
void reset_pitch(void);

/**
 * @brief Generates a realistic pitch reading for the current tick.
 *
 * @param phase              Current flight phase
 * @param target_pitch       Ideal pitch from get_target_pitch()
 * @param crashed            Non-zero when the aircraft has crashed
 * @param crash_type         Active crash type
 * @param time_elapsed       Current simulation time (seconds)
 * @param crash_trigger_time Time (s) at which crash was triggered
 * @return Pitch angle in degrees
 */
float generate_pitch(FlightPhase phase, float target_pitch, int crashed,
                     CrashType crash_type, float time_elapsed,
                     float crash_trigger_time);

/* ------------------------------------------------------------------ */

/**
 * @brief Resets internal roll state to 0.
 */
void reset_roll(void);

/**
 * @brief Generates a realistic roll reading for the current tick.
 *
 * @param phase       Current flight phase
 * @param target_roll Ideal roll from get_target_roll()
 * @param crashed     Non-zero when the aircraft has crashed
 * @return Roll angle in degrees
 */
float generate_roll(FlightPhase phase, float target_roll, int crashed);

#endif /* SENSORS_H */
