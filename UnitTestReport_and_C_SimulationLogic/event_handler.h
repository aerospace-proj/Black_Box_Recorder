/**
 * @file event_handler.h
 * @brief Flight event types and logging declarations
 * @author Team G1
 */

#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

/**
 * @brief Enumeration of all loggable in-flight events.
 */
typedef enum {
    EVENT_CRASH        = 0,  /**< Crash scenario triggered          */
    EVENT_OVERSPEED    = 1,  /**< Speed exceeded safe limit         */
    EVENT_LOW_SPEED    = 2,  /**< Speed below stall threshold       */
    EVENT_HIGH_PITCH   = 3,  /**< Pitch outside normal range        */
    EVENT_HIGH_ROLL    = 4,  /**< Roll outside normal range         */
    EVENT_RAPID_DESCENT= 5   /**< Altitude drop > 80 m in one tick  */
} EventType;

/**
 * @brief Logs a flight event to events.txt.
 *
 * @param type  Type of event
 * @param value Numeric context value (speed, pitch, roll, drop rate, or 0 for crash)
 */
void log_event(EventType type, float value);

#endif /* EVENT_HANDLER_H */
