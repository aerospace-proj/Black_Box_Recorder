/**
 * @file utils.h
 * @brief Utility function declarations (timestamp, sensor reset)
 * @author Team G1
 */

#ifndef UTILS_H
#define UTILS_H

/**
 * @brief Writes the current wall-clock time as "HH:MM:SS" into buffer.
 * @param buffer Destination string buffer
 * @param size   Size of buffer in bytes (minimum 9)
 */
void get_timestamp(char* buffer, int size);

/**
 * @brief Resets all sensor internal states to their initial values.
 *        Calls reset_altitude(), reset_speed(), reset_pitch(), reset_roll().
 */
void reset_sensors(void);

#endif /* UTILS_H */
