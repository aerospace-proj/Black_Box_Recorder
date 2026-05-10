package blackbox;

import java.time.LocalTime;
import java.time.format.DateTimeFormatter;

/**
 * Evenement affichable dans le dashboard.
 * IMPORTANT : SEULS les evenements PHASE_CHANGE et CRASH sont conserves.
 * Les evenements type pitch_high / low_speed / etc. ne sont PAS stockes.
 */
public class FlightEvent {
    public enum Type { PHASE_CHANGE, CRASH, IMPACT }

    private static final DateTimeFormatter FMT = DateTimeFormatter.ofPattern("HH:mm:ss");

    private final Type   type;
    private final double simTime;
    private final String message;
    private final String realTime;

    public FlightEvent(Type type, double simTime, String message) {
        this.type     = type;
        this.simTime  = simTime;
        this.message  = message;
        this.realTime = LocalTime.now().format(FMT);
    }

    public Type   getType()    { return type; }
    public double getSimTime() { return simTime; }
    public String getMessage() { return message; }
    public String getRealTime(){ return realTime; }

    @Override
    public String toString() {
        return String.format("[%s] t=%.0fs  %-13s  %s",
                realTime, simTime, type.name(), message);
    }
}