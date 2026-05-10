package blackbox;

import javax.swing.*;
import javax.swing.border.EmptyBorder;
import java.awt.*;
import java.sql.SQLException;

public class Main {

    private static final String DEFAULT_EXE =
            "C:\\Users\\moumn\\OneDrive\\Bureau\\black\\flight_sim.exe";

    /* ================================================================== */
    /*  6 SCENARIOS — 5 capteurs + 1 cause majeure                        */
    /* ================================================================== */
    private static final String[] SCENARIO_NAMES = {
        "0 — NO CRASH  (Normal flight)",
        "1 — OVERSPEED  [SPEED sensor]",
        "2 — HIGH ALTITUDE / HYPOXIA  [ALTITUDE sensor]",
        "3 — EXTREME PITCH  [PITCH sensor]",
        "4 — UNCONTROLLED ROLL  [ROLL sensor]",
        "5 — ENGINE FAILURE  [ENGINE]",
        "6 — PILOT ERROR / CFIT  [#1 cause of crashes]"
    };

    private static final String[] SCENARIO_SENSORS = {
        "—",
        "SPEED",
        "ALTITUDE",
        "PITCH",
        "ROLL",
        "ENGINE",
        "HUMAN FACTOR"
    };

    private static final String[] SCENARIO_CAUSES = {
        "Normal flight — all systems nominal. No incident expected.",

        "SPEED — Overspeed / Structural Failure\n\n" +
        "The aircraft exceeds its maximum operating speed (Vmo/Mmo).\n" +
        "Cause: Uncontrolled descent, stuck autopilot, or turbulence dive.\n" +
        "Effect: Aerodynamic forces tear the airframe apart mid-flight.\n\n" +
        "Real case: Air France 447 (2009) — speed sensor (Pitot) failure\n" +
        "caused autopilot disconnect and subsequent loss of control.",

        "ALTITUDE — Hypoxia / Depressurization\n\n" +
        "The aircraft climbs above safe altitude without pressurization.\n" +
        "Cause: Pressurization failure, oxygen system malfunction.\n" +
        "Effect: Crew incapacitation from hypoxia, uncontrolled climb.\n\n" +
        "Real case: Helios Airways 522 (2005) — pressurization not set,\n" +
        "entire crew lost consciousness at 34,000 ft. Aircraft flew on\n" +
        "autopilot until fuel exhaustion.",

        "PITCH — Nose Dive / Tail Strike\n\n" +
        "Extreme pitch-up or pitch-down causes loss of lift or structural damage.\n" +
        "Cause: Trim runaway, elevator malfunction, or pilot over-correction.\n" +
        "Effect: Aircraft enters an irrecoverable dive or stalls completely.\n\n" +
        "Real case: EgyptAir 990 (1999) — co-pilot deliberately\n" +
        "pushed nose down, aircraft exceeded -40 deg pitch.",

        "ROLL — Spiral Dive / Wing Drop\n\n" +
        "The aircraft enters an uncontrolled bank exceeding structural limits.\n" +
        "Cause: Aileron jam, asymmetric thrust, or spatial disorientation.\n" +
        "Effect: Spiral dive — altitude loss accelerates with bank angle.\n\n" +
        "Real case: LOT Polish Airlines 5055 (1987) — engine compressor\n" +
        "failure caused asymmetric thrust and uncontrolled roll to impact.",

        "ENGINE FAILURE — Dual Flameout\n\n" +
        "Complete loss of all engine power during cruise.\n" +
        "Cause: Fuel starvation, bird strike ingestion, or mechanical seizure.\n" +
        "Effect: Aircraft becomes a glider — crash if no runway reachable.\n\n" +
        "Real case: Air Transat 236 (2001) — fuel leak caused both engines\n" +
        "to flame out over the Atlantic. Pilots glided 120 km to the Azores.",

        "PILOT ERROR / CFIT — #1 Cause of Crashes\n\n" +
        "Controlled Flight Into Terrain: aircraft is fully functional but\n" +
        "flies into the ground due to human error.\n" +
        "Cause: Distraction, wrong altimeter setting, ignored GPWS warnings,\n" +
        "fatigue, or spatial disorientation.\n" +
        "Effect: Full-speed controlled impact with terrain or water.\n\n" +
        "Real case: Korean Air 801 (1997) — crew ignored GPWS warnings\n" +
        "during approach, impacted Guam hillside at cruise speed."
    };

