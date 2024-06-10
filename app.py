# app.py

from flask import Flask, jsonify, render_template
import mysql.connector
from mysql.connector import Error

app = Flask(__name__)

def create_connection():
    print("Attempting to connect to the database...")
    try:
        connection = mysql.connector.connect(
            host='35.180.203.161',
            user='root',       # Replace with your remote username
            password='j2chj2+/Boz4', # Replace with your remote password
            database='home'
        )
        if connection.is_connected():
            print("Database connection successful")
            return connection
    except Error as e:
        print(f"Error while connecting to MySQL: {e}")
        return None

@app.route('/')
def index():
    print("Rendering index page")
    return render_template('index.html')

@app.route('/data')
def data():
    print("Fetching data...")
    try:
        connection = create_connection()
        if connection:
            try:
                cursor = connection.cursor(dictionary=True)
                cursor.execute("SELECT * FROM ampere ORDER BY date DESC LIMIT 1")
                result = cursor.fetchone()
                connection.close()
                if result:
                    print(f"Current ampere reading: {result['ampere']} at {result['date']}")
                else:
                    print("No data found")
                return jsonify(result)
            except Error as e:
                print(f"Error while executing query: {e}")
                return jsonify({"error": "Error executing query"})
        else:
            print("Failed to connect to the database")
            return jsonify({"error": "Error connecting to database"})
    except Exception as e:
        print(f"An unexpected error occurred: {e}")
        return jsonify({"error": "An unexpected error occurred"})

if __name__ == "__main__":
    print("Starting Flask app...")
    app.run(host='0.0.0.0', port=5000, debug=True)
