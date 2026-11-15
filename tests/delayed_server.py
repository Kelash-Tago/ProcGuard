import socket
import time

print("Server is starting...")
print("Waiting 4 seconds before becoming available...")

# Simulate a server that is temporarily unavailable
time.sleep(4)

# Create a TCP server
server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Allow reuse of the port
server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

# Listen on localhost, port 50007
server.bind(("127.0.0.1", 50007))
server.listen(1)

print("Server is now READY!")

# Wait for a client connection
connection, address = server.accept()

print("Client connected:", address)

# Send a response to the client
connection.sendall(b"Hello! Server is working.")

connection.close()
server.close()

print("Server stopped.")