    /* CrashType C correspondant : 0=none,2=overspeed,3=nose_dive(alt),
       3=nose_dive(pitch),4=spiral,5=engine,7=terrain */
    private static final int[] CRASH_TYPE_C = { 0, 2, 3, 3, 4, 5, 7 };

    private static final Color BG      = new Color(15, 23, 42);
    private static final Color CARD    = new Color(30, 41, 59);
    private static final Color ACCENT  = new Color(59, 130, 246);
    private static final Color DANGER  = new Color(239, 68, 68);
    private static final Color WARN    = new Color(234, 179, 8);
    private static final Color OK      = new Color(34, 197, 94);
    private static final Color TEXT    = new Color(226, 232, 240);
    private static final Color SUBDUED = new Color(148, 163, 184);

    /* ================================================================== */
    /*  STEP 1 — Selection du scenario                                    */
    /* ================================================================== */
    private static int showScenarioDialog() {
        final int[] selected = {0};

        JDialog dialog = new JDialog((Frame) null, "Flight Crash Simulator — Select Scenario", true);
        dialog.setSize(780, 580);
        dialog.setLocationRelativeTo(null);
        dialog.setDefaultCloseOperation(JDialog.DISPOSE_ON_CLOSE);
        dialog.getContentPane().setBackground(BG);
        dialog.setLayout(new BorderLayout(0, 0));

        /* Header */
        JPanel headerPanel = new JPanel(new BorderLayout());
        headerPanel.setBackground(BG);
        headerPanel.setBorder(new EmptyBorder(20, 20, 10, 20));

        JLabel title = new JLabel("SELECT CRASH SCENARIO", SwingConstants.CENTER);
        title.setForeground(TEXT);
        title.setFont(new Font("SansSerif", Font.BOLD, 18));

        JLabel subtitle = new JLabel(
            "5 sensor-based crashes  +  1 real-world leading cause", SwingConstants.CENTER);
        subtitle.setForeground(SUBDUED);
        subtitle.setFont(new Font("SansSerif", Font.PLAIN, 12));

        headerPanel.add(title,    BorderLayout.CENTER);
        headerPanel.add(subtitle, BorderLayout.SOUTH);
        dialog.add(headerPanel, BorderLayout.NORTH);

        /* Center */
        JPanel center = new JPanel(new BorderLayout(12, 0));
        center.setBackground(BG);
        center.setBorder(new EmptyBorder(0, 16, 0, 16));

        /* List */
        JList<String> list = new JList<>(SCENARIO_NAMES);
        list.setBackground(CARD);
        list.setForeground(TEXT);
        list.setSelectionBackground(ACCENT);
        list.setSelectionForeground(Color.WHITE);
        list.setFont(new Font("SansSerif", Font.PLAIN, 14));
        list.setFixedCellHeight(46);
        list.setBorder(new EmptyBorder(6, 10, 6, 10));
        list.setSelectedIndex(0);

        JScrollPane listScroll = new JScrollPane(list);
        listScroll.setBorder(BorderFactory.createLineBorder(new Color(51, 65, 85)));
        listScroll.setPreferredSize(new Dimension(310, 0));
        listScroll.getViewport().setBackground(CARD);

        /* Detail panel */
        JPanel detail = new JPanel(new BorderLayout(0, 10));
        detail.setBackground(CARD);
        detail.setBorder(BorderFactory.createCompoundBorder(
                BorderFactory.createLineBorder(new Color(51, 65, 85)),
                new EmptyBorder(16, 16, 16, 16)));

        /* Sensor badge */
        JLabel sensorBadge = new JLabel("SENSOR: —");
        sensorBadge.setForeground(BG);
        sensorBadge.setBackground(SUBDUED);
        sensorBadge.setOpaque(true);
        sensorBadge.setFont(new Font("Monospaced", Font.BOLD, 11));
        sensorBadge.setBorder(new EmptyBorder(3, 8, 3, 8));

        JPanel badgeRow = new JPanel(new FlowLayout(FlowLayout.LEFT, 0, 0));
        badgeRow.setBackground(CARD);
        badgeRow.add(sensorBadge);

        JLabel detailTitle = new JLabel(SCENARIO_NAMES[0]);
        detailTitle.setForeground(OK);
        detailTitle.setFont(new Font("SansSerif", Font.BOLD, 14));

        JTextArea detailText = new JTextArea(SCENARIO_CAUSES[0]);
        detailText.setBackground(CARD);
        detailText.setForeground(SUBDUED);
        detailText.setFont(new Font("SansSerif", Font.PLAIN, 12));
        detailText.setLineWrap(true);
        detailText.setWrapStyleWord(true);
        detailText.setEditable(false);
        detailText.setBorder(new EmptyBorder(8, 0, 0, 0));

        JScrollPane textScroll = new JScrollPane(detailText);
        textScroll.setBorder(null);
        textScroll.getViewport().setBackground(CARD);

        JPanel detailTop = new JPanel(new BorderLayout(0, 6));
        detailTop.setBackground(CARD);
        detailTop.add(badgeRow,    BorderLayout.NORTH);
        detailTop.add(detailTitle, BorderLayout.CENTER);

        detail.add(detailTop,   BorderLayout.NORTH);
        detail.add(textScroll,  BorderLayout.CENTER);

        /* Sensor badge colors */
        Color[] badgeColors = {
            SUBDUED, new Color(239,68,68), new Color(168,85,247),
            new Color(234,179,8), new Color(249,115,22),
            new Color(20,184,166), new Color(239,68,68)
        };

        list.addListSelectionListener(e -> {
            if (!e.getValueIsAdjusting()) {
                int idx = list.getSelectedIndex();
                if (idx >= 0) {
                    selected[0] = idx;
                    detailTitle.setText(SCENARIO_NAMES[idx]);
                    detailText.setText(SCENARIO_CAUSES[idx]);
                    detailText.setCaretPosition(0);
                    detailTitle.setForeground(idx == 0 ? OK : DANGER);
                    sensorBadge.setText("SENSOR: " + SCENARIO_SENSORS[idx]);
                    sensorBadge.setBackground(badgeColors[idx]);
                    sensorBadge.setForeground(idx == 0 ? new Color(30,41,59) : Color.WHITE);
                }
            }
        });

        center.add(listScroll, BorderLayout.WEST);
        center.add(detail,     BorderLayout.CENTER);
        dialog.add(center, BorderLayout.CENTER);

        /* Buttons */
        JPanel buttons = new JPanel(new FlowLayout(FlowLayout.RIGHT, 16, 14));
        buttons.setBackground(BG);

        JButton btnCancel = new JButton("Cancel");
        btnCancel.setBackground(new Color(51, 65, 85));
        btnCancel.setForeground(TEXT);
        btnCancel.setFocusPainted(false);
        btnCancel.setFont(new Font("SansSerif", Font.PLAIN, 13));
        btnCancel.addActionListener(e -> { selected[0] = -1; dialog.dispose(); });

        JButton btnNext = new JButton("Next  ▶");
        btnNext.setBackground(ACCENT);
        btnNext.setForeground(Color.WHITE);
        btnNext.setFocusPainted(false);
        btnNext.setFont(new Font("SansSerif", Font.BOLD, 14));
        btnNext.addActionListener(e -> dialog.dispose());

        buttons.add(btnCancel);
        buttons.add(btnNext);
        dialog.add(buttons, BorderLayout.SOUTH);

        dialog.setVisible(true);
        return selected[0];
    }

