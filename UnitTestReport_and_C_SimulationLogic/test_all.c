#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "flight_phase.h"
#include "crash_scenario.h"
#include "circular_buffer.h"
#include "sensors.h"
#include "error_handler.h"
#include "event_handler.h"
#include "utils.h"

/* ================================================================== */
/*  MINI TEST FRAMEWORK                                                */
/* ================================================================== */

static int tests_run    = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define ASSERT(cond, msg) do {                                        \
    tests_run++;                                                      \
    if (cond) {                                                       \
        tests_passed++;                                               \
        printf("  [PASS] %s\n", msg);                                \
    } else {                                                          \
        tests_failed++;                                               \
        printf("  [FAIL] %s  (line %d)\n", msg, __LINE__);          \
    }                                                                 \
} while(0)

#define ASSERT_FLOAT_EQ(a, b, eps, msg) \
    ASSERT(fabs((double)(a) - (double)(b)) < (double)(eps), msg)

#define ASSERT_FLOAT_RANGE(val, lo, hi, msg) \
    ASSERT((val) >= (lo) && (val) <= (hi), msg)

#define TEST_SECTION(name) \
    printf("\n+--------------------------------------+\n"); \
    printf("?  %-36s?\n", name);                           \
    printf("+--------------------------------------+\n");

/* ================================================================== */
/*  1. FLIGHT PHASE TESTS                                              */
/* ================================================================== */

static void test_flight_phase_names(void) {
    TEST_SECTION("Flight Phase ? Names");

    ASSERT(strcmp(get_phase_name(PHASE_ON_GROUND),  "ON GROUND")  == 0, "PHASE_ON_GROUND name");
    ASSERT(strcmp(get_phase_name(PHASE_TAKEOFF),     "TAKEOFF")    == 0, "PHASE_TAKEOFF name");
    ASSERT(strcmp(get_phase_name(PHASE_CLIMB),       "CLIMB")      == 0, "PHASE_CLIMB name");
    ASSERT(strcmp(get_phase_name(PHASE_CRUISE),      "CRUISE")     == 0, "PHASE_CRUISE name");
    ASSERT(strcmp(get_phase_name(PHASE_DESCENT),     "DESCENT")    == 0, "PHASE_DESCENT name");
    ASSERT(strcmp(get_phase_name(PHASE_LANDING),     "LANDING")    == 0, "PHASE_LANDING name");
    ASSERT(strcmp(get_phase_name(PHASE_CRASH),       "CRASH")      == 0, "PHASE_CRASH name");
    ASSERT(strcmp(get_phase_name(PHASE_POST_CRASH),  "POST-CRASH") == 0, "PHASE_POST_CRASH name");
}

static void test_flight_phase_transitions_380s(void) {
    TEST_SECTION("Flight Phase ? Transitions (380s flight)");

    set_flight_duration(380);

    /* t=0 ? ON_GROUND */
    ASSERT(update_flight_phase(0.0f, 0)  == PHASE_ON_GROUND, "t=0   is ON_GROUND");
    ASSERT(update_flight_phase(5.0f, 0)  == PHASE_ON_GROUND, "t=5   is ON_GROUND");

    /* t=16 ? TAKEOFF */
    ASSERT(update_flight_phase(16.0f, 0) == PHASE_TAKEOFF,   "t=16  is TAKEOFF");

    /* t=40 ? CLIMB */
    ASSERT(update_flight_phase(40.0f, 0) == PHASE_CLIMB,     "t=40  is CLIMB");

    /* t=150 ? CRUISE */
    ASSERT(update_flight_phase(150.0f, 0) == PHASE_CRUISE,   "t=150 is CRUISE");

    /* t=290 ? DESCENT */
    ASSERT(update_flight_phase(290.0f, 0) == PHASE_DESCENT,  "t=290 is DESCENT");

    /* t=360 ? LANDING */
    ASSERT(update_flight_phase(360.0f, 0) == PHASE_LANDING,  "t=360 is LANDING");

    /* crashed=1 ? always CRASH */
    ASSERT(update_flight_phase(100.0f, 1) == PHASE_CRASH,    "crashed=1 ? CRASH");
    ASSERT(update_flight_phase(0.0f,   1) == PHASE_CRASH,    "crashed=1 at t=0 ? CRASH");
}

