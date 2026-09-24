#!/usr/bin/env python3
from time import sleep
from ev3dev2.motor import LargeMotor, MediumMotor, OUTPUT_A, OUTPUT_B
from ev3dev2.sensor.lego import ColorSensor
from ev3dev2.sensor import INPUT_1

# Initialize hardware components
drive_motor = LargeMotor(OUTPUT_B)
steering_motor = MediumMotor(OUTPUT_A)
color_sensor = ColorSensor(INPUT_1)

# Set color sensor mode to read color IDs (e.g., 0=None, 1=Black, 3=Green, 5=Red)
color_sensor.mode = 'COL-COLOR'

def main():
    print("Robot running with color sensor active...")
    
    # Center steering and start moving forward
    steering_motor.run_to_abs_pos(position_sp=0, speed_sp=300, stop_action="hold")
    drive_motor.run_forever(speed_sp=300)
    
    try:
        while True:
            # Read the current color value
            detected_color = color_sensor.color
            print(f"Current color ID: {detected_color}")
            
            # Example action based on color detection
            if detected_color == 5:  # For instance, if Red is detected
                print("Red detected! Adjusting path...")
                # Add your turn or stop logic here
                
            sleep(0.05)
            
    except KeyboardInterrupt:
        # Safely stop motors on exit
        drive_motor.stop()
        steering_motor.stop()
        print("Program stopped safely.")

if __name__ == '__main__':
    main()