    /* ================================================================== */
    /*  STEP 2 — Briefing avant lancement                                 */
    /* ================================================================== */
    private static boolean showCrashBriefing(int scenario, int duration, int crashTime) {
        final boolean[] confirmed = {false};

        JDialog dialog = new JDialog((Frame) null, "Mission Briefing", true);
        dialog.setSize(520, 440);
        dialog.setLocationRelativeTo(null);
        dialog.setDefaultCloseOperation(JDialog.DISPOSE_ON_CLOSE);
        dialog.getContentPane().setBackground(BG);
        dialog.setLayout(new BorderLayout(0, 0));

        JLabel header = new JLabel("MISSION BRIEFING", SwingConstants.CENTER);
        header.setForeground(TEXT);
        header.setFont(new Font("SansSerif", Font.BOLD, 18));
        header.setBorder(new EmptyBorder(20, 0, 6, 0));
        header.setBackground(BG);
        header.setOpaque(true);
        dialog.add(header, BorderLayout.NORTH);

        JPanel info = new JPanel();
        info.setBackground(BG);
        info.setLayout(new BoxLayout(info, BoxLayout.Y_AXIS));
        info.setBorder(new EmptyBorder(10, 30, 10, 30));

        info.add(makeRow("SCENARIO",
                SCENARIO_NAMES[scenario], scenario == 0 ? OK : DANGER));
        info.add(Box.createVerticalStrut(8));
        info.add(makeRow("SENSOR INVOLVED",
                SCENARIO_SENSORS[scenario], ACCENT));
        info.add(Box.createVerticalStrut(14));

        JSeparator sep = new JSeparator();
        sep.setForeground(new Color(51, 65, 85));
        sep.setMaximumSize(new Dimension(Integer.MAX_VALUE, 1));
        info.add(sep);
        info.add(Box.createVerticalStrut(14));

        info.add(makeRow("FLIGHT DURATION",
                duration + " s  (" + String.format("%.1f", duration / 60.0) + " min)", TEXT));
        info.add(Box.createVerticalStrut(8));

        /* Estimated time to impact per scenario (matches crash_scenario.c power curve) */
        int[] timeToImpact = { 0, 32, 30, 28, 30, 35, 30, 33 };  /* ~30s each */
        int   tti = (scenario < timeToImpact.length) ? timeToImpact[scenario] : 60;

        if (scenario != 0) {
            int min = crashTime / 60;
            int sec = crashTime % 60;
            info.add(makeRow("CRASH TRIGGER",
                    String.format("T = %d s  (%d min %02d s into flight)", crashTime, min, sec),
                    WARN));
            info.add(Box.createVerticalStrut(8));
            info.add(makeRow("TIME TO IMPACT",
                    String.format("~%d seconds after trigger  (impact at T=%ds)", tti, crashTime + tti),
                    DANGER));
        } else {
            info.add(makeRow("EXPECTED OUTCOME",
                    "Normal landing — no incident.", OK));
        }

        dialog.add(info, BorderLayout.CENTER);

        JPanel buttons = new JPanel(new FlowLayout(FlowLayout.RIGHT, 16, 14));
        buttons.setBackground(BG);

        JButton btnBack = new JButton("◀  Back");
        btnBack.setBackground(new Color(51, 65, 85));
        btnBack.setForeground(TEXT);
        btnBack.setFocusPainted(false);
        btnBack.setFont(new Font("SansSerif", Font.PLAIN, 13));
        btnBack.addActionListener(e -> dialog.dispose());

        JButton btnStart = new JButton(scenario == 0 ? "▶  START FLIGHT" : "▶  START SIMULATION");
        btnStart.setBackground(scenario == 0 ? OK : DANGER);
        btnStart.setForeground(Color.WHITE);
        btnStart.setFocusPainted(false);
        btnStart.setFont(new Font("SansSerif", Font.BOLD, 14));
        btnStart.addActionListener(e -> { confirmed[0] = true; dialog.dispose(); });

        buttons.add(btnBack);
        buttons.add(btnStart);
        dialog.add(buttons, BorderLayout.SOUTH);

        dialog.setVisible(true);
        return confirmed[0];
    }