static void test_flight_phase_transitions_200s(void) {
    TEST_SECTION("Flight Phase ? Transitions (200s flight)");

    set_flight_duration(200);

    ASSERT(update_flight_phase(0.0f,  0) == PHASE_ON_GROUND, "200s: t=0   ON_GROUND");
    ASSERT(update_flight_phase(10.0f, 0) == PHASE_TAKEOFF,   "200s: t=10  TAKEOFF");
    ASSERT(update_flight_phase(25.0f, 0) == PHASE_CLIMB,     "200s: t=25  CLIMB");
    ASSERT(update_flight_phase(80.0f, 0) == PHASE_CRUISE,    "200s: t=80  CRUISE");
    ASSERT(update_flight_phase(160.0f,0) == PHASE_DESCENT,   "200s: t=160 DESCENT");
    ASSERT(update_flight_phase(185.0f,0) == PHASE_LANDING,   "200s: t=185 LANDING");

    set_flight_duration(380); /* restore default */
}

static void test_target_altitude(void) {
    TEST_SECTION("Flight Phase ? Target Altitude");

    set_flight_duration(380);

    ASSERT_FLOAT_EQ(get_target_altitude(PHASE_ON_GROUND, 0.0f),  0.0f,   0.1f,
                    "ON_GROUND altitude = 0");
    ASSERT_FLOAT_RANGE(get_target_altitude(PHASE_CRUISE, 150.0f),
                    8990.0f, 9010.0f,
                    "CRUISE altitude ~9000m");
    ASSERT(get_target_altitude(PHASE_TAKEOFF, 20.0f) > 0.0f,
           "TAKEOFF altitude > 0");
    ASSERT(get_target_altitude(PHASE_CLIMB, 80.0f) > 500.0f,
           "CLIMB altitude > 500m");
    ASSERT(get_target_altitude(PHASE_DESCENT, 300.0f) < 9000.0f,
           "DESCENT altitude < 9000m");
    ASSERT(get_target_altitude(PHASE_LANDING, 370.0f) < 500.0f,
           "LANDING altitude < 500m");
}

static void test_target_speed(void) {
    TEST_SECTION("Flight Phase ? Target Speed");

    ASSERT_FLOAT_EQ(get_target_speed(PHASE_ON_GROUND, 0.0f), 0.0f, 0.1f,
                    "ON_GROUND speed = 0");
    ASSERT_FLOAT_EQ(get_target_speed(PHASE_TAKEOFF, 0.0f),  75.0f, 0.1f,
                    "TAKEOFF speed = 75 m/s");
    ASSERT_FLOAT_EQ(get_target_speed(PHASE_CRUISE, 9000.0f), 232.0f, 0.1f,
                    "CRUISE speed = 232 m/s (451 kts)");
    ASSERT(get_target_speed(PHASE_CLIMB, 5000.0f) > 75.0f,
           "CLIMB speed > 75 m/s");
    ASSERT(get_target_speed(PHASE_CLIMB, 5000.0f) <= 220.0f,
           "CLIMB speed <= 220 m/s");
}

static void test_target_pitch_roll(void) {
    TEST_SECTION("Flight Phase ? Target Pitch & Roll");

    ASSERT_FLOAT_EQ(get_target_pitch(PHASE_ON_GROUND),  0.0f,  0.1f, "ON_GROUND pitch=0");
    ASSERT_FLOAT_EQ(get_target_pitch(PHASE_TAKEOFF),    12.0f, 0.1f, "TAKEOFF pitch=12");
    ASSERT_FLOAT_EQ(get_target_pitch(PHASE_CLIMB),       8.0f, 0.1f, "CLIMB pitch=8");
    ASSERT_FLOAT_EQ(get_target_pitch(PHASE_CRUISE),      2.0f, 0.1f, "CRUISE pitch=2");
    ASSERT_FLOAT_EQ(get_target_pitch(PHASE_DESCENT),    -3.0f, 0.1f, "DESCENT pitch=-3");
    ASSERT_FLOAT_EQ(get_target_pitch(PHASE_LANDING),     5.0f, 0.1f, "LANDING pitch=5");

    ASSERT_FLOAT_EQ(get_target_roll(PHASE_TAKEOFF),  0.0f, 0.1f, "TAKEOFF roll=0");
    ASSERT_FLOAT_EQ(get_target_roll(PHASE_LANDING),  0.0f, 0.1f, "LANDING roll=0");
    ASSERT_FLOAT_EQ(get_target_roll(PHASE_CRUISE),   2.0f, 0.1f, "CRUISE roll=2");
}

