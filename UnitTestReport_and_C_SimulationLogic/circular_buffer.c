#include "circular_buffer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static BlackBoxRecord buffer[BLACKBOX_SIZE];
static int  head    = 0;
static int  count   = 0;
static int  next_id = 1;

void init_blackbox(void) {
    memset(buffer, 0, sizeof(buffer));
    head = count = 0; next_id = 1;
    printf("[BLACKBOX] Initialized (capacity: %d)\n", BLACKBOX_SIZE);
}

void record_to_blackbox(BlackBoxRecord rec) {
    rec.id        = next_id++;
    buffer[head]  = rec;
    head          = (head + 1) % BLACKBOX_SIZE;
    if (count < BLACKBOX_SIZE) count++;
}

int get_blackbox_count(void) { return count; }

BlackBoxRecord* get_all_records(int* out) { *out = count; return buffer; }

/* Returns malloc'd chronological copy - caller must free */
BlackBoxRecord* get_ordered_records(int* out) {
    *out = count;
    if (count == 0) return NULL;
    BlackBoxRecord* ordered = malloc(count * sizeof(BlackBoxRecord));
    if (!ordered) { *out = 0; return NULL; }
    int start = (count < BLACKBOX_SIZE) ? 0 : head;
    for (int i = 0; i < count; i++)
        ordered[i] = buffer[(start + i) % BLACKBOX_SIZE];
    return ordered;
}

void print_blackbox(void) {
    int n; BlackBoxRecord* ordered = get_ordered_records(&n);
    printf("\n=== BLACK BOX  (%d records) ===\n", n);
    printf(" ID  | Time | Alt      | Spd     | Pit   | Rol   | Phase\n");
    printf("-----|------|----------|---------|-------|-------|------\n");
    for (int i = 0; i < n; i++) {
        printf("[%3d] %4.0fs | %7.1fm | %6.1f  | %5.1f | %5.1f | %s\n",
               ordered[i].id, ordered[i].time_elapsed,
               ordered[i].altitude, ordered[i].speed,
               ordered[i].pitch,    ordered[i].roll,
               ordered[i].phase);
    }
    free(ordered);
    printf("=== END BLACK BOX ===\n\n");
}

void print_pre_crash_analysis(float crash_time) {
    int n; BlackBoxRecord* ordered = get_ordered_records(&n);
    if (!ordered) return;

    printf("\n### CRASH INVESTIGATION REPORT ###\n\n");
    printf("Last 30s before crash:\n");
    printf(" Time | Alt      | Spd     | Pit   | Rol\n");
    printf("------|----------|---------|-------|------\n");

    for (int i = 0; i < n; i++) {
        if (ordered[i].time_elapsed >= crash_time - 30 &&
            ordered[i].time_elapsed <= crash_time)
            printf(" %4.0fs | %7.1fm | %6.1f  | %5.1f | %5.1f\n",
                   ordered[i].time_elapsed, ordered[i].altitude,
                   ordered[i].speed, ordered[i].pitch, ordered[i].roll);
    }

    /* Find record closest to crash */
    BlackBoxRecord last = {0};
    for (int i = 0; i < n; i++)
        if (ordered[i].time_elapsed <= crash_time) last = ordered[i];
    free(ordered);

    printf("\n--- At impact (t=%.0fs) ---\n", last.time_elapsed);
    printf("  Altitude : %.1f m\n",      last.altitude);
    printf("  Speed    : %.1f m/s\n",    last.speed);
    printf("  Pitch    : %.1f deg\n",    last.pitch);
    printf("  Roll     : %.1f deg\n",    last.roll);
    printf("  Crash    : %s\n",          last.crash_type);
    printf("\n### END REPORT ###\n");
}