    private static JPanel makeRow(String label, String value, Color valueColor) {
        JPanel row = new JPanel(new BorderLayout(12, 0));
        row.setBackground(BG);
        row.setMaximumSize(new Dimension(Integer.MAX_VALUE, 60));

        JLabel lbl = new JLabel(label);
        lbl.setForeground(SUBDUED);
        lbl.setFont(new Font("Monospaced", Font.PLAIN, 11));
        lbl.setPreferredSize(new Dimension(150, 0));

        JTextArea val = new JTextArea(value);
        val.setBackground(BG);
        val.setForeground(valueColor);
        val.setFont(new Font("SansSerif", Font.BOLD, 13));
        val.setLineWrap(true);
        val.setWrapStyleWord(true);
        val.setEditable(false);
        val.setBorder(null);

        row.add(lbl, BorderLayout.WEST);
        row.add(val, BorderLayout.CENTER);
        return row;
    }

    /* ================================================================== */
    /*  MAIN                                                               */
    /* ================================================================== */
    public static void main(String[] args) {
        String cExecutable = args.length > 0 ? args[0] : DEFAULT_EXE;
        int duration = 380;

        while (true) {
            int scenario = showScenarioDialog();
            if (scenario < 0) { System.exit(0); }

            int crashTime = (scenario == 0) ? 0 : (int)(duration * 0.65f);
            boolean confirmed = showCrashBriefing(scenario, duration, crashTime);

            if (confirmed) {
                int crashTypeC = CRASH_TYPE_C[scenario];
                launchSimulation(cExecutable, duration, crashTime, crashTypeC);
                break;
            }
        }
    }

