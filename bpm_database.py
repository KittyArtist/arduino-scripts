import sqlite3
import paho.mqtt.client as mqtt

# Database connection
conn = sqlite3.connect("pulse_data.db")
c = conn.cursor()

# Ensure table structure for pulses
c.execute(""" 
    CREATE TABLE IF NOT EXISTS pulses (
        id INTEGER PRIMARY KEY,
        bpm INTEGER,  -- Allow NULL values
        body_temperature REAL,  -- Allow NULL values
        timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
    )
""")
conn.commit()

# Ensure table structure for environment with temperature and humidity in the same table
c.execute(""" 
    CREATE TABLE IF NOT EXISTS environment (
        id INTEGER PRIMARY KEY,
        temperature REAL,
        humidity REAL,
        timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
    )
""")
conn.commit()

def on_message(client, userdata, msg):
    topic = msg.topic
    payload = msg.payload.decode()
    
    print(f"Received message on topic '{topic}': {payload}")  # Debugging line

    if topic == "sensors/pulse":
        c.execute("INSERT INTO pulses (bpm, body_temperature) VALUES (?, NULL)", (int(payload),))
        print(f"Logged BPM: {payload}")

    elif topic == "sensors/body_temperature":
        c.execute("UPDATE pulses SET body_temperature = ? WHERE id = (SELECT MAX(id) FROM pulses)", (float(payload),))
        print(f"Updated Body Temperature: {payload}")

    elif topic == "sensors/temperature_humidity":  # Handle the combined message
        try:
            # Split the payload to get temperature and humidity
            parts = payload.split(", ")
            temperature = None
            humidity = None
            
            for part in parts:
                if "Temperature" in part:
                    temperature = float(part.split(": ")[1])
                elif "Humidity" in part:
                    humidity = float(part.split(": ")[1])

            # Insert into database, allowing NULL values
            c.execute("INSERT INTO environment (temperature, humidity) VALUES (?, ?)", (temperature, humidity))
            print(f"Logged Temperature: {temperature}, Humidity: {humidity}")
        except (ValueError, IndexError) as e:
            print("Failed to parse temperature and humidity from payload!", e)

    elif topic == "sensors/humidity":  # Handle humidity separately
        try:
            humidity = float(payload)
            c.execute("INSERT INTO environment (temperature, humidity) VALUES (?, ?)", (None, humidity))
            print(f"Logged Humidity Only: {humidity}")
        except ValueError as e:
            print("Failed to parse humidity from payload!", e)

    elif topic == "sensors/temperature":  # Handle temperature separately
        try:
            temperature = float(payload)
            c.execute("INSERT INTO environment (temperature, humidity) VALUES (?, ?)", (temperature, None))
            print(f"Logged Temperature Only: {temperature}")
        except ValueError as e:
            print("Failed to parse temperature from payload!", e)

    conn.commit()

# MQTT Setup
client = mqtt.Client()
client.on_message = on_message

client.connect("localhost", 1883, 60)  # Change "localhost" if your MQTT broker is on a different machine
client.subscribe("sensors/pulse")
client.subscribe("sensors/body_temperature")
client.subscribe("sensors/temperature_humidity")  # Subscribe to the combined topic
client.subscribe("sensors/humidity")  # Subscribe to the humidity topic
client.subscribe("sensors/temperature")  # Subscribe to the temperature topic

# Run MQTT loop
client.loop_forever()