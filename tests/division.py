import random
import time

print("Division Application Started")

# Generate two random numbers
num1 = random.randint(10, 100)
num2 = random.randint(0, 1)

print("First number:", num1)
print("Second number:", num2)

time.sleep(1)

try:
    # Divide the numbers
    result = num1 / num2

    print("Result:", result)
    print("Division completed successfully!")

except ZeroDivisionError:
    print("Error: Cannot divide by zero!")

    # Exit with failure status
    exit(1)
