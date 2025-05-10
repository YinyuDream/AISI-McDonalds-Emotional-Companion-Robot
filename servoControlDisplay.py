import serial
import time

def arduino_connect(port='COM3', baudrate=9600, timeout=1, max_retries=5, retry_delay=2):
    retries = 0
    while retries < max_retries:
        try:
            ser_connection = serial.Serial(port, baudrate, timeout=timeout)
            print(f"Successfully connected to Arduino on {port}")
            return ser_connection
        except serial.SerialException as e:
            retries += 1
            print(f"Error connecting to Arduino on {port}: {e}. Retrying ({retries}/{max_retries})...")
            if retries < max_retries:
                time.sleep(retry_delay)
        except Exception as e:
            print(f"An unexpected error occurred during Arduino connection attempt: {e}")
            return None # Exit on unexpected errors

    print(f"Failed to connect to Arduino on {port} after {max_retries} attempts.")
    return None

def map_value(value, from_min, from_max, to_min, to_max):
    # Map a value from one range to another
    return (value - from_min) * (to_max - to_min) / (from_max - from_min) + to_min

def main():
    # Connect to Arduino
    arduino = arduino_connect()
    print("Arduino connected")
    print(
        "Welcome to the servo control program!\n"
        "You can control the servo motors using the following commands:\n"
        "1. blink\n"
        "2. eye_movement\n"
        "3. happiness\n"
        "4. sadness\n"
        "5. anger\n"
        "6. surprise\n"
        "7. fear\n"
        "8. disgust\n"
        "9. neutral\n"
        "10. exit\n"
    )
    while True:
        type = input("Please input the instruction:")
        if type == "blink":
            arduino.write(0x01.to_bytes(1, byteorder='little'))
        elif type == "eye_movement":
            arduino.write(0x02.to_bytes(1, byteorder='little'))
            x_axis = input("Please input the normalized x-axis angle (-1 ~ 1):")
            y_axis = input("Please input the normalized y-axis angle (-1 ~ 1):")
            x_axis = int(map_value(float(x_axis), -1, 1, 80, 120))
            y_axis = int(map_value(float(y_axis), -1, 1, 160, 180))
            arduino.write(x_axis.to_bytes(1, byteorder='little'))
            arduino.write(y_axis.to_bytes(1, byteorder='little'))
        elif type == "happiness":
            arduino.write(0x11.to_bytes(1, byteorder='little'))
        elif type == "sadness" or type =="anger":
            arduino.write(0x12.to_bytes(1, byteorder='little'))
        elif type == "surprise" or type == "fear":
            arduino.write(0x13.to_bytes(1, byteorder='little'))
        elif type == "disgust" or type == "neutral":
            arduino.write(0x10.to_bytes(1, byteorder='little'))
        elif type == "exit":
            break
        else:
            print("Invalid type.")

if __name__ == "__main__":
    main()