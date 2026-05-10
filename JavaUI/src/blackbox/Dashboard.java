package blackbox;

import javax.swing.*;
import javax.swing.border.EmptyBorder;
import javax.swing.border.TitledBorder;
import java.awt.*;
import java.util.ArrayList;
import java.util.List;

/**
 * Dashboard temps reel.
 * - Affiche les telemetries live (alt, spd, pit, rol, phase)
 * - Journal des evenements : UNIQUEMENT phases et crashes (les evenements
 *   pitch_high, low_speed, high_altitude, etc. ne s'affichent PAS).
 * - Statut gros plan : FLYING / CRASHED / ACCIDENT DECLARED
 * - Graphique simple altitude = f(temps) sur les 200 derniers points
 */
public class Dashboard extends JFrame {

    /* Couleurs */
    private static final Color BG        = new Color(15, 23, 42);
    private static final Color CARD      = new Color(30, 41, 59);
    private static final Color ACCENT    = new Color(59, 130, 246);
    private static final Color OK        = new Color(34, 197, 94);
    private static final Color WARN      = new Color(234, 179, 8);
    private static final Color DANGER    = new Color(239, 68, 68);
    private static final Color TEXT      = new Color(226, 232, 240);
    private static final Color TEXT_DIM  = new Color(148, 163, 184);

    /* Labels live */
    private final JLabel lblTime     = big("0s");
    private final JLabel lblPhase    = big("---");
    private final JLabel lblAlt      = big("0 m");
    private final JLabel lblSpd      = big("0 kts");
    private final JLabel lblPit      = big("0.0 deg");
    private final JLabel lblRol      = big("0.0 deg");
    private final JLabel lblStatus   = bigStatus("FLYING");
    private final JLabel lblBuffer   = small("Buffer: 0 / 200");

    /* Events */
    private final DefaultListModel<String> evModel = new DefaultListModel<>();
    private final JList<String>            evList  = new JList<>(evModel);

    /* Graphique */
    private final AltitudePanel chart = new AltitudePanel();

    public Dashboard() {
        super("Aerospace Flight Data Recorder - Black Box Dashboard");
        setDefaultCloseOperation(EXIT_ON_CLOSE);
        setSize(1180, 720);
        setLocationRelativeTo(null);
        getContentPane().setBackground(BG);
        setLayout(new BorderLayout(10, 10));

        add(buildHeader(),  BorderLayout.NORTH);
        add(buildCenter(),  BorderLayout.CENTER);
        add(buildFooter(),  BorderLayout.SOUTH);
    }

    /* ================= UI Builders ================= */

    private JPanel buildHeader() {
        JPanel h = new JPanel(new GridLayout(1, 6, 10, 0));
        h.setBackground(BG);
        h.setBorder(new EmptyBorder(12, 12, 6, 12));
        h.add(card("TIME (sim)", lblTime));
        h.add(card("PHASE",      lblPhase));
        h.add(card("ALTITUDE",   lblAlt));
        h.add(card("SPEED (kts)", lblSpd));
        h.add(card("PITCH",      lblPit));
        h.add(card("ROLL",       lblRol));
        return h;
    }