    /* ================================================================== */
    /*  LANCEMENT                                                         */
    /* ================================================================== */
    private static void launchSimulation(String exe, int duration, int crashTime, int crashType) {

        Dashboard dash = new Dashboard();
        SwingUtilities.invokeLater(() -> dash.setVisible(true));

        final CircularBuffer buffer = new CircularBuffer();
        final DatabaseManager db    = new DatabaseManager();
        try {
            db.connect();
        } catch (SQLException e) {
            System.err.println("[MAIN] DB failed: " + e.getMessage()); return;
        }
        Runtime.getRuntime().addShutdownHook(new Thread(db::close));

        FlightDataReader.Listener listener = new FlightDataReader.Listener() {
            @Override public void onInit(String info) {
                System.out.println("[INIT] " + info);
            }
            @Override public void onData(FlightRecord rec) {
                buffer.add(rec);
                try { db.insertRecord(rec); } catch (SQLException ignored) {}
                dash.updateLive(rec, buffer.size());
            }
            @Override public void onPhaseChange(double t, String phase) {
                FlightEvent ev = new FlightEvent(FlightEvent.Type.PHASE_CHANGE, t, "Phase -> " + phase);
                dash.addEvent(ev);
                try { db.insertEvent(ev); } catch (SQLException ignored) {}
            }
            @Override public void onCrash(double t, String type, String desc, String cause) {
                FlightEvent ev = new FlightEvent(FlightEvent.Type.CRASH, t,
                        type + " | " + desc + " | cause: " + cause);
                dash.addEvent(ev);
                dash.setCrashedBanner(type);
                try { db.insertEvent(ev); } catch (SQLException ignored) {}
            }
            @Override public void onImpact(double t, double alt) {
                FlightRecord last = buffer.getLast();
                String ct = last != null ? last.getCrashType() : "UNKNOWN";
                FlightEvent ev = new FlightEvent(FlightEvent.Type.IMPACT, t,
                        "IMPACT SOL - ACCIDENT DECLARE (" + ct + ")");
                dash.addEvent(ev);
                dash.declareAccident(ct, t, alt);
                try { db.insertEvent(ev); } catch (SQLException ignored) {}
            }
            @Override public void onEnd() {
                dash.setSimulationEnded();
                try { System.out.println("[END] Records: " + db.countRecords()); }
                catch (SQLException ignored) {}
            }
        };

        new Thread(new FlightDataReader(exe, duration, crashTime, crashType, listener),
                "C-Sim-Reader").start();
    }
}