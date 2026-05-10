package blackbox;

import java.sql.*;
import java.util.ArrayList;
import java.util.List;

/**
 * Gere la persistance dans SQLite.
 * - Table flight_data : stocke UNIQUEMENT les 200 derniers enregistrements.
 *   Des qu'un nouvel enregistrement est insere et que count > 200, le plus
 *   ancien est supprime (effet buffer circulaire cote DB).
 * - Table flight_events : stocke UNIQUEMENT les evenements PHASE_CHANGE / CRASH / IMPACT.
 *   Les evenements de type pitch_high, low_speed, high_altitude, etc. ne sont
 *   pas stockes (conformement au cahier des charges).
 */
public class DatabaseManager {

    public static final int    MAX_RECORDS = 200;
    public static final String DB_FILE     = "flight_data.db";

    private Connection conn;

    public void connect() throws SQLException {
        conn = DriverManager.getConnection("jdbc:sqlite:" + DB_FILE);
        conn.setAutoCommit(true);
        createTables();
        clearAll();
    }

    private void createTables() throws SQLException {
        try (Statement st = conn.createStatement()) {
            st.execute("CREATE TABLE IF NOT EXISTS flight_data (" +
                    "id INTEGER PRIMARY KEY AUTOINCREMENT," +
                    "sim_time   REAL    NOT NULL," +
                    "phase      TEXT    NOT NULL," +
                    "altitude   REAL    NOT NULL," +
                    "speed      REAL    NOT NULL," +
                    "pitch      REAL    NOT NULL," +
                    "roll       REAL    NOT NULL," +
                    "crashed    INTEGER NOT NULL," +
                    "crash_type TEXT    NOT NULL)");

            st.execute("CREATE TABLE IF NOT EXISTS flight_events (" +
                    "id INTEGER PRIMARY KEY AUTOINCREMENT," +
                    "event_type TEXT    NOT NULL," +
                    "sim_time   REAL    NOT NULL," +
                    "message    TEXT    NOT NULL," +
                    "real_time  TEXT    NOT NULL)");
        }
    }

    private void clearAll() throws SQLException {
        try (Statement st = conn.createStatement()) {
            st.execute("DELETE FROM flight_data");
            st.execute("DELETE FROM flight_events");
            st.execute("DELETE FROM sqlite_sequence WHERE name='flight_data'");
            st.execute("DELETE FROM sqlite_sequence WHERE name='flight_events'");
        }
    }

    /**
     * Insere un enregistrement et supprime le plus ancien si > 200.
     */
    public synchronized void insertRecord(FlightRecord r) throws SQLException {
        String ins = "INSERT INTO flight_data(sim_time,phase,altitude,speed,pitch,roll,crashed,crash_type)" +
                     " VALUES(?,?,?,?,?,?,?,?)";
        try (PreparedStatement ps = conn.prepareStatement(ins, Statement.RETURN_GENERATED_KEYS)) {
            ps.setDouble(1, r.getTime());
            ps.setString(2, r.getPhase());
            ps.setDouble(3, r.getAltitude());
            ps.setDouble(4, r.getSpeed());
            ps.setDouble(5, r.getPitch());
            ps.setDouble(6, r.getRoll());
            ps.setInt   (7, r.isCrashed() ? 1 : 0);
            ps.setString(8, r.getCrashType());
            ps.executeUpdate();
            try (ResultSet k = ps.getGeneratedKeys()) {
                if (k.next()) r.setId(k.getInt(1));
            }
        }

        /* Garde seulement les 200 dernieres lignes */
        try (Statement st = conn.createStatement()) {
            st.execute(
                "DELETE FROM flight_data WHERE id IN (" +
                "  SELECT id FROM flight_data ORDER BY id DESC LIMIT -1 OFFSET " + MAX_RECORDS +
                ")");
        }
    }

   
    /**
     * Stocke un evenement (SEULEMENT PHASE / CRASH / IMPACT).
     */
    public synchronized void insertEvent(FlightEvent ev) throws SQLException {
        String ins = "INSERT INTO flight_events(event_type,sim_time,message,real_time) VALUES(?,?,?,?)";
        try (PreparedStatement ps = conn.prepareStatement(ins)) {
            ps.setString(1, ev.getType().name());
            ps.setDouble(2, ev.getSimTime());
            ps.setString(3, ev.getMessage());
            ps.setString(4, ev.getRealTime());
            ps.executeUpdate();
        }
    }

    public synchronized int countRecords() throws SQLException {
        try (Statement st = conn.createStatement();
             ResultSet rs = st.executeQuery("SELECT COUNT(*) FROM flight_data")) {
            return rs.next() ? rs.getInt(1) : 0;
        }
    }

    public synchronized List<FlightRecord> getAllRecords() throws SQLException {
        List<FlightRecord> out = new ArrayList<>();
        String q = "SELECT sim_time,phase,altitude,speed,pitch,roll,crashed,crash_type" +
                   " FROM flight_data ORDER BY id ASC";
        try (Statement st = conn.createStatement();
             ResultSet rs = st.executeQuery(q)) {
            while (rs.next()) {
                out.add(new FlightRecord(
                        rs.getDouble(1), rs.getString(2),
                        rs.getDouble(3), rs.getDouble(4),
                        rs.getDouble(5), rs.getDouble(6),
                        rs.getInt(7) == 1, rs.getString(8)));
            }
        }
        return out;
    }

    public void close() {
        try { if (conn != null) conn.close(); } catch (SQLException ignored) {}
    }
}