/* ================================================================== */
/*  2. CRASH SCENARIO TESTS                                            */
/* ================================================================== */

static void test_crash_names(void) {
    TEST_SECTION("Crash Scenario ? Names");

    ASSERT(strcmp(get_crash_name(CRASH_NONE),           "NORMAL FLIGHT")                  == 0,
           "CRASH_NONE name");
    ASSERT(strcmp(get_crash_name(CRASH_STALL),          "AERODYNAMIC STALL")              == 0,
           "CRASH_STALL name");
    ASSERT(strcmp(get_crash_name(CRASH_OVERSPEED),      "OVERSPEED STRUCTURAL FAILURE")   == 0,
           "CRASH_OVERSPEED name");
    ASSERT(strcmp(get_crash_name(CRASH_NOSE_DIVE),      "UNCONTROLLED NOSE DIVE")         == 0,
           "CRASH_NOSE_DIVE name");
    ASSERT(strcmp(get_crash_name(CRASH_SPIRAL),         "SPIRAL DIVE")                    == 0,
           "CRASH_SPIRAL name");
    ASSERT(strcmp(get_crash_name(CRASH_ENGINE_FAILURE), "ENGINE FAILURE")                 == 0,
           "CRASH_ENGINE_FAILURE name");
    ASSERT(strcmp(get_crash_name(CRASH_TERRAIN),        "CFIT (Controlled Flight Into Terrain)") == 0,
           "CRASH_TERRAIN name");
}

static void test_crash_info(void) {
    TEST_SECTION("Crash Scenario ? CrashInfo");

    CrashInfo info = get_crash_info(CRASH_STALL, 200.0f);
    ASSERT(info.type == CRASH_STALL,        "CrashInfo.type = STALL");
    ASSERT_FLOAT_EQ(info.trigger_time, 200.0f, 0.1f, "CrashInfo.trigger_time = 200");
    ASSERT_FLOAT_EQ(info.impact_time,  230.0f, 0.1f, "CrashInfo.impact_time = trigger+30");
    ASSERT(strlen(info.description) > 0,    "CrashInfo.description not empty");
    ASSERT(strlen(info.cause) > 0,          "CrashInfo.cause not empty");

    CrashInfo info2 = get_crash_info(CRASH_NONE, 0.0f);
    ASSERT(info2.type == CRASH_NONE,        "CrashInfo NONE type");
}

static void test_crash_altitude_effects(void) {
    TEST_SECTION("Crash Scenario ? Altitude Effects");

    float alt = 5000.0f;

    /* Each crash should reduce altitude over time */
    float stall    = apply_crash_altitude(CRASH_STALL,          10.0f, 0.0f, alt);
    float overspd  = apply_crash_altitude(CRASH_OVERSPEED,      10.0f, 0.0f, alt);
    float nosedive = apply_crash_altitude(CRASH_NOSE_DIVE,      10.0f, 0.0f, alt);
    float spiral   = apply_crash_altitude(CRASH_SPIRAL,         10.0f, 0.0f, alt);
    float engine   = apply_crash_altitude(CRASH_ENGINE_FAILURE, 10.0f, 0.0f, alt);
    float terrain  = apply_crash_altitude(CRASH_TERRAIN,        10.0f, 0.0f, alt);

    ASSERT(stall   < alt, "STALL reduces altitude");
    ASSERT(overspd < alt, "OVERSPEED reduces altitude");
    ASSERT(nosedive< alt, "NOSE_DIVE reduces altitude");
    ASSERT(spiral  < alt, "SPIRAL reduces altitude");
    ASSERT(engine  < alt, "ENGINE_FAILURE reduces altitude");
    ASSERT(terrain < alt, "TERRAIN reduces altitude");

    /* All must be >= 0 */
    ASSERT(stall   >= 0.0f, "STALL altitude >= 0");
    ASSERT(overspd >= 0.0f, "OVERSPEED altitude >= 0");

    /* Faster crashes lose altitude faster */
    ASSERT(terrain < engine, "TERRAIN faster than ENGINE_FAILURE");
    ASSERT(spiral  < stall,  "SPIRAL faster than STALL");

    /* t=0 (trigger moment) ? no change yet */
    float no_change = apply_crash_altitude(CRASH_STALL, 0.0f, 0.0f, alt);
    ASSERT_FLOAT_EQ(no_change, alt, 0.1f, "At trigger moment, altitude unchanged");
}

