import socket

print("Client application started")

try:
    # Create a TCP socket
    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # Set connection timeout
    client.settimeout(2)

    # Try connecting to the server
    client.connect(("127.0.0.1", 50007))

    print("Connection successful!")

    # Receive data from server
    message = client.recv(1024)

    print("Server response:", message.decode())

    client.close()

    print("Application completed successfully")

except ConnectionRefusedError:
    print("ERROR: Server is not available!")
    print("Application failed.")

    exit(1)

except socket.timeout:
    print("ERROR: Connection timed out!")
    exit(1)
