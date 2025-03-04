from flask import Flask, render_template, request, redirect, url_for, jsonify
import mysql.connector
from mysql.connector import Error  # Import Error properly
from datetime import datetime  # ✅ Import datetime

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

@app.route('/burner')
def burner():
    print("Getting burner...")

    try:
        connection = create_connection()
        if not connection:
            print("Failed to connect to the database")
            return jsonify({"error": "Error connecting to database"}), 500
        
        try:
            cursor = connection.cursor(dictionary=True)
            cursor.execute("SELECT * FROM burner ORDER BY bur_id DESC LIMIT 1")
            result = cursor.fetchone()
            connection.close()

            if result:
                # ✅ Convert datetime fields to strings before returning
                if "stamp" in result and isinstance(result["stamp"], datetime):
                    result["stamp"] = result["stamp"].strftime("%Y-%m-%d %H:%M:%S")
                
                print(f"onoff: {result['onoff']}")  # Debugging
                print("Returning result to browser:", result)
                return jsonify(result)  # Make sure result is JSON serializable
            else:
                print("No data found")
                return jsonify({"error": "No data found"}), 404

        except Error as e:
            print(f"Error while executing query: {e}")
            return jsonify({"error": "Error executing query"}), 500

    except Exception as e:
        print(f"An unexpected error occurred: {e}")
        return jsonify({"error": "An unexpected error occurred"}), 500
        

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