static void test_crash_pitch_effects(void) {
    TEST_SECTION("Crash Scenario ? Pitch Effects");

    /* Stall: starts positive then goes negative */
    float pitch_stall_early = apply_crash_pitch(CRASH_STALL, 0.0f, 0.0f);
    float pitch_stall_late  = apply_crash_pitch(CRASH_STALL, 5.0f, 0.0f);
    ASSERT(pitch_stall_late < pitch_stall_early, "STALL pitch decreases over time");

    /* Nose dive: fixed -45 */
    float pitch_nd = apply_crash_pitch(CRASH_NOSE_DIVE, 5.0f, 0.0f);
    ASSERT_FLOAT_EQ(pitch_nd, -45.0f, 0.1f, "NOSE_DIVE pitch = -45 deg");

    /* Engine failure: slight negative pitch */
    float pitch_eng = apply_crash_pitch(CRASH_ENGINE_FAILURE, 5.0f, 0.0f);
    ASSERT_FLOAT_EQ(pitch_eng, -5.0f, 0.1f, "ENGINE_FAILURE pitch = -5 deg");
}

static void test_random_crash(void) {
    TEST_SECTION("Crash Scenario ? Random Crash");

    srand(42);
    int none_count  = 0;
    int crash_count = 0;
    for (int i = 0; i < 100; i++) {
        CrashType ct = get_random_crash();
        ASSERT(ct >= CRASH_NONE && ct <= CRASH_TERRAIN, "Random crash type in valid range");
        if (ct == CRASH_NONE) none_count++;
        else                  crash_count++;
    }
    /* With 50% crash probability, both should occur in 100 trials */
    ASSERT(none_count  > 0, "Some runs have no crash");
    ASSERT(crash_count > 0, "Some runs have a crash");
    printf("  [INFO] In 100 trials: %d normal, %d crashes\n", none_count, crash_count);
}

/* ================================================================== */
/*  3. CIRCULAR BUFFER TESTS                                           */
/* ================================================================== */

static BlackBoxRecord make_record(int id, float time, float alt, float spd) {
    BlackBoxRecord r;
    memset(&r, 0, sizeof(r));
    r.id           = id;
    r.time_elapsed = time;
    r.altitude     = alt;
    r.speed        = spd;
    r.pitch        = 2.0f;
    r.roll         = 1.0f;
    r.has_error    = 0;
    snprintf(r.phase,      sizeof(r.phase),      "CRUISE");
    snprintf(r.crash_type, sizeof(r.crash_type), "NORMAL FLIGHT");
    snprintf(r.timestamp,  sizeof(r.timestamp),  "12:00:00");
    return r;
}

static void test_circular_buffer_basic(void) {
    TEST_SECTION("Circular Buffer ? Basic Operations");

    init_blackbox();
    ASSERT(get_blackbox_count() == 0, "Empty after init");

    /* Add 5 records */
    for (int i = 0; i < 5; i++) {
        BlackBoxRecord r = make_record(i, (float)i, 5000.0f + i, 200.0f);
        record_to_blackbox(r);
    }
    ASSERT(get_blackbox_count() == 5, "Count = 5 after 5 inserts");

    /* Get ordered */
    int n = 0;
    BlackBoxRecord* ordered = get_ordered_records(&n);
    ASSERT(n == 5, "get_ordered_records returns 5");
    ASSERT(ordered != NULL, "get_ordered_records not NULL");
    ASSERT_FLOAT_EQ(ordered[0].time_elapsed, 0.0f, 0.1f, "First record time = 0");
    ASSERT_FLOAT_EQ(ordered[4].time_elapsed, 4.0f, 0.1f, "Last record time = 4");
    free(ordered);
}

