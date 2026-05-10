/**
 * @file crash_scenario.h
 * @brief Crash scenario types, info structures, and declarations
 * @author Team G1
 */

#ifndef CRASH_SCENARIO_H
#define CRASH_SCENARIO_H

/**
 * @brief Enumeration of all possible crash types
 */
typedef enum {
    CRASH_NONE            = 0,
    CRASH_STALL           = 1,
    CRASH_OVERSPEED       = 2,
    CRASH_NOSE_DIVE       = 3,
    CRASH_SPIRAL          = 4,
    CRASH_ENGINE_FAILURE  = 5,
    CRASH_CONTROL_FAILURE = 6,
    CRASH_TERRAIN         = 7
} CrashType;

/**
 * @brief Holds metadata about a crash scenario
 */
typedef struct {
    CrashType type;
    float     trigger_time;               /**< When the crash starts (seconds) */
    float     impact_time;                /**< Estimated impact time (seconds) */
    char      description[256];           /**< Human-readable description */
    char      cause[256];                 /**< Root cause */
} CrashInfo;

/**
 * @brief Returns the human-readable name for a crash type.
 */
const char* get_crash_name(CrashType type);

/**
 * @brief Randomly selects a crash type (50% chance of crash).
 * @return Selected CrashType (may be CRASH_NONE)
 */
CrashType get_random_crash(void);

/**
 * @brief Builds a CrashInfo struct for the given type and trigger time.
 */
CrashInfo get_crash_info(CrashType type, float trigger_time);

/**
 * @brief Computes the new altitude during a crash sequence.
 * @param crash_type      Type of crash
 * @param time_elapsed    Current simulation time (seconds)
 * @param trigger_time    Time at which crash was triggered (seconds)
 * @param current_altitude Altitude just before calling this function
 * @return New altitude (clamped >= 0)
 */
float apply_crash_altitude(CrashType crash_type, float time_elapsed,
                           float trigger_time, float current_altitude);

/**
 * @brief Computes the pitch angle during a crash sequence.
 * @param crash_type   Type of crash
 * @param time_elapsed Current simulation time (seconds)
 * @param trigger_time Time at which crash was triggered (seconds)
 * @return Pitch angle in degrees
 */
float apply_crash_pitch(CrashType crash_type, float time_elapsed,
                        float trigger_time);

#endif /* CRASH_SCENARIO_H */
