package blackbox;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;

/**
 * Lance l'executable C et lit ses sorties ligne par ligne.
 *
 * Protocole C -> Java :
 *   INIT|duration=...|crash_time=...|crash_type=...
 *   PHASE|<phase_name>
 *   DATA|time|phase|alt|spd|pit|rol|crashed|crash_type
 *   CRASH|<type>|<desc>|<cause>
 *   IMPACT|<time>|<altitude>
 *   END
 */
public class FlightDataReader implements Runnable {

    public interface Listener {
        void onInit(String info);
        void onData(FlightRecord rec);
        void onPhaseChange(double simTime, String phase);
        void onCrash(double simTime, String type, String desc, String cause);
        void onImpact(double simTime, double altitude);
        void onEnd();
    }

    private final String   cExecutable;
    private final int      duration;
    private final int      crashTime;
    private final int      crashType;
    private final Listener listener;
    private volatile Process process;
    private volatile double  lastSimTime = 0.0;

    public FlightDataReader(String cExecutable, int duration, int crashTime,
                            int crashType, Listener listener) {
        this.cExecutable = cExecutable;
        this.duration    = duration;
        this.crashTime   = crashTime;
        this.crashType   = crashType;
        this.listener    = listener;
    }

    public void stop() {
        if (process != null) process.destroy();
    }

    @Override
    public void run() {
        try {
            System.out.println("[READER] Lancement : " + cExecutable
                    + " " + duration + " " + crashTime + " " + crashType);

            ProcessBuilder pb = new ProcessBuilder(
                    cExecutable,
                    String.valueOf(duration),
                    String.valueOf(crashTime),
                    String.valueOf(crashType));

            /* CRITIQUE : ne pas fusionner stderr dans stdout
             * Les messages d'erreur du C ne doivent pas polluer le protocole */
            pb.redirectErrorStream(false);

            /* Dossier de travail = dossier contenant l'exe */
            java.io.File exeFile = new java.io.File(cExecutable);
            if (exeFile.getParentFile() != null) {
                pb.directory(exeFile.getParentFile());
                System.out.println("[READER] Working dir : " + exeFile.getParentFile());
            }

            process = pb.start();

            /* Thread separé pour vider stderr (evite le blocage du processus C) */
            Thread stderrDrain = new Thread(() -> {
                try (BufferedReader err = new BufferedReader(
                        new InputStreamReader(process.getErrorStream()))) {
                    String line;
                    while ((line = err.readLine()) != null) {
                        System.err.println("[C-STDERR] " + line);
                    }
                } catch (IOException ignored) {}
            }, "C-Stderr-Drain");
            stderrDrain.setDaemon(true);
            stderrDrain.start();

            /* Lecture stdout ligne par ligne — bloquant jusqu'a END ou fin du processus */
            try (BufferedReader br = new BufferedReader(
                    new InputStreamReader(process.getInputStream()))) {

                String line;
                while ((line = br.readLine()) != null) {
                    line = line.trim();
                    if (!line.isEmpty()) {
                        System.out.println("[READER] << " + line); // debug
                        parseLine(line);
                    }
                }
            }

            process.waitFor();
            System.out.println("[READER] Processus C termine (code="
                    + process.exitValue() + ")");

        } catch (IOException e) {
            System.err.println("[READER] IOException : " + e.getMessage());
            e.printStackTrace();
        } catch (InterruptedException e) {
            System.err.println("[READER] Interrupted : " + e.getMessage());
            Thread.currentThread().interrupt();
        } finally {
            listener.onEnd();
        }
    }

    private void parseLine(String line) {
        if (line.isEmpty()) return;
        String[] p = line.split("\\|", -1);
        if (p.length == 0) return;
        String tag = p[0];

        switch (tag) {

            case "INIT":
                listener.onInit(line.length() > 5 ? line.substring(5) : line);
                break;

            case "DATA":
                /* DATA|time|phase|alt|spd|pit|rol|crashed|crash_type */
                if (p.length >= 9) {
                    try {
                        double  t   = Double.parseDouble(p[1]);
                        String  ph  = p[2];
                        double  alt = Double.parseDouble(p[3]);
                        double  spd = Double.parseDouble(p[4]);
                        double  pit = Double.parseDouble(p[5]);
                        double  rol = Double.parseDouble(p[6]);
                        boolean cr  = "1".equals(p[7].trim());
                        String  ct  = p[8];
                        lastSimTime = t;
                        listener.onData(new FlightRecord(t, ph, alt, spd, pit, rol, cr, ct));
                    } catch (NumberFormatException e) {
                        System.err.println("[READER] Erreur parse DATA : " + line);
                    }
                } else {
                    System.err.println("[READER] DATA incomplet (" + p.length + " champs) : " + line);
                }
                break;

            case "PHASE":
                if (p.length >= 2)
                    listener.onPhaseChange(lastSimTime, p[1]);
                break;

            case "CRASH":
                if (p.length >= 4)
                    listener.onCrash(lastSimTime, p[1], p[2], p[3]);
                else if (p.length >= 2)
                    listener.onCrash(lastSimTime, p[1], "", "");
                break;

            case "IMPACT":
                if (p.length >= 3) {
                    try {
                        double t   = Double.parseDouble(p[1]);
                        double alt = Double.parseDouble(p[2]);
                        listener.onImpact(t, alt);
                    } catch (NumberFormatException e) {
                        System.err.println("[READER] Erreur parse IMPACT : " + line);
                    }
                }
                break;

            case "END":
                /* gere dans finally */
                break;

            default:
                /* ligne ignoree (messages interactifs du C, etc.) */
                System.out.println("[READER] ligne ignoree : " + line);
                break;
        }
    }
}