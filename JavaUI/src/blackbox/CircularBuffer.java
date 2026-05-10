package blackbox;

import java.util.ArrayList;
import java.util.List;

/**
 * Buffer circulaire de capacite fixe (200).
 * Les enregistrements les plus anciens sont ECRASES quand le buffer est plein.
 */
public class CircularBuffer {
    public static final int CAPACITY = 200;

    private final FlightRecord[] buffer = new FlightRecord[CAPACITY];
    private int head  = 0;
    private int count = 0;

    public synchronized void add(FlightRecord rec) {
        buffer[head] = rec;
        head = (head + 1) % CAPACITY;
        if (count < CAPACITY) count++;
    }

    public synchronized int size() { return count; }

    public synchronized boolean isFull() { return count == CAPACITY; }

    /** Retourne tous les enregistrements dans l'ordre chronologique. */
    public synchronized List<FlightRecord> getOrdered() {
        List<FlightRecord> list = new ArrayList<>(count);
        int start = (count < CAPACITY) ? 0 : head;
        for (int i = 0; i < count; i++) {
            list.add(buffer[(start + i) % CAPACITY]);
        }
        return list;
    }

    public synchronized FlightRecord getLast() {
        if (count == 0) return null;
        int idx = (head - 1 + CAPACITY) % CAPACITY;
        return buffer[idx];
    }

    public synchronized void clear() {
        head = 0;
        count = 0;
        for (int i = 0; i < CAPACITY; i++) buffer[i] = null;
    }
}