/**
 * @file error_handler.h
 * @brief Sensor range validation and error logging
 * @author Team G1
 */

#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include "sensors.h"

/**
 * @brief Checks all sensor values in *sensor for out-of-range conditions.
 *        Sets sensor->has_error and sensor->error_message when a problem
 *        is found, and appends a line to errors.txt.
 *
 * Limits checked:
 *   Altitude : 0 .. 10 000 m
 *   Speed    : 0 .. 300 m/s
 *   Pitch    : -45 .. +45 degrees
 *   Roll     : -90 .. +90 degrees
 *
 * @param sensor Pointer to the SensorData struct to validate (modified in place)
 */
void check_errors(SensorData* sensor);

#endif /* ERROR_HANDLER_H */
