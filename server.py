from flask import Flask, render_template, request, redirect, url_for
import mysql.connector

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
    except mysql.connector.Error as e:
        print(f"Error connecting to database: {e}")
        return None

# Get the latest burner status
def get_latest_status():
    connection = create_connection()
    if connection:
        try:
            cursor = connection.cursor()
            cursor.execute("SELECT onoff FROM burner ORDER BY bur_id DESC LIMIT 1")
            result = cursor.fetchone()
            cursor.close()
            connection.close()
            return result[0] if result else "off"  # Default to "off" if no data
        except mysql.connector.Error as e:
            print(f"Error fetching status: {e}")
    return "off"  # Return default value if error occurs

@app.route('/update_status')
def update_status():
    current_status = get_latest_status()
    return render_template('update_status.html', current_status=current_status)

@app.route('/submit_status', methods=['POST'])
def submit_status():
    current_status = get_latest_status()
    new_status = "off" if current_status == "on" else "on"  # Toggle status

    connection = create_connection()
    if not connection:
        return "Error: Database connection failed!", 500

    try:
        query = "INSERT INTO burner (onoff) VALUES (%s)"
        cursor = connection.cursor()
        cursor.execute(query, (new_status,))
        connection.commit()
        cursor.close()
        connection.close()
        return redirect(url_for('update_status'))  # Redirect to update page
    except mysql.connector.Error as e:
        print(f"Error while executing query: {e}")
        return f"Error executing query: {str(e)}", 500
    except Exception as e:
        print(f"Unexpected error: {e}")
        return f"Unexpected error: {str(e)}", 500

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
