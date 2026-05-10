#include "event_handler.h"
#include <stdio.h>
#include <time.h>

static void get_time_str(char* buf, int size) {
    time_t     now = time(NULL);
    struct tm* t   = localtime(&now);
    if (t) strftime(buf, size, "%H:%M:%S", t);
    else   snprintf(buf, size, "00:00:00");
}

static void log_to_file(const char* line) {
    FILE* f = fopen("events.txt", "a");
    if (f) {
        fprintf(f, "%s\n", line);
        fclose(f);
    }
}

void log_event(EventType type, float value) {
    char ts[16];
    get_time_str(ts, sizeof(ts));

    char line[200];

    switch (type) {
    case EVENT_CRASH:
        snprintf(line, sizeof(line),
                 "[%s] CRASH TRIGGERED", ts);
        break;
    case EVENT_OVERSPEED:
        snprintf(line, sizeof(line),
                 "[%s] OVERSPEED  speed=%.1f m/s", ts, value);
        break;
    case EVENT_LOW_SPEED:
        snprintf(line, sizeof(line),
                 "[%s] LOW SPEED / STALL RISK  speed=%.1f m/s", ts, value);
        break;
    case EVENT_HIGH_PITCH:
        snprintf(line, sizeof(line),
                 "[%s] HIGH PITCH  pitch=%.1f deg", ts, value);
        break;
    case EVENT_HIGH_ROLL:
        snprintf(line, sizeof(line),
                 "[%s] HIGH ROLL   roll=%.1f deg", ts, value);
        break;
    case EVENT_RAPID_DESCENT:
        snprintf(line, sizeof(line),
                 "[%s] RAPID DESCENT  drop=%.1f m/s", ts, value);
        break;
    default:
        snprintf(line, sizeof(line),
                 "[%s] UNKNOWN EVENT  value=%.1f", ts, value);
        break;
    }

    log_to_file(line);
}