static void test_circular_buffer_overflow(void) {
    TEST_SECTION("Circular Buffer ? Overflow (wraps at 200)");

    init_blackbox();

    /* Insert 210 records ? only last 200 should remain */
    for (int i = 0; i < 210; i++) {
        BlackBoxRecord r = make_record(i, (float)i, 5000.0f, 200.0f);
        record_to_blackbox(r);
    }

    ASSERT(get_blackbox_count() == BLACKBOX_SIZE,
           "Count capped at BLACKBOX_SIZE (200)");

    int n = 0;
    BlackBoxRecord* ordered = get_ordered_records(&n);
    ASSERT(n == BLACKBOX_SIZE, "Ordered count = 200");

    /* First record should be record #10 (oldest kept = 210-200=10) */
    ASSERT_FLOAT_EQ(ordered[0].time_elapsed, 10.0f, 0.1f,
                    "Oldest remaining = record 10");
    ASSERT_FLOAT_EQ(ordered[199].time_elapsed, 209.0f, 0.1f,
                    "Newest remaining = record 209");
    free(ordered);
}

static void test_circular_buffer_chronological(void) {
    TEST_SECTION("Circular Buffer ? Chronological Order");

    init_blackbox();

    for (int i = 0; i < 50; i++) {
        BlackBoxRecord r = make_record(i, (float)i * 2.0f, 1000.0f * i, 100.0f);
        record_to_blackbox(r);
    }

    int n = 0;
    BlackBoxRecord* ordered = get_ordered_records(&n);
    ASSERT(n == 50, "50 records returned");

    int in_order = 1;
    for (int i = 1; i < n; i++) {
        if (ordered[i].time_elapsed < ordered[i-1].time_elapsed) {
            in_order = 0; break;
        }
    }
    ASSERT(in_order, "Records are in chronological order");
    free(ordered);
}

static void test_circular_buffer_reinit(void) {
    TEST_SECTION("Circular Buffer ? Re-initialisation");

    init_blackbox();
    for (int i = 0; i < 20; i++) {
        BlackBoxRecord r = make_record(i, (float)i, 5000.0f, 200.0f);
        record_to_blackbox(r);
    }
    ASSERT(get_blackbox_count() == 20, "20 records before reinit");

    init_blackbox();
    ASSERT(get_blackbox_count() == 0, "0 records after reinit");

    int n = 0;
    BlackBoxRecord* ordered = get_ordered_records(&n);
    ASSERT(n == 0 && ordered == NULL, "get_ordered_records returns NULL when empty");
}

/* ================================================================== */
/*  4. SENSOR TESTS                                                    */
/* ================================================================== */

static void test_altitude_sensor(void) {
    TEST_SECTION("Altitude Sensor ? Normal Flight");

    srand(42);
    set_flight_duration(380);
    reset_altitude();

    /* ON_GROUND must return 0 */
    float alt = generate_altitude(PHASE_ON_GROUND, 0.0f, 0.0f,
                                  0, CRASH_NONE, 0.0f);
    ASSERT_FLOAT_EQ(alt, 0.0f, 0.1f, "ON_GROUND altitude = 0");

    /* CRUISE: should be close to 9000 */
    reset_altitude();
    for (int i = 0; i < 10; i++) {
        alt = generate_altitude(PHASE_CRUISE, 9000.0f, 150.0f + i,
                                0, CRASH_NONE, 0.0f);
    }
    ASSERT_FLOAT_RANGE(alt, 8900.0f, 9100.0f, "CRUISE altitude near 9000m");

    /* Altitude always within [0, 10000] */
    ASSERT(alt >= 0.0f && alt <= 10000.0f, "Altitude clamped to [0,10000]");
}

static void test_altitude_sensor_crash(void) {
    TEST_SECTION("Altitude Sensor ? Crash Effects");

    srand(42);
    reset_altitude();

    /* Simulate crash: altitude should fall */
    float alt_before = 8000.0f;
    float alt = alt_before;

    for (int i = 1; i <= 5; i++) {
        alt = generate_altitude(PHASE_CRASH, 8000.0f,
                                (float)(200 + i), 1,
                                CRASH_NOSE_DIVE, 200.0f);
    }
    ASSERT(alt < alt_before, "Altitude decreases during crash");
    ASSERT(alt >= 0.0f, "Altitude never negative during crash");
}

