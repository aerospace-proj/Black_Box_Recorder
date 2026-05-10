#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <string.h>
#include "flight_phase.h"
#include "sensors.h"
#include "circular_buffer.h"
#include "error_handler.h"
#include "event_handler.h"
#include "crash_scenario.h"
#include "database.h"
#include "utils.h"

/* ================================================================== */
/*  MODE DE FONCTIONNEMENT                                             */
/* ================================================================== */
static int java_mode = 0;  /* 1 = lance par Java, protocole stdout */

/* ================================================================== */
/*  WARNING / EVENT LOG (in-memory, printed at end)                   */
/* ================================================================== */

#define MAX_WARNINGS 500

typedef struct {
    float time_sec;
    char  type[20];
    char  message[150];
} SimWarning;

static SimWarning warning_log[MAX_WARNINGS];
static int        warning_count = 0;

static void add_warning(float t, const char* type, const char* msg) {
    if (warning_count >= MAX_WARNINGS) return;
    warning_log[warning_count].time_sec = t;
    strncpy(warning_log[warning_count].type,    type, 19);
    strncpy(warning_log[warning_count].message, msg, 149);
    warning_log[warning_count].type[19]    = '\0';
    warning_log[warning_count].message[149]= '\0';
    warning_count++;
}

/* ================================================================== */
/*  SAFE INPUT                                                         */
/* ================================================================== */

int safe_input_int(const char* prompt, int min, int max, int default_value) {
    char buf[100];
    int  value;
    printf("%s", prompt);
    if (fgets(buf, sizeof(buf), stdin) != NULL) {
        if (sscanf(buf, "%d", &value) == 1 && value >= min && value <= max)
            return value;
    }
    printf("Invalid input. Using default: %d\n", default_value);
    return default_value;
}

/* ================================================================== */
/*  PERIODIC DB FLUSH HELPER                                           */
/* ================================================================== */

/**
 * Flushes the current ring-buffer content to the database.
 * Safe to call multiple times — SQLite REPLACE / INSERT handles duplicates
 * if you add a UNIQUE constraint, otherwise records accumulate (fine for
 * the last-200 approach because count stays bounded).
 */
static void flush_to_db(void) {
    int             record_count = 0;
    BlackBoxRecord* ordered      = get_ordered_records(&record_count);
    if (ordered != NULL && record_count > 0) {
        save_all_records_to_db(ordered, record_count);
        free(ordered);
    }
}

/* ================================================================== */
/*  MAIN                                                               */
/* ================================================================== */

