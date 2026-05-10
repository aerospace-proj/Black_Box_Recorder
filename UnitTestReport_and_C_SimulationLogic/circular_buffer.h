/**
 * @file circular_buffer.h
 * @brief Black-box circular buffer — keeps the last 200 records
 * @author Team G1
 */

#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#define BLACKBOX_SIZE 200  /**< Maximum number of records stored in memory */

/**
 * @brief One second of recorded flight data.
 */
typedef struct {
    int   id;                   /**< Auto-assigned sequential ID             */
    float time_elapsed;         /**< Simulation time in seconds              */
    float altitude;             /**< Altitude in metres                      */
    float speed;                /**< Speed in m/s                            */
    float pitch;                /**< Pitch angle in degrees                  */
    float roll;                 /**< Roll angle in degrees                   */
    int   has_error;            /**< Non-zero when a sensor warning is active*/
    char  timestamp[30];        /**< Real-wall clock at time of recording     */
    char  error_message[150];   /**< Warning/error description               */
    char  crash_type[64];       /**< Name of crash type (or "NORMAL FLIGHT") */
    char  phase[32];            /**< Flight phase name                       */
} BlackBoxRecord;

/**
 * @brief Initialises (clears) the circular buffer.
 *        Must be called once before any recording.
 */
void init_blackbox(void);

/**
 * @brief Appends one record to the circular buffer.
 *        Overwrites the oldest record when full.
 * @param rec Record to store (id is assigned internally)
 */
void record_to_blackbox(BlackBoxRecord rec);

/**
 * @brief Returns the current number of records in the buffer (0..BLACKBOX_SIZE).
 */
int get_blackbox_count(void);

/**
 * @brief Returns a pointer to the raw internal buffer and sets *out to count.
 * @warning The order is NOT guaranteed to be chronological when the buffer
 *          has wrapped around. Use get_ordered_records() for analysis.
 * @param out Set to the number of valid records.
 * @return Pointer to internal array (do NOT free).
 */
BlackBoxRecord* get_all_records(int* out);

/**
 * @brief Returns a malloc'd chronological copy of all records.
 * @param out Set to the number of records in the returned array.
 * @return Caller-owned array (must call free() when done), or NULL on failure.
 */
BlackBoxRecord* get_ordered_records(int* out);

/**
 * @brief Prints all buffered records to stdout in a formatted table.
 */
void print_blackbox(void);

/**
 * @brief Prints a crash investigation report for the 30 seconds before impact.
 * @param crash_time Simulation time (seconds) when the crash was triggered.
 */
void print_pre_crash_analysis(float crash_time);

#endif /* CIRCULAR_BUFFER_H */
