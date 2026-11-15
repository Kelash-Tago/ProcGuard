import os
import time

if not os.path.exists("success.txt"):
    print("Temporary error! File not found.")
    open("success.txt", "w").close()
    exit(1)

print("File found!")
print("Program completed successfully!")