static void test_speed_sensor(void) {
    TEST_SECTION("Speed Sensor ? Normal Flight");

    srand(42);
    reset_speed();

    /* ON_GROUND: speed = 0
     * NOTE: generate_speed converges toward target (0) with acceleration=8.
     * After reset, internal state=0, so first call returns 0+noise.
     * We allow a wider tolerance since noise can be up to 2.5 m/s. */
    float spd = generate_speed(PHASE_ON_GROUND, 0.0f, 0);
    ASSERT_FLOAT_EQ(spd, 0.0f, 3.0f, "ON_GROUND speed = 0");

    /* CRUISE: should reach 220 m/s */
    reset_speed();
    float s = 0.0f;
    for (int i = 0; i < 30; i++) {
        s = generate_speed(PHASE_CRUISE, 232.0f, 0);
    }
    ASSERT_FLOAT_RANGE(s, 227.0f, 237.0f, "After 30 ticks CRUISE speed ~232 m/s (451 kts)");

    /* Crashed: speed decreases over multiple ticks */
    reset_speed();
    for (int i = 0; i < 5; i++) generate_speed(PHASE_CRUISE, 220.0f, 0);
    float spd_before = generate_speed(PHASE_CRUISE, 220.0f, 0);
    float spd_crash = spd_before;
    for (int i = 0; i < 10; i++)
        spd_crash = generate_speed(PHASE_CRASH, 220.0f, 1);
    ASSERT(spd_crash < spd_before, "Speed decreases when crashed");
    ASSERT(spd_crash >= 0.0f, "Speed never negative when crashed");
}

static void test_speed_to_knots(void) {
    TEST_SECTION("Speed Sensor ? Unit Conversion");

    ASSERT_FLOAT_EQ(speed_to_knots(0.0f),    0.0f,   0.01f, "0 m/s = 0 kts");
    ASSERT_FLOAT_EQ(speed_to_knots(1.0f),    1.9438f, 0.01f, "1 m/s = 1.944 kts");
    ASSERT_FLOAT_EQ(speed_to_knots(220.0f),  427.64f, 0.5f,  "220 m/s = ~427 kts");
    ASSERT_FLOAT_EQ(speed_to_knots(100.0f),  194.38f, 0.5f,  "100 m/s = ~194 kts");
}

static void test_pitch_sensor(void) {
    TEST_SECTION("Pitch Sensor ? Normal Flight");

    srand(42);
    reset_pitch();

    /* After several ticks at takeoff target (12 deg), should converge */
    float p = 0.0f;
    for (int i = 0; i < 10; i++) {
        p = generate_pitch(PHASE_TAKEOFF, 12.0f, 0, CRASH_NONE, (float)i, 0.0f);
    }
    ASSERT_FLOAT_RANGE(p, -10.1f, 15.1f, "Pitch within sensor limits [-10,15]");

    /* Pitch during crash: should match crash profile */
    reset_pitch();
    float p_crash = generate_pitch(PHASE_CRASH, 0.0f, 1,
                                   CRASH_NOSE_DIVE, 205.0f, 200.0f);
    ASSERT_FLOAT_EQ(p_crash, -45.0f, 0.1f, "NOSE_DIVE crash pitch = -45 deg");
}

static void test_roll_sensor(void) {
    TEST_SECTION("Roll Sensor ? Normal Flight");

    srand(42);
    reset_roll();

    /* TAKEOFF: target roll = 0, should stay near 0 */
    float r = 0.0f;
    for (int i = 0; i < 10; i++) {
        r = generate_roll(PHASE_TAKEOFF, 0.0f, 0);
    }
    ASSERT_FLOAT_RANGE(r, -2.0f, 2.0f, "TAKEOFF roll near 0");

    /* Normal roll within [-35, 35] */
    reset_roll();
    int all_in_range = 1;
    for (int i = 0; i < 50; i++) {
        float rv = generate_roll(PHASE_CRUISE, 2.0f, 0);
        if (rv < -35.0f || rv > 35.0f) { all_in_range = 0; break; }
    }
    ASSERT(all_in_range, "Normal roll always in [-35, 35]");

    /* Crashed: roll can exceed normal limits */
    reset_roll();
    for (int i = 0; i < 5; i++) generate_roll(PHASE_CRUISE, 2.0f, 0);
    int crash_expanded = 0;
    for (int i = 0; i < 200; i++) {
        float rv = generate_roll(PHASE_CRASH, 2.0f, 1);
        if (rv > 35.0f || rv < -35.0f) { crash_expanded = 1; break; }
    }
    ASSERT(crash_expanded, "Crash roll can exceed normal +-35 limits");
}

/* ================================================================== */
/*  5. ERROR HANDLER TESTS                                             */
/* ================================================================== */

