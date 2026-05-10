#include "database.h"
#include "circular_buffer.h"
#include <stdio.h>
#include <string.h>
#include <sqlite3.h>


static sqlite3* db = NULL;

static void exec_sql(const char* sql) {
    char* err = NULL;
    if (sqlite3_exec(db, sql, NULL, NULL, &err) != SQLITE_OK) {
        fprintf(stderr, "[DATABASE] SQL error: %s\n", err ? err : "unknown");
        sqlite3_free(err);
    }
}

void init_database(void) {
    if (sqlite3_open("flight_data.db", &db) != SQLITE_OK) {
        fprintf(stderr, "[DATABASE] Cannot open flight_data.db: %s\n",
                sqlite3_errmsg(db));
        db = NULL;
        return;
    }

    /* Performance pragmas */
    exec_sql("PRAGMA journal_mode=WAL;");
    exec_sql("PRAGMA synchronous=NORMAL;");

    /* Create tables with the exact schema Java reads */
    exec_sql(
        "CREATE TABLE IF NOT EXISTS flight_data ("
        "  id         INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  sim_time   REAL    NOT NULL,"
        "  phase      TEXT    NOT NULL,"
        "  altitude   REAL    NOT NULL,"
        "  speed      REAL    NOT NULL,"
        "  pitch      REAL    NOT NULL,"
        "  roll       REAL    NOT NULL,"
        "  crashed    INTEGER NOT NULL,"
        "  crash_type TEXT    NOT NULL"
        ");"
    );

    exec_sql(
        "CREATE TABLE IF NOT EXISTS flight_events ("
        "  id         INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  event_type TEXT    NOT NULL,"
        "  sim_time   REAL    NOT NULL,"
        "  message    TEXT    NOT NULL,"
        "  real_time  TEXT    NOT NULL"
        ");"
    );

    printf("[DATABASE] Opened flight_data.db\n");
}

void close_database(void) {
    if (db) {
        sqlite3_close(db);
        db = NULL;
        printf("[DATABASE] Connection closed.\n");
    }
}

/*  Data management                                                    */
void clear_old_flight_data(void) {
    if (!db) return;
    exec_sql("DELETE FROM flight_data;");
    exec_sql("DELETE FROM flight_events;");
    exec_sql("DELETE FROM sqlite_sequence WHERE name='flight_data';");
    exec_sql("DELETE FROM sqlite_sequence WHERE name='flight_events';");
    printf("[DATABASE] All previous flight data cleared.\n");
}

/**
 * Saves up to the last BLACKBOX_SIZE (200) records to flight_data.
 * Clears existing rows first so the table always holds one flight's data.
 */
void save_all_records_to_db(BlackBoxRecord* records, int count) {
    if (!db || !records || count <= 0) return;

    /* Wipe current content */
    exec_sql("DELETE FROM flight_data;");

    const char* sql =
        "INSERT INTO flight_data"
        "(sim_time, phase, altitude, speed, pitch, roll, crashed, crash_type)"
        " VALUES (?, ?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "[DATABASE] Prepare failed: %s\n", sqlite3_errmsg(db));
        return;
    }

    /* Keep only the most recent BLACKBOX_SIZE records */
    int start = (count > BLACKBOX_SIZE) ? count - BLACKBOX_SIZE : 0;

    exec_sql("BEGIN TRANSACTION;");

    for (int i = start; i < count; i++) {
        BlackBoxRecord* r = &records[i];

        /*
         * crashed column:
         *   1  if the record belongs to a crash (has_error set by crash logic)
         *   0  otherwise
         * Java reads: rs.getInt(7) == 1 -> isCrashed()
         */
        int crashed_flag = (strncmp(r->crash_type, "NORMAL FLIGHT", 13) != 0
                            && r->crash_type[0] != '\0') ? 1 : 0;

        sqlite3_bind_double(stmt, 1, (double)r->time_elapsed);
        sqlite3_bind_text  (stmt, 2, r->phase,      -1, SQLITE_STATIC);
        sqlite3_bind_double(stmt, 3, (double)r->altitude);
        sqlite3_bind_double(stmt, 4, (double)r->speed);
        sqlite3_bind_double(stmt, 5, (double)r->pitch);
        sqlite3_bind_double(stmt, 6, (double)r->roll);
        sqlite3_bind_int   (stmt, 7, crashed_flag);
        sqlite3_bind_text  (stmt, 8, r->crash_type, -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            fprintf(stderr, "[DATABASE] Insert error at record %d: %s\n",
                    i, sqlite3_errmsg(db));
        }
        sqlite3_reset(stmt);
    }

    exec_sql("COMMIT;");
    sqlite3_finalize(stmt);

    printf("[DATABASE] Saved %d records to flight_data.db\n",
           count - start);
}

/* ------------------------------------------------------------------ */
/*  Reporting                                                          */
/* ------------------------------------------------------------------ */

int get_flight_count(void) {
    if (!db) return 0;

    sqlite3_stmt* stmt = NULL;
    const char* sql = "SELECT COUNT(*) FROM flight_data;";
    int result = 0;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW)
            result = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    return result;
}

void print_all_crash_summaries(void) {
    if (!db) return;

    const char* sql =
        "SELECT sim_time, phase, altitude, speed, crash_type"
        " FROM flight_data"
        " WHERE crashed = 1"
        " ORDER BY sim_time ASC;";

    sqlite3_stmt* stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "[DATABASE] Query failed: %s\n", sqlite3_errmsg(db));
        return;
    }

    printf("\n=== CRASH RECORDS IN DATABASE ===\n");
    printf(" Time  | Phase        | Alt (m)  | Spd (m/s) | Crash Type\n");
    printf("-------|--------------|----------|-----------|-----------------------------\n");

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        found = 1;
        printf(" %5.0f | %-12s | %8.1f | %9.1f | %s\n",
               sqlite3_column_double(stmt, 0),
               sqlite3_column_text  (stmt, 1),
               sqlite3_column_double(stmt, 2),
               sqlite3_column_double(stmt, 3),
               sqlite3_column_text  (stmt, 4));
    }

    if (!found)
        printf("  (no crash records found)\n");

    printf("=== END CRASH RECORDS ===\n\n");
    sqlite3_finalize(stmt);
}
