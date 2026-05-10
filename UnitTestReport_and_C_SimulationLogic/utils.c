#include <stdio.h>
#include <time.h>

/**
 * @brief Gets current timestamp as HH:MM:SS string
 * @param buffer Output buffer
 * @param size   Buffer size
 */
void get_timestamp(char* buffer, int size) {
    time_t     now   = time(NULL);
    struct tm* t_ptr = localtime(&now);

    if (t_ptr != NULL) {
        strftime(buffer, size, "%H:%M:%S", t_ptr);
    } else {
        snprintf(buffer, size, "00:00:00");
    }
}

/**
 *  Resets all sensors
 */
void reset_sensors(void) {
    extern void reset_altitude(void);
    extern void reset_speed(void);
    extern void reset_pitch(void);
    extern void reset_roll(void);

    reset_altitude();
    reset_speed();
    reset_pitch();
    reset_roll();
}