static void test_error_handler_valid(void) {
    TEST_SECTION("Error Handler ? Valid Sensor Values");

    SensorData s;
    snprintf(s.timestamp, sizeof(s.timestamp), "12:00:00");
    s.altitude = 5000.0f;
    s.speed    = 200.0f;
    s.pitch    = 5.0f;
    s.roll     = 10.0f;
    s.has_error = 0;
    s.error_message[0] = '\0';

    check_errors(&s);
    ASSERT(s.has_error == 0, "No error for valid sensor values");
    ASSERT(s.error_message[0] == '\0', "No error message for valid values");
}

static void test_error_handler_altitude(void) {
    TEST_SECTION("Error Handler ? Altitude Out of Range");

    SensorData s;
    snprintf(s.timestamp, sizeof(s.timestamp), "12:00:00");
    s.altitude = 12000.0f; /* above 10000 */
    s.speed    = 200.0f;
    s.pitch    = 5.0f;
    s.roll     = 10.0f;
    s.has_error = 0;
    s.error_message[0] = '\0';

    check_errors(&s);
    ASSERT(s.has_error == 1, "Error flag set for altitude > 10000");
    ASSERT(strlen(s.error_message) > 0, "Error message set for altitude > 10000");
}

static void test_error_handler_speed(void) {
    TEST_SECTION("Error Handler ? Speed Out of Range");

    SensorData s;
    snprintf(s.timestamp, sizeof(s.timestamp), "12:00:00");
    s.altitude = 5000.0f;
    s.speed    = 350.0f; /* above 300 */
    s.pitch    = 5.0f;
    s.roll     = 10.0f;
    s.has_error = 0;
    s.error_message[0] = '\0';

    check_errors(&s);
    ASSERT(s.has_error == 1, "Error flag set for speed > 300");
}

static void test_error_handler_pitch(void) {
    TEST_SECTION("Error Handler ? Pitch Out of Range");

    SensorData s;
    snprintf(s.timestamp, sizeof(s.timestamp), "12:00:00");
    s.altitude  = 5000.0f;
    s.speed     = 200.0f;
    s.pitch     = 60.0f; /* above 45 */
    s.roll      = 10.0f;
    s.has_error = 0;
    s.error_message[0] = '\0';

    check_errors(&s);
    ASSERT(s.has_error == 1, "Error flag set for pitch > 45");

    /* Negative pitch too extreme */
    s.pitch     = -50.0f;
    s.has_error = 0;
    s.error_message[0] = '\0';
    check_errors(&s);
    ASSERT(s.has_error == 1, "Error flag set for pitch < -45");
}

static void test_error_handler_roll(void) {
    TEST_SECTION("Error Handler ? Roll Out of Range");

    SensorData s;
    snprintf(s.timestamp, sizeof(s.timestamp), "12:00:00");
    s.altitude  = 5000.0f;
    s.speed     = 200.0f;
    s.pitch     = 5.0f;
    s.roll      = 95.0f; /* above 90 */
    s.has_error = 0;
    s.error_message[0] = '\0';

    check_errors(&s);
    ASSERT(s.has_error == 1, "Error flag set for roll > 90");
}

static void test_error_handler_boundaries(void) {
    TEST_SECTION("Error Handler ? Boundary Values");

    SensorData s;
    snprintf(s.timestamp, sizeof(s.timestamp), "12:00:00");

    /* Exact boundary values ? should NOT trigger error */
    s.altitude  = 10000.0f;
    s.speed     = 300.0f;
    s.pitch     = 45.0f;
    s.roll      = 90.0f;
    s.has_error = 0;
    s.error_message[0] = '\0';
    check_errors(&s);
    ASSERT(s.has_error == 0, "Exact max boundary values ? no error");

    s.altitude  = 0.0f;
    s.speed     = 0.0f;
    s.pitch     = -45.0f;
    s.roll      = -90.0f;
    s.has_error = 0;
    s.error_message[0] = '\0';
    check_errors(&s);
    ASSERT(s.has_error == 0, "Exact min boundary values ? no error");
}

/* ================================================================== */
/*  6. UTILS TESTS                                                     */
/* ================================================================== */