int main(int argc, char* argv[]) {

    int       total_duration     = 380;
    CrashType crash_type         = CRASH_NONE;
    CrashInfo crash_info;
    int       crashed            = 0;
    float     crash_trigger_time = 0.0f;

    memset(&crash_info, 0, sizeof(crash_info));
    srand((unsigned int)time(NULL));

    /* ================================================================
     *  MODE JAVA : flight_sim.exe <duration> <crash_time> <crash_type>
     *  Si 3 arguments passes, on ne demande rien au clavier et on
     *  envoie le protocole DATA| sur stdout.
     * ================================================================ */
    if (argc >= 4) {
        java_mode          = 1;
        total_duration     = atoi(argv[1]);
        crash_trigger_time = (float)atoi(argv[2]);
        int ct             = atoi(argv[3]);

        if (total_duration < 10 || total_duration > 600) total_duration = 380;
        if (ct < 0 || ct > 7) ct = 0;

        if (ct == 0) {
            crash_type = get_random_crash();
            if (crash_type != CRASH_NONE) {
                float pct = 0.60f + ((float)rand() / (float)RAND_MAX) * 0.25f;
                crash_trigger_time = (float)total_duration * pct;
            }
        } else {
            crash_type = (CrashType)ct;
        }

        if (crash_type != CRASH_NONE)
            crash_info = get_crash_info(crash_type, crash_trigger_time);

        printf("INIT|duration=%d|crash_time=%.0f|crash_type=%s\n",
               total_duration, crash_trigger_time, get_crash_name(crash_type));
        fflush(stdout);

        /* Sauter directement a l'initialisation */
        goto sim_init;
    }

    /* ================================================================
     *  MODE INTERACTIF : pas d'arguments
     * ================================================================ */
    java_mode = 0;

    printf("\n");
    printf("================================================================================\n");
    printf("              AIRCRAFT FLIGHT CRASH SIMULATOR WITH BLACK BOX\n");
    printf("                        REAL TIME SIMULATION  (v3)\n");
    printf("================================================================================\n\n");

    /* -- SELECT CRASH SCENARIO -- */

    printf("SELECT CRASH SCENARIO\n");
    printf("---------------------\n");
    printf("  1. AERODYNAMIC STALL\n");
    printf("     Cause : Low airspeed at high angle of attack, possible icing or pilot error.\n");
    printf("     Effect: Aircraft loses lift and pitches down violently.\n\n");
    printf("  2. OVERSPEED / STRUCTURAL FAILURE\n");
    printf("     Cause : Uncontrolled descent or turbulence exceeds maximum operating speed.\n");
    printf("     Effect: Airframe overstress leads to structural breakup in mid-air.\n\n");
    printf("  3. UNCONTROLLED NOSE DIVE\n");
    printf("     Cause : Trim runaway, autopilot malfunction, or pilot spatial disorientation.\n");
    printf("     Effect: Aircraft pitches -45 deg and impacts ground at extreme speed.\n\n");
    printf("  4. SPIRAL DIVE\n");
    printf("     Cause : Spatial disorientation in IMC, instrument failure or distraction.\n");
    printf("     Effect: Increasing bank and descent rate - fastest altitude loss scenario.\n\n");
    printf("  5. ENGINE FAILURE\n");
    printf("     Cause : Fuel starvation, bird strike, mechanical failure or contamination.\n");
    printf("     Effect: Gradual loss of altitude, crash if no emergency landing available.\n\n");
    printf("  6. CFIT - Controlled Flight Into Terrain\n");
    printf("     Cause : Navigation error, wrong altimeter setting or GPWS warning ignored.\n");
    printf("     Effect: Aircraft flies normally but descends directly into terrain.\n\n");
    printf("  0. NO CRASH - Normal flight (380 seconds)\n\n");

    int crash_choice = safe_input_int("Enter scenario (0-6): ", 0, 6, 0);

    switch (crash_choice) {
    case 1:  crash_type = CRASH_STALL;          break;
    case 2:  crash_type = CRASH_OVERSPEED;       break;
    case 3:  crash_type = CRASH_NOSE_DIVE;       break;
    case 4:  crash_type = CRASH_SPIRAL;          break;
    case 5:  crash_type = CRASH_ENGINE_FAILURE;  break;
    case 6:  crash_type = CRASH_TERRAIN;         break;
    default: crash_type = CRASH_NONE;            break;
    }

    total_duration = 380;

    if (crash_type != CRASH_NONE) {
        float pct          = 0.60f + ((float)rand() / (float)RAND_MAX) * 0.25f;
        crash_trigger_time = (float)total_duration * pct;
        crash_info         = get_crash_info(crash_type, crash_trigger_time);

        printf("\n");
        printf("================================================================================\n");
        printf("  [DISPATCH] SCENARIO LOADED\n");
        printf("  Crash type        : %s\n", get_crash_name(crash_type));
        printf("  Scheduled trigger : T=%.0f s (%.1f min into flight)\n",
               crash_trigger_time, crash_trigger_time / 60.0f);
        printf("  Description       : %s\n", crash_info.description);
        printf("  Root cause        : %s\n", crash_info.cause);
        printf("================================================================================\n");
    } else {
        printf("\n[DISPATCH] No crash scheduled - normal flight (380 seconds).\n");
    }

    printf("\nPress ENTER to start the simulation...");
    getchar();

sim_init:
    /* -- INITIALIZE SYSTEMS -- */

    init_blackbox();
    init_database();
    reset_sensors();

    /* En mode Java, Java vide deja la DB dans db.connect() — le C ne touche pas a la DB */
    if (!java_mode) {
        printf("\nClear previous flight data from database? (1=Yes / 0=No, default=0): ");
        int do_clear = safe_input_int("", 0, 1, 0);
        if (do_clear) {
            clear_old_flight_data();
            printf("[DATABASE] Previous flight data cleared.\n");
        } else {
            printf("[DATABASE] Previous flight data kept.\n");
        }
    }
    /* En mode Java : pas de clear ici, Java s'en charge */

    set_flight_duration(total_duration);

    if (!java_mode) {
        printf("\nStarting simulation...\n");
        printf("================================================================================\n\n");
    }

    float       previous_altitude = 0.0f;
    clock_t     start_time        = clock();
    int         current_second    = 0;
    FlightPhase last_phase        = PHASE_ON_GROUND;
    int         last_flush_second = 0;   /* tracks when we last flushed */

    /* ================================================================
     *  REAL-TIME SIMULATION LOOP
     * ================================================================ */

    while (current_second < total_duration) {

        clock_t now          = clock();
        float   elapsed_secs = (float)(now - start_time) / CLOCKS_PER_SEC;
        int     time_elapsed = (int)elapsed_secs;

        if (time_elapsed >= total_duration) {
            current_second = total_duration;
            break;
        }

        if (time_elapsed > current_second) {
            current_second = time_elapsed;
            float t = (float)current_second;

            /* -- TRIGGER CRASH -- */
            if (!crashed && crash_type != CRASH_NONE && t >= crash_trigger_time) {
                crashed = 1;
                log_event(EVENT_CRASH, t);
                if (java_mode) {
                    crash_info = get_crash_info(crash_type, crash_trigger_time);
                    printf("CRASH|%s|%s|%s\n",
                           get_crash_name(crash_type),
                           crash_info.description,
                           crash_info.cause);
                    fflush(stdout);
                }
            }

            /* -- FLIGHT PHASE -- */
            FlightPhase phase = update_flight_phase(t, crashed);

            if (phase != last_phase) {
                if (java_mode) {
                    printf("PHASE|%s\n", get_phase_name(phase));
                    fflush(stdout);
                } else {
                    printf("\n--- [PHASE CHANGE] %s -> %s at T=%ds ---\n",
                           get_phase_name(last_phase), get_phase_name(phase), current_second);
                }
                last_phase = phase;
            }

            /* -- TARGET VALUES -- */
            float target_alt = get_target_altitude(phase, t);
            float target_spd = get_target_speed(phase, previous_altitude);
            float target_pit = get_target_pitch(phase);
            float target_rol = get_target_roll(phase);

            /* -- SENSOR VALUES -- */
            float altitude = generate_altitude(phase, target_alt, t,
                                               crashed, crash_type, crash_trigger_time);
            float speed    = generate_speed(phase, target_spd, crashed);
            float pitch    = generate_pitch(phase, target_pit, crashed,
                                            crash_type, t, crash_trigger_time);
            float roll     = generate_roll(phase, target_rol, crashed);

            /* -- TIMESTAMP -- */
            char timestamp[30];
            get_timestamp(timestamp, sizeof(timestamp));

            /* -- SENSOR ERROR CHECK -- */
            SensorData sensor;
            snprintf(sensor.timestamp, sizeof(sensor.timestamp), "%s", timestamp);
            sensor.altitude         = altitude;
            sensor.speed            = speed;
            sensor.pitch            = pitch;
            sensor.roll             = roll;
            sensor.has_error        = 0;
            sensor.error_message[0] = '\0';
            check_errors(&sensor);

            /* -- BUILD THE BLACKBOX RECORD -- */
            BlackBoxRecord record;
            snprintf(record.timestamp,     sizeof(record.timestamp),     "%s", timestamp);
            record.time_elapsed = t;
            record.altitude     = altitude;
            record.speed        = speed;
            record.pitch        = pitch;
            record.roll         = roll;
            record.has_error    = sensor.has_error;
            snprintf(record.error_message, sizeof(record.error_message), "%s", sensor.error_message);
            snprintf(record.crash_type,    sizeof(record.crash_type),    "%s", get_crash_name(crash_type));
            snprintf(record.phase,         sizeof(record.phase),         "%s", get_phase_name(phase));

            /* -- WARNINGS -- */
            char warn_buf[150];
            char combined_warn[150];
            combined_warn[0] = '\0';

            if (sensor.has_error) {
                snprintf(warn_buf, sizeof(warn_buf),
                         "[T=%4ds] WARNING Sensor out of range: %s",
                         current_second, sensor.error_message);
                add_warning(t, "WARNING", warn_buf);
                if (combined_warn[0] == '\0')
                    strncpy(combined_warn, sensor.error_message, sizeof(combined_warn) - 1);
            }

            if (speed > 175.0f) { /* 175 m/s = 340 kts = Vmo */
                snprintf(warn_buf, sizeof(warn_buf),
                         "[T=%4ds] OVERSPEED Speed=%.1f kts",
                         current_second, speed_to_knots(speed));
                log_event(EVENT_OVERSPEED, speed);
                add_warning(t, "EVENT", warn_buf);
                if (combined_warn[0] == '\0')
                    snprintf(combined_warn, sizeof(combined_warn), "OVERSPEED %.1f kts", speed_to_knots(speed));
                record.has_error = 1;
            }

            if (speed < 60.0f && altitude > 100.0f) {
                snprintf(warn_buf, sizeof(warn_buf),
                         "[T=%4ds] LOW SPEED / STALL RISK Speed=%.1f kts",
                         current_second, speed_to_knots(speed));
                log_event(EVENT_LOW_SPEED, speed);
                add_warning(t, "EVENT", warn_buf);
                if (combined_warn[0] == '\0')
                    snprintf(combined_warn, sizeof(combined_warn), "LOW SPEED %.1f kts", speed_to_knots(speed));
                record.has_error = 1;
            }

            if (pitch > 12.0f || pitch < -8.0f) {
                snprintf(warn_buf, sizeof(warn_buf),
                         "[T=%4ds] HIGH PITCH Pitch=%.1f deg",
                         current_second, pitch);
                log_event(EVENT_HIGH_PITCH, pitch);
                add_warning(t, "EVENT", warn_buf);
                if (combined_warn[0] == '\0')
                    snprintf(combined_warn, sizeof(combined_warn), "HIGH PITCH %.1f deg", pitch);
                record.has_error = 1;
            }

            if (roll > 30.0f || roll < -30.0f) {
                snprintf(warn_buf, sizeof(warn_buf),
                         "[T=%4ds] HIGH ROLL Roll=%.1f deg",
                         current_second, roll);
                log_event(EVENT_HIGH_ROLL, roll);
                add_warning(t, "EVENT", warn_buf);
                if (combined_warn[0] == '\0')
                    snprintf(combined_warn, sizeof(combined_warn), "HIGH ROLL %.1f deg", roll);
                record.has_error = 1;
            }

            if (previous_altitude - altitude > 80.0f && altitude > 0.0f) {
                snprintf(warn_buf, sizeof(warn_buf),
                         "[T=%4ds] RAPID DESCENT Drop=%.1f m in 1s",
                         current_second, previous_altitude - altitude);
                log_event(EVENT_RAPID_DESCENT, previous_altitude - altitude);
                add_warning(t, "EVENT", warn_buf);
                if (combined_warn[0] == '\0')
                    snprintf(combined_warn, sizeof(combined_warn),
                             "RAPID DESCENT %.1f m/s", previous_altitude - altitude);
                record.has_error = 1;
            }

            if (crashed && t >= crash_trigger_time && t < crash_trigger_time + 2.0f) {
                snprintf(warn_buf, sizeof(warn_buf),
                         "[T=%4ds] !!! CRITICAL FAILURE: %s !!!",
                         current_second, get_crash_name(crash_type));
                printf("\n%s\n\n", warn_buf);
                add_warning(t, "CRITICAL", warn_buf);
                if (combined_warn[0] == '\0')
                    snprintf(combined_warn, sizeof(combined_warn),
                             "CRITICAL FAILURE %s", get_crash_name(crash_type));
                record.has_error = 1;
            }

            if (combined_warn[0] != '\0')
                snprintf(record.error_message, sizeof(record.error_message), "%s", combined_warn);

            /* -- PUSH INTO RING BUFFER -- */
            record_to_blackbox(record);

            /* -- SORTIE PROTOCOLE JAVA (chaque seconde) -- */
            if (java_mode) {
                printf("DATA|%.0f|%s|%.1f|%.1f|%.1f|%.1f|%d|%s\n",
                       t,
                       get_phase_name(phase),
                       altitude, speed_to_knots(speed), pitch, roll,
                       crashed,
                       get_crash_name(crash_type));
                fflush(stdout);
            }

            /* -- PERIODIC DB FLUSH -- */
            if (current_second - last_flush_second >= 30) {
                flush_to_db();
                last_flush_second = current_second;
                if (!java_mode)
                    printf("[DATABASE] Periodic save at T=%ds\n", current_second);
            }

            /* -- AFFICHAGE CONSOLE (mode interactif uniquement) -- */
            if (!java_mode) {
                if (current_second % 5 == 0 || current_second == total_duration ||
                    (crashed && altitude < 100.0f) ||
                    (crashed && t >= crash_trigger_time && t < crash_trigger_time + 5.0f)) {

                    printf("[%4ds] %-12s | Alt:%8.1f m | Spd:%7.1f kts                "
                           " | Pit:%6.1f deg | Rol:%6.1f deg",
                           current_second, get_phase_name(phase),
                           altitude, speed_to_knots(speed), pitch, roll);

                    if (record.has_error)          printf(" | [WARNING]");
                    if (crashed && altitude < 100) printf(" | [IMPACT IMMINENT]");
                    printf("\n");
                }
            }

            previous_altitude = altitude;

            /* -- IMPACT CHECK -- */
            if (crashed && altitude <= 0.0f) {
                /* At impact: all parameters set to 0 */
                altitude = 0.0f;
                speed    = 0.0f;
                pitch    = 0.0f;
                roll     = 0.0f;

                /* Update the record with zeroed values */
                record.altitude = 0.0f;
                record.speed    = 0.0f;
                record.pitch    = 0.0f;
                record.roll     = 0.0f;
                record_to_blackbox(record);

                if (java_mode) {
                    /* Send one final DATA line with all zeros */
                    printf("DATA|%.0f|CRASH|0.0|0.0|0.0|0.0|1|%s\n",
                           t, get_crash_name(crash_type));
                    fflush(stdout);
                    printf("IMPACT|%.0f|0.0\n", t);
                    fflush(stdout);
                } else {
                    printf("\n");
                    printf("================================================================================\n");
                    printf("  >>> AIRCRAFT IMPACT at T=%.0f seconds <<<\n", t);
                    printf("  Alt: 0.0 m | Spd: 0.0 kts | Pit: 0.0 deg | Rol: 0.0 deg\n");
                    printf("  >>> Black box signal lost. Recovering data... <<<\n");
                    printf("================================================================================\n\n");
                }
                break;
            }
        }

        Sleep(10);
    }

    /* ================================================================
     *  END OF FLIGHT — final flush of the ring buffer to the database.
     * ================================================================ */
    {
        int             record_count = 0;
        BlackBoxRecord* ordered      = get_ordered_records(&record_count);

        if (ordered != NULL && record_count > 0) {
            printf("[DATABASE] Final save: %d records (buffer capacity: %d)...\n",
                   record_count, BLACKBOX_SIZE);
            save_all_records_to_db(ordered, record_count);
            free(ordered);
        }
    }

    /* -- BLACK BOX PRINTOUT -- */
    print_blackbox();

    /* -- CRASH INVESTIGATION -- */
    if (crashed) {
        print_pre_crash_analysis(crash_trigger_time);
    } else {
        printf("\n");
        printf("================================================================================\n");
        printf("                     FLIGHT COMPLETED SUCCESSFULLY\n");
        printf("================================================================================\n");
        printf("No crash occurred. Aircraft completed full flight profile safely.\n");
    }

    /* ================================================================
     *  END-OF-SIMULATION SUMMARY
     * ================================================================ */
    printf("\n");
    printf("################################################################################\n");
    printf("#                    END-OF-SIMULATION SUMMARY REPORT                        #\n");
    printf("################################################################################\n\n");

    printf("=== CRASH TYPE ===\n");
    if (crash_type == CRASH_NONE) {
        printf("  None — Normal flight completed.\n");
    } else {
        printf("  Type         : %s\n",   get_crash_name(crash_type));
        printf("  Triggered at : T=%.0f seconds\n", crash_trigger_time);
        printf("  Description  : %s\n",   crash_info.description);
        printf("  Cause        : %s\n",   crash_info.cause);
    }
    printf("\n");

    printf("=== WARNINGS & EVENTS DURING FLIGHT (%d total) ===\n\n", warning_count);
    if (warning_count == 0) {
        printf("  No warnings or events recorded.\n");
    } else {
        for (int i = 0; i < warning_count; i++) {
            printf("  [%7.1fs] [%-8s] %s\n",
                   warning_log[i].time_sec,
                   warning_log[i].type,
                   warning_log[i].message);
        }
    }
    printf("\n");

    printf("=== FLIGHT STATISTICS ===\n");
    printf("  Total records in black box  : %d / %d\n",
           get_blackbox_count(), BLACKBOX_SIZE);
    printf("  Records saved to database   : %d (last %d only)\n",
           get_blackbox_count() < BLACKBOX_SIZE ? get_blackbox_count() : BLACKBOX_SIZE,
           BLACKBOX_SIZE);
    printf("  Total warnings/events       : %d\n", warning_count);
    printf("  Crash occurred              : %s\n", crashed ? "YES" : "NO");
    if (crashed)
        printf("  Crash type                  : %s\n", get_crash_name(crash_type));

    printf("\n");
    printf("################################################################################\n\n");

    /* -- DATABASE SUMMARY -- */
    printf("================================================================================\n");
    printf("                        DATABASE SUMMARY\n");
    printf("================================================================================\n");
    printf("Total flights recorded in database: %d\n", get_flight_count());
    print_all_crash_summaries();

    close_database();

    if (java_mode) {
        printf("END\n");
        fflush(stdout);
    } else {
        printf("\n=== SIMULATION COMPLETE ===\n");
    }

    return 0;
}
