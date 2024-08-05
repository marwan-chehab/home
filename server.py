from flask import Flask, request, jsonify
import mysql.connector
from mysql.connector import Error

app = Flask(__name__)

def create_connection():
    """ Create a MySQL database connection. """
    try:
        connection = mysql.connector.connect(
            host='35.180.203.161', 
            user='root', 
            password='j2chj2+/Boz4',
            database='home' 
        )
        if connection.is_connected():
            print("Successfully connected to the database")
            return connection
    except Error as e:
        print(f"Error while connecting to MySQL: {e}")
        return None

@app.route('/insert', methods=['POST'])
def insert_row():
    """ Insert a row into the ampere table. """
    ampere = request.json['ampere']
    heater = request.json['heater']
    warehouse = request.json['warehouse']
    connection = create_connection()
    if connection is not None:
        try:
            cursor = connection.cursor()
            sql_insert_query = "INSERT INTO ampere (ampere, heater, warehouse) VALUES (%s, %s, %s)"
            cursor.execute(sql_insert_query, (ampere, heater, warehouse))
            connection.commit()
            return jsonify({"message": "Row inserted successfully"}), 200
        except Error as e:
            return jsonify({"message": f"Error while inserting into MySQL table: {e}"}), 500
        finally:
            if connection.is_connected():
                connection.close()
                print("MySQL connection is closed")
    return jsonify({"message": "Error while connecting to MySQL"}), 500

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
