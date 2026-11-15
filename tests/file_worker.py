import os
import time

print("File Processing Worker Started")

# Check whether the file is available
if not os.path.exists("input.txt"):
    print("Input file not found!")
    exit(1)

# Read the input file
with open("input.txt", "r") as file:
    data = file.read()

print("File read successfully!")
print("File content:", data)

print("Processing completed!")
