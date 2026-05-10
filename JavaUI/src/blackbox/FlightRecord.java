package blackbox;

/**
 * Represente un enregistrement de vol (une ligne du black-box).
 */
public class FlightRecord {
    private int id;              // id attribue lors de l'insertion DB
    private double time;
    private String phase;
    private double altitude;
    private double speed;
    private double pitch;
    private double roll;
    private boolean crashed;
    private String crashType;

    public FlightRecord(double time, String phase, double altitude, double speed,
                        double pitch, double roll, boolean crashed, String crashType) {
        this.time      = time;
        this.phase     = phase;
        this.altitude  = altitude;
        this.speed     = speed;
        this.pitch     = pitch;
        this.roll      = roll;
        this.crashed   = crashed;
        this.crashType = crashType;
    }

    public int     getId()        { return id; }
    public void    setId(int id)  { this.id = id; }
    public double  getTime()      { return time; }
    public String  getPhase()     { return phase; }
    public double  getAltitude()  { return altitude; }
    public double  getSpeed()     { return speed; } /* speed is in kts — already converted by C */
    public double  getPitch()     { return pitch; }
    public double  getRoll()      { return roll; }
    public boolean isCrashed()    { return crashed; }
    public String  getCrashType() { return crashType; }

    @Override
    public String toString() {
        return String.format("t=%.0fs phase=%s alt=%.1fm spd=%.1fkts pit=%.1f rol=%.1f%s",
                time, phase, altitude, speed * 1.94384, pitch, roll,
                crashed ? " [CRASHED:" + crashType + "]" : "");
    }
}