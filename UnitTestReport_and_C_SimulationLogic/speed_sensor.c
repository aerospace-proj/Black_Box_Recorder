#include "sensors.h"
#include <stdlib.h>

static float current_speed = 0.0f;

void reset_speed(void) {
    current_speed = 0.0f;
}

float generate_speed(FlightPhase phase, float target_speed, int crashed) {
    int random_int = rand() % 100;
    float noise = (float)(random_int - 50) / 20.0f;
    float acceleration = 8.0f;

    if (crashed) {
        /*
         * During a crash, speed does NOT drop to zero.
         * The aircraft is still moving — it accelerates downward
         * due to gravity and loss of lift.
         * Speed decreases slightly (drag, engine loss) but stays
         * well above 0 until impact.
         *
         * Minimum crash speed = 80 m/s (155 kts) — realistic impact speed.
         * Below 10,000 ft limit is 250 kts, impact is typically 150-250 kts.
         */
        current_speed = current_speed - 5.0f;  /* slow deceleration during fall */
        if (current_speed < 0.0f) current_speed = 0.0f; /* never below 155 kts at impact */
        return current_speed + noise;
    }

    if (current_speed < target_speed) {
        current_speed += acceleration;
        if (current_speed > target_speed) current_speed = target_speed;
    }
    else if (current_speed > target_speed) {
        current_speed -= acceleration;
        if (current_speed < target_speed) current_speed = target_speed;
    }

    float final_speed = current_speed + noise;
    if (final_speed < 0) final_speed = 0;

    return final_speed;
}

float speed_to_knots(float speed_ms) {
    return speed_ms * 1.94384f;
}
