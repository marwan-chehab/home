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

    expected_fields = [
		'system_status', 'pv1_voltage', 'pv2_voltage', 'pv1_charge_power', 'pv2_charge_power', 
		'buck1_current', 'buck2_current', 'output_active_power', 'output_apparent_power', 
		'ac_charge_power', 'ac_charge_apparent_power', 'battery_voltage', 'battery_soc', 'bus_voltage', 
		'grid_voltage', 'line_frequency', 'output_voltage', 'output_frequency', 'dc_output_voltage', 
		'inverter_temp', 'dcdc_temp', 'load_percent', 'battery_port_voltage', 'battery_bus_voltage', 
		'work_time_total', 'buck1_temp', 'buck2_temp', 'output_current', 'inverter_current', 'ac_input_power', 
		'ac_input_va', 'fault_bit', 'warning_bit', 'fault_value', 'warning_value', 'device_type_code', 
		'check_step', 'production_line_mode', 'constant_power_ok_flag', 'pv1_energy_today', 'pv1_energy_total', 
		'pv2_energy_today', 'pv2_energy_total', 'ac_charge_energy_today', 'ac_charge_energy_total', 
		'bat_discharge_energy_today', 'bat_discharge_energy_total', 'ac_discharge_energy_today', 
		'ac_discharge_energy_total', 'ac_charge_current', 'ac_discharge_watt', 'ac_discharge_va', 
		'bat_discharge_watt', 'bat_discharge_va', 'battery_watt', 'bat_overcharge_flag', 'mppt_fan_speed', 
		'inv_fan_speed', 'bms_status', 'bms_error', 'bms_warn_info', 'bms_soc', 'bms_battery_volt', 'bms_battery_curr', 
		'bms_battery_temp', 'bms_max_curr', 'bms_constant_volt', 'bms_info', 'bms_pack_info', 'bms_using_cap', 
		'bms_cell1_volt', 'bms2_cell16_volt', 'bms2_status', 'bms2_error', 'bms2_warn_info', 'bms2_soc', 
		'bms2_battery_volt', 'bms2_battery_curr', 'bms2_battery_temp', 'bms2_max_curr', 'bms2_constant_volt', 
		'bms2_info', 'bms2_pack_info', 'bms2_using_cap', 'bms2_cell1_volt', 'bms2_cell16_volt', 'solar1_status', 
		'solar1_fault_code', 'solar1_warning_code', 'solar1_bat_voltage', 'solar1_pv1_voltage', 'solar1_pv2_voltage', 
		'solar1_buck1_current', 'solar1_buck2_current', 'solar1_pv1_charge_power', 'solar1_pv2_charge_power', 
		'solar1_hs1_temp', 'solar1_hs2_temp', 'solar1_epv1_today', 'solar1_epv2_today', 'solar1_epv1_total', 
		'solar1_epv2_total', 'solar2_status', 'solar2_fault_code', 'solar2_warning_code', 'solar2_bat_voltage', 
		'solar2_pv1_voltage', 'solar2_pv2_voltage', 'solar2_buck1_current', 'solar2_buck2_current', 
		'solar2_pv1_charge_power', 'solar2_pv2_charge_power', 'solar2_hs1_temp', 'solar2_hs2_temp', 
		'solar2_epv1_today', 'solar2_epv2_today', 'solar2_epv1_total', 'solar2_epv2_total', 
		'solar_connect_ok_flag', 'solar_batvolt_consist_flag', 'solar_type_switch_state', 
		'solar_mode_switch_state', 'solar_address_switch_state', 'bms_gauge_rm', 'bms_gauge_fcc', 
		'bms_fw', 'bms_delta_volt', 'bms_cycle_count', 'bms_soh', 'bms_ic_current', 'bms_mcu_version', 
		'bms_gauge_version', 'bms_fr_version', 'bms2_gauge_rm', 'bms2_gauge_fcc', 'bms2_fw', 
		'bms2_delta_volt', 'bms2_cycle_count', 'bms2_soh', 'bms2_ic_current', 'bms2_mcu_version', 
		'bms2_gauge_version', 'bms2_fr_version', 'inverter_id']

    missing = [field for field in expected_fields if field not in data]
    if missing:
        return jsonify({"error": f"Missing fields: {', '.join(missing)}"}), 400

    connection = create_connection()
    if not connection:
        return jsonify({"error": "Database connection failed"}), 500

    placeholders = ", ".join(["%s"] * len(expected_fields))
    fields_str = ", ".join(expected_fields)

    sql = f"INSERT INTO growatt ({fields_str}) VALUES ({placeholders})"
    values = tuple(data[field] for field in expected_fields)

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
