from flask import Flask, render_template, request, redirect, url_for, jsonify
import mysql.connector
from mysql.connector import Error
from datetime import datetime

app = Flask(__name__)

# Function to create a database connection
def create_connection():
    try:
        connection = mysql.connector.connect(
            host="localhost",
            user="root",
            password="j2chj2+/Boz4",
            database="home"
        )
        return connection
    except Error as e:
        print(f"Error connecting to database: {e}")
        return None

def get_latest_status():
    connection = create_connection()
    if connection:
        try:
            cursor = connection.cursor(dictionary=True)
            query = """
                SELECT dc.device_type, dc.status, dc.stamp AS latest_stamp
                FROM device_control dc
                WHERE dc.stamp = (
                    SELECT MAX(stamp)
                    FROM device_control
                    WHERE device_type = dc.device_type
                )
                ORDER BY dc.device_type;
            """
            cursor.execute(query)
            devices = cursor.fetchall()

            # Convert datetime objects to strings
            for device in devices:
                if "latest_stamp" in device and isinstance(device["latest_stamp"], datetime):
                    device["latest_stamp"] = device["latest_stamp"].strftime("%Y-%m-%d %H:%M:%S")

            cursor.close()
            connection.close()
            return devices
        except Error as e:
            print(f"Error fetching device statuses: {e}")
    return []

@app.route('/get_device_statuses')
def get_device_statuses():
    devices = get_latest_status()
    return jsonify(devices)  # ✅ Ensures it returns JSON instead of HTML

@app.route('/update_status')
def update_status():
    devices = get_latest_status()
    return render_template('update_status.html', devices=devices)

@app.route('/submit_status', methods=['POST'])
def submit_status():
    device_type = request.form.get("device_type")
    new_status = request.form.get("onoff")

    connection = create_connection()
    if not connection:
        return "Error: Database connection failed!", 500

    try:
        query = "INSERT INTO device_control (device_type, status) VALUES (%s, %s)"
        cursor = connection.cursor()
        cursor.execute(query, (device_type, new_status))
        connection.commit()
        cursor.close()
        connection.close()
        return redirect(url_for('update_status'))
    except Error as e:
        print(f"Error while executing query: {e}")
        return f"Error executing query: {str(e)}", 500

@app.route('/device/<device_type>')
def device(device_type):
    try:
        connection = create_connection()
        if not connection:
            return jsonify({"error": "Error connecting to database"}), 500
        
        try:
            cursor = connection.cursor(dictionary=True)
            query = "SELECT * FROM device_control WHERE device_type = %s ORDER BY id DESC LIMIT 1"
            cursor.execute(query, (device_type,))
            result = cursor.fetchone()
            connection.close()

            if result:
                if "stamp" in result and isinstance(result["stamp"], datetime):
                    result["stamp"] = result["stamp"].strftime("%Y-%m-%d %H:%M:%S")
                return jsonify(result)
            else:
                return jsonify({"error": f"No data found for {device_type}"}), 404

        except Error as e:
            print(f"Error while executing query for {device_type}: {e}")
            return jsonify({"error": "Error executing query"}), 500

    except Exception as e:
        print(f"An unexpected error occurred: {e}")
        return jsonify({"error": "An unexpected error occurred"}), 500

@app.route('/growatt', methods=['POST'])
def growatt():
    data = request.get_json()

    required_fields = [
        'inverter_id', 'battery_voltage', 'battery_soc', 'output_current',
        'inverter_current', 'inverter_temp', 'fan_speed_1', 'fan_speed_2',
        'pv_input_power', 'grid_voltage', 'line_frequency', 'output_voltage',
        'output_frequency', 'ac_charge_current', 'solar_buck1_current',
        'solar_buck2_current', 'total_solar_charge_current'
    ]

    if not all(field in data for field in required_fields):
        return jsonify({"error": "Missing one or more required fields"}), 400

    connection = create_connection()
    if not connection:
        return jsonify({"error": "Database connection failed"}), 500

    sql = """
    INSERT INTO growatt (
        inverter_id, battery_voltage, battery_soc, output_current,
        inverter_current, inverter_temp, fan_speed_1, fan_speed_2,
        pv_input_power, grid_voltage, line_frequency, output_voltage,
        output_frequency, ac_charge_current, solar_buck1_current,
        solar_buck2_current, total_solar_charge_current
    ) VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
    """
    values = tuple(data[field] for field in required_fields)

    try:
        cursor = connection.cursor()
        cursor.execute(sql, values)
        connection.commit()
        cursor.close()
        connection.close()
        return jsonify({"status": "success"}), 200
    except Error as e:
        print(f"Database error: {e}")
        return jsonify({"error": str(e)}), 500


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
