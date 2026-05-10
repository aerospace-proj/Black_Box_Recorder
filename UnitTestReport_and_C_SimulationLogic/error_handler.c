#include "error_handler.h"
#include <stdio.h>
#include <string.h>

/* Sensor limits */
#define ALT_MIN    0.0f
#define ALT_MAX    10000.0f
#define SPD_MIN    0.0f
#define SPD_MAX    300.0f  /* 583 kts = Vmo */
#define PITCH_MIN  -45.0f
#define PITCH_MAX   45.0f
#define ROLL_MIN   -90.0f
#define ROLL_MAX    90.0f

static void log_to_file(const char* message) {
    FILE* f = fopen("errors.txt", "a");
    if (f) {
        fprintf(f, "%s\n", message);
        fclose(f);
    }
}

void check_errors(SensorData* sensor) {
    if (!sensor) return;

    sensor->has_error        = 0;
    sensor->error_message[0] = '\0';

    char buf[150];

    /* Altitude */
    if (sensor->altitude < ALT_MIN || sensor->altitude > ALT_MAX) {
        snprintf(buf, sizeof(buf),
                 "[%s] ALTITUDE OUT OF RANGE: %.1f m (limit %.0f..%.0f)",
                 sensor->timestamp, sensor->altitude, ALT_MIN, ALT_MAX);
        strncpy(sensor->error_message, buf, sizeof(sensor->error_message) - 1);
        sensor->has_error = 1;
        log_to_file(buf);
        return;   /* one error per tick is enough */
    }

    /* Speed */
    if (sensor->speed < SPD_MIN || sensor->speed > SPD_MAX) {
        snprintf(buf, sizeof(buf),
                 "[%s] SPEED OUT OF RANGE: %.1f m/s (limit %.0f..%.0f)",
                 sensor->timestamp, sensor->speed, SPD_MIN, SPD_MAX);
        strncpy(sensor->error_message, buf, sizeof(sensor->error_message) - 1);
        sensor->has_error = 1;
        log_to_file(buf);
        return;
    }

    /* Pitch */
    if (sensor->pitch < PITCH_MIN || sensor->pitch > PITCH_MAX) {
        snprintf(buf, sizeof(buf),
                 "[%s] PITCH OUT OF RANGE: %.1f deg (limit %.0f..%.0f)",
                 sensor->timestamp, sensor->pitch, PITCH_MIN, PITCH_MAX);
        strncpy(sensor->error_message, buf, sizeof(sensor->error_message) - 1);
        sensor->has_error = 1;
        log_to_file(buf);
        return;
    }

    /* Roll */
    if (sensor->roll < ROLL_MIN || sensor->roll > ROLL_MAX) {
        snprintf(buf, sizeof(buf),
                 "[%s] ROLL OUT OF RANGE: %.1f deg (limit %.0f..%.0f)",
                 sensor->timestamp, sensor->roll, ROLL_MIN, ROLL_MAX);
        strncpy(sensor->error_message, buf, sizeof(sensor->error_message) - 1);
        sensor->has_error = 1;
        log_to_file(buf);
        return;
    }
}
