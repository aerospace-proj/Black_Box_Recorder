# Flight Data Recorder - Black Box Simulation

## What is this?

This project simulates an aircraft Flight Data Recorder (Black Box).  
It records 4 sensors:

- Altitude
- Speed
- Pitch
- Roll

The system detects crashes and displays real-time data on a Java Swing dashboard.

---

# Part 1: Run the C Simulation

## Step 1: Download SQLite

Download `sqlite3.c` and `sqlite3.h` from:

https://www.sqlite.org/download.html

(Download the **amalgamation version**)

Place both files in your project folder.

---

## Step 2: Compile with GCC

```bash
gcc -Wall -Wextra -o flight_sim.exe test_all.c altitude_sensor.c speed_sensor.c pitch_sensor.c roll_sensor.c flight_phase.c crash_scenario.c circular_buffer.c utils.c error_handler.c event_handler.c database.c sqlite3.c -lm -DSQLITE_WIN32_GETVERSIONEX=0 -D_WIN32_WINNT=0x0600
```

---

## Step 3: Run

```bash
flight_sim.exe
```

The simulation runs for **200 seconds**.

You will see:

- Crash announcement (time and type)
- Sensor values every 10 seconds
- Events:
  - Overspeed
  - Low speed
  - Abnormal pitch/roll
- Final summary with crash details

Data is automatically saved to:

```text
flight_data.db
```

---

# Part 2: Run the Java Swing Dashboard

## Step 1: Download SQLite JDBC Driver

Download:

```text
sqlite-jdbc-3.47.0.0.jar
```

From:

https://github.com/xerial/sqlite-jdbc/releases

Create a folder called:

```text
lib
```

Put the `.jar` file inside the `lib` folder.

---

## Step 2: Update Database Path

In `Dashboard.java`, change this line to your path:

```java
String url = "jdbc:sqlite:C:\\YOUR_PATH\\flight_data.db";
```

---

## Step 3: Compile and Run

### Compile

```bash
javac -cp "lib\sqlite-jdbc-3.47.0.0.jar" flightUI\*.java
```

### Run

```bash
java -cp ".;lib\sqlite-jdbc-3.47.0.0.jar" flightUI.Main
```

The dashboard shows:

- 4 sensor values updating in real-time
- Event log with timestamps
- Crash alert when detected

---

# Part 3: Run Tests

## Step 1: Install Python Library

```bash
pip install fpdf
```

---

## Step 2: Run Test Script

Place `test_flight.py` in the same folder as `flight_sim.exe` and run:

```bash
python test_flight.py
```

The script runs multiple flight tests and generates a PDF report with results.

---

# Requirements

- GCC compiler
- SQLite amalgamation files:
  - `sqlite3.c`
  - `sqlite3.h`
- Java JDK 8 or higher
- SQLite JDBC driver:
  - `sqlite-jdbc-3.47.0.0.jar`
- Python 3 with `fpdf` library