static void test_get_timestamp(void) {
    TEST_SECTION("Utils ? get_timestamp");

    char buf[30];
    get_timestamp(buf, sizeof(buf));

    /* Format must be HH:MM:SS (8 chars + null) */
    ASSERT(strlen(buf) == 8, "Timestamp length = 8");
    ASSERT(buf[2] == ':', "Timestamp has ':' at position 2");
    ASSERT(buf[5] == ':', "Timestamp has ':' at position 5");

    /* Each part must be numeric */
    int h1 = buf[0] - '0', h2 = buf[1] - '0';
    int m1 = buf[3] - '0', m2 = buf[4] - '0';
    int s1 = buf[6] - '0', s2 = buf[7] - '0';

    ASSERT(h1 >= 0 && h1 <= 9, "Hour digit 1 is numeric");
    ASSERT(h2 >= 0 && h2 <= 9, "Hour digit 2 is numeric");
    ASSERT(m1 >= 0 && m1 <= 5, "Minute tens digit is 0-5");
    ASSERT(m2 >= 0 && m2 <= 9, "Minute units digit is numeric");
    ASSERT(s1 >= 0 && s1 <= 5, "Second tens digit is 0-5");
    ASSERT(s2 >= 0 && s2 <= 9, "Second units digit is numeric");

    printf("  [INFO] Current timestamp: %s\n", buf);
}

static void test_reset_sensors(void) {
    TEST_SECTION("Utils ? reset_sensors");

    srand(42);
    set_flight_duration(380);

    /* Generate some values to change internal state */
    for (int i = 0; i < 20; i++) {
        generate_altitude(PHASE_CRUISE, 9000.0f, (float)i, 0, CRASH_NONE, 0.0f);
        generate_speed(PHASE_CRUISE, 220.0f, 0);
        generate_pitch(PHASE_CRUISE, 2.0f, 0, CRASH_NONE, (float)i, 0.0f);
        generate_roll(PHASE_CRUISE, 2.0f, 0);
    }

    /* After reset, ON_GROUND should give 0 for altitude */
    reset_sensors();
    float alt = generate_altitude(PHASE_ON_GROUND, 0.0f, 0.0f, 0, CRASH_NONE, 0.0f);
    ASSERT_FLOAT_EQ(alt, 0.0f, 0.1f, "After reset: ON_GROUND altitude = 0");

    float spd = generate_speed(PHASE_ON_GROUND, 0.0f, 0);
    ASSERT_FLOAT_EQ(spd, 0.0f, 0.5f, "After reset: ON_GROUND speed = 0");
}

/* ================================================================== */
/*  MAIN ? Run all tests                                               */
/* ================================================================== */

int main(void) {
    srand((unsigned int)time(NULL));

    printf("\n");
    printf("+------------------------------------------------------+\n");
    printf("?   AIRCRAFT FLIGHT CRASH SIMULATOR ? UNIT TESTS      ?\n");
    printf("+------------------------------------------------------+\n");

    /* Flight Phase */
    test_flight_phase_names();
    test_flight_phase_transitions_380s();
    test_flight_phase_transitions_200s();
    test_target_altitude();
    test_target_speed();
    test_target_pitch_roll();

    /* Crash Scenario */
    test_crash_names();
    test_crash_info();
    test_crash_altitude_effects();
    test_crash_pitch_effects();
    test_random_crash();

    /* Circular Buffer */
    test_circular_buffer_basic();
    test_circular_buffer_overflow();
    test_circular_buffer_chronological();
    test_circular_buffer_reinit();

    /* Sensors */
    test_altitude_sensor();
    test_altitude_sensor_crash();
    test_speed_sensor();
    test_speed_to_knots();
    test_pitch_sensor();
    test_roll_sensor();

    /* Error Handler */
    test_error_handler_valid();
    test_error_handler_altitude();
    test_error_handler_speed();
    test_error_handler_pitch();
    test_error_handler_roll();
    test_error_handler_boundaries();

    /* Utils */
    test_get_timestamp();
    test_reset_sensors();

    /* -- FINAL SUMMARY -- */
    printf("\n");
    printf("+------------------------------------------------------+\n");
    printf("?                   TEST SUMMARY                      ?\n");
    printf("?------------------------------------------------------?\n");
    printf("?  Total   : %-3d                                      ?\n", tests_run);
    printf("?  Passed  : %-3d  ?                                   ?\n", tests_passed);
    printf("?  Failed  : %-3d  ?                                   ?\n", tests_failed);
    printf("+------------------------------------------------------+\n\n");

    return (tests_failed == 0) ? 0 : 1;
}
