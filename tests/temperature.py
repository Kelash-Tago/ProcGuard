import random
import time

print("Temperature Monitor Started")

temperature = random.randint(20, 40)

print("Current Temperature:", temperature)

time.sleep(2)

if temperature > 30:
    print("Sensor error! Application stopped.")
    exit(1)

print("Temperature recorded successfully!")