    private JComponent buildCenter() {
        JSplitPane sp = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT,
                buildChartPanel(), buildEventsPanel());
        sp.setResizeWeight(0.62);
        sp.setBorder(null);
        sp.setBackground(BG);
        sp.setDividerSize(6);
        return sp;
    }

    private JPanel buildChartPanel() {
        JPanel p = new JPanel(new BorderLayout());
        p.setBackground(CARD);
        p.setBorder(titled("ALTITUDE (live - 200 derniers points)"));
        p.add(chart, BorderLayout.CENTER);
        return p;
    }

    private JPanel buildEventsPanel() {
        JPanel p = new JPanel(new BorderLayout());
        p.setBackground(CARD);
        p.setBorder(titled("EVENEMENTS (phases et crashes uniquement)"));

        evList.setBackground(CARD);
        evList.setForeground(TEXT);
        evList.setFont(new Font(Font.MONOSPACED, Font.PLAIN, 12));
        evList.setSelectionBackground(ACCENT);

        JScrollPane sc = new JScrollPane(evList);
        sc.setBorder(null);
        sc.getViewport().setBackground(CARD);
        p.add(sc, BorderLayout.CENTER);
        return p;
    }

    private JPanel buildFooter() {
        JPanel f = new JPanel(new BorderLayout(10, 0));
        f.setBackground(BG);
        f.setBorder(new EmptyBorder(0, 12, 12, 12));

        JPanel stCard = new JPanel(new BorderLayout());
        stCard.setBackground(CARD);
        stCard.setBorder(titled("STATUT DU VOL"));
        stCard.add(lblStatus, BorderLayout.CENTER);

        JPanel right = new JPanel(new BorderLayout());
        right.setBackground(CARD);
        right.setBorder(titled("BLACKBOX BUFFER"));
        right.add(lblBuffer, BorderLayout.CENTER);
        right.setPreferredSize(new Dimension(260, 80));

        f.add(stCard, BorderLayout.CENTER);
        f.add(right,  BorderLayout.EAST);
        return f;
    }

    private JPanel card(String title, JLabel value) {
        JPanel c = new JPanel(new BorderLayout(4, 4));
        c.setBackground(CARD);
        c.setBorder(BorderFactory.createCompoundBorder(
                BorderFactory.createLineBorder(new Color(51, 65, 85), 1),
                new EmptyBorder(10, 12, 10, 12)));
        JLabel t = new JLabel(title);
        t.setForeground(TEXT_DIM);
        t.setFont(new Font("SansSerif", Font.PLAIN, 11));
        c.add(t, BorderLayout.NORTH);
        c.add(value, BorderLayout.CENTER);
        return c;
    }

    private TitledBorder titled(String text) {
        TitledBorder b = BorderFactory.createTitledBorder(
                BorderFactory.createLineBorder(new Color(51, 65, 85), 1), text);
        b.setTitleColor(TEXT_DIM);
        b.setTitleFont(new Font("SansSerif", Font.BOLD, 12));
        return b;
    }

    private static JLabel big(String txt) {
        JLabel l = new JLabel(txt);
        l.setForeground(TEXT);
        l.setFont(new Font("SansSerif", Font.BOLD, 22));
        return l;
    }

    private static JLabel bigStatus(String txt) {
        JLabel l = new JLabel(txt, SwingConstants.CENTER);
        l.setForeground(OK);
        l.setFont(new Font("SansSerif", Font.BOLD, 26));
        return l;
    }

    private static JLabel small(String txt) {
        JLabel l = new JLabel(txt, SwingConstants.CENTER);
        l.setForeground(TEXT);
        l.setFont(new Font("SansSerif", Font.BOLD, 16));
        return l;
    }

    /* ================= API (thread-safe via Swing EDT) ================= */

    public void updateLive(FlightRecord r, int bufSize) {
        SwingUtilities.invokeLater(() -> {
            lblTime.setText(String.format("%.0fs", r.getTime()));
            lblPhase.setText(r.getPhase());
            lblAlt.setText(String.format("%.1f m", r.getAltitude()));
            lblSpd.setText(String.format("%.1f kts", r.getSpeed())); /* speed already in kts from C */
            lblPit.setText(String.format("%.1f deg", r.getPitch()));
            lblRol.setText(String.format("%.1f deg", r.getRoll()));
            lblBuffer.setText(String.format("Buffer: %d / 200", bufSize));
            chart.pushAltitude(r.getAltitude());
            if (r.isCrashed() && !"NORMAL_FLIGHT".equals(r.getCrashType())) {
                lblPhase.setForeground(DANGER);
            } else {
                lblPhase.setForeground(TEXT);
            }
        });
    }

    public void addEvent(FlightEvent ev) {
        SwingUtilities.invokeLater(() -> {
            String prefix;
            switch (ev.getType()) {
                case PHASE_CHANGE: prefix = "[PHASE] "; break;
                case CRASH:        prefix = "[CRASH] "; break;
                case IMPACT:       prefix = "[IMPACT] "; break;
                default:           prefix = "[EVT] ";
            }
            evModel.addElement(String.format("[%s] t=%.0fs  %s%s",
                    ev.getRealTime(), ev.getSimTime(), prefix, ev.getMessage()));
            evList.ensureIndexIsVisible(evModel.getSize() - 1);
        });
    }

    public void declareAccident(String crashType, double time, double altitude) {
        SwingUtilities.invokeLater(() -> {
            lblStatus.setForeground(DANGER);
            lblStatus.setText("ACCIDENT DECLARE  -  " + crashType
                    + "  @ t=" + (int)time + "s  |  alt=" + (int)altitude + " m");
        });
    }

    public void setCrashedBanner(String crashType) {
        SwingUtilities.invokeLater(() -> {
            lblStatus.setForeground(WARN);
            lblStatus.setText("CRASH EN COURS  -  " + crashType);
        });
    }

    public void setSimulationEnded() {
        SwingUtilities.invokeLater(() -> {
            // ne change pas si accident deja declare
            if (!lblStatus.getText().startsWith("ACCIDENT")) {
                lblStatus.setForeground(TEXT_DIM);
                lblStatus.setText("SIMULATION TERMINEE");
            }
        });
    }

    /* ================= Simple chart panel ================= */

    private static class AltitudePanel extends JPanel {
        private final List<Double> points = new ArrayList<>();
        private static final int MAX = 200;

        AltitudePanel() {
            setBackground(new Color(30, 41, 59));
        }

        synchronized void pushAltitude(double v) {
            points.add(v);
            if (points.size() > MAX) points.remove(0);
            repaint();
        }

        @Override
        protected void paintComponent(Graphics g) {
            super.paintComponent(g);
            Graphics2D g2 = (Graphics2D) g;
            g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING,
                                RenderingHints.VALUE_ANTIALIAS_ON);

            int w = getWidth(), h = getHeight();
            int padL = 50, padR = 10, padT = 10, padB = 24;

            /* Grille */
            g2.setColor(new Color(51, 65, 85));
            for (int i = 0; i <= 5; i++) {
                int y = padT + (h - padT - padB) * i / 5;
                g2.drawLine(padL, y, w - padR, y);
            }

            /* Axes labels */
            g2.setColor(new Color(148, 163, 184));
            g2.setFont(new Font(Font.SANS_SERIF, Font.PLAIN, 10));
            g2.drawString("10000 m", 5, padT + 10);
            g2.drawString("5000 m",  5, padT + (h - padT - padB) / 2 + 4);
            g2.drawString("0 m",     5, h - padB + 4);

            List<Double> snap;
            synchronized (this) { snap = new ArrayList<>(points); }
            if (snap.size() < 2) return;

            double maxAlt = 10000.0;
            int n = snap.size();
            int plotW = w - padL - padR;
            int plotH = h - padT - padB;

            g2.setColor(new Color(59, 130, 246));
            g2.setStroke(new BasicStroke(2f));
            int prevX = padL;
            int prevY = padT + plotH - (int)((snap.get(0) / maxAlt) * plotH);
            for (int i = 1; i < n; i++) {
                int x = padL + (int)((i / (double)(MAX - 1)) * plotW);
                int y = padT + plotH - (int)((snap.get(i) / maxAlt) * plotH);
                g2.drawLine(prevX, prevY, x, y);
                prevX = x; prevY = y;
            }
        }
    }
}