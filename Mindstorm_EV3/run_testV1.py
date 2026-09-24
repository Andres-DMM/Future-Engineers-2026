#!/usr/bin/env python3
from time import sleep
from ev3dev2.motor import LargeMotor, MediumMotor, OUTPUT_A, OUTPUT_B
from ev3dev2.sensor.lego import ColorSensor
from ev3dev2.sensor import INPUT_1

# Initialize hardware components
drive_motor = LargeMotor(OUTPUT_B)       # Rear large motor for driving
steering_motor = MediumMotor(OUTPUT_A)   # Steering medium motor
color_sensor = ColorSensor(INPUT_1)      # Downward color sensor

# Use RGB mode for precise color tracking (Values range usually from 0 to 1025)
color_sensor.mode = 'RGB-RAW'

def steer(angle, speed=300):
    """Helper function to safely adjust steering position"""
    steering_motor.run_to_abs_pos(position_sp=angle, speed_sp=speed, stop_action="hold")

def run_challenge():
    print("Starting WRO Challenge: Tracking Blue and Orange Lines...")
    
    # Start moving forward at a steady search speed
    drive_motor.run_forever(speed_sp=250)
    
    try:
        while True:
            # Read raw RGB values: (Red, Green, Blue)
            r, g, b = color_sensor.rgb
            
            # --- COLOR DETECTION LOGIC ---
            
            # 1. Check for Blue line (Blue value is significantly higher than Red and Green)
            if b > 100 and b > r * 1.5 and b > g * 1.5:
                print("Blue line detected! Executing turn...")
                
                # Action for Blue: Steer sharply or adjust course
                steer(angle=-35, speed=400) # Adjust angle as needed
                sleep(0.8) # Duration of the turn maneuver
                
                # Return to default counter-clockwise curving angle
                steer(angle=-20, speed=400)
                
            # 2. Check for Orange line (High Red, medium Green, low Blue)
            elif r > 150 and g > 80 and b < 50:
                print("Orange line detected! Executing adjustment...")
                
                # Action for Orange: Perform alternative adjustment
                steer(angle=15, speed=400)  # Counter-steer or change path
                sleep(0.8)
                
                # Return to default curving angle
                steer(angle=-20, speed=400)
                
            else:
                # Default running state: Maintain standard counter-clockwise curve
                steer(angle=-20)
                
            sleep(0.02)
            
    except KeyboardInterrupt:
        # Safety stop when stopping the script
        drive_motor.stop()
        steering_motor.stop()
        print("Robot stopped safely.")

if __name__ == '__main__':
    run_challenge()