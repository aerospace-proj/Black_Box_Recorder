/**
 * @file database.h
 * @brief SQLite persistence layer for the black-box simulator
 * @author Team G1
 *
 * The database file is "flight_data.db" in the working directory.
 *
 * Schema
 * ------
 *  flights (id, start_time, duration, crash_type, total_records)
 *  flight_records (id, flight_id, sim_time, phase, altitude, speed,
 *                  pitch, roll, has_error, error_message, crash_type)
 *
 * Only the last BLACKBOX_SIZE (200) records are retained per flight.
 *
 * Java side reads from this same file via JDBC/SQLite.
 */

#ifndef DATABASE_H
#define DATABASE_H

#include "circular_buffer.h"

/* ------------------------------------------------------------------ */
/*  Lifecycle                                                          */
/* ------------------------------------------------------------------ */

/**
 * @brief Opens (or creates) flight_data.db and initialises the schema.
 *        Assigns a new flight_id for the current session.
 *        Must be called before any other database function.
 */
void init_database(void);

/**
 * @brief Closes the SQLite connection.
 *        Safe to call even if init_database() was never called.
 */
void close_database(void);

/* ------------------------------------------------------------------ */
/*  Data management                                                    */
/* ------------------------------------------------------------------ */

/**
 * @brief Deletes all records from previous flights.
 *        Optional — only call when the user explicitly requests it.
 */
void clear_old_flight_data(void);

/**
 * @brief Inserts (or replaces) a batch of records for the current flight.
 *        Keeps only the most recent BLACKBOX_SIZE rows per flight.
 *
 * @param records Pointer to an array of BlackBoxRecord
 * @param count   Number of records in the array
 */
void save_all_records_to_db(BlackBoxRecord* records, int count);

/* ------------------------------------------------------------------ */
/*  Reporting                                                          */
/* ------------------------------------------------------------------ */

/**
 * @brief Returns the total number of flights stored in the database.
 */
int get_flight_count(void);

/**
 * @brief Prints a one-line crash summary for every flight in the DB.
 */
void print_all_crash_summaries(void);

#endif /* DATABASE_H */
