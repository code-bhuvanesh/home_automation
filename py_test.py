import bluetooth

# Replace with the MAC address of your ESP32
target_device = 'C8:F0:9E:A6:C4:66'

def send_message(sock, message):
    sock.send(message.encode())
    

def main():
    try:
        # Create a Bluetooth socket
        sock = bluetooth.BluetoothSocket(bluetooth.RFCOMM)

        # Connect to the target Bluetooth device
        sock.connect((target_device, 1))  # RFCOMM channel 1

        # while True:
            # message = input("Enter message to send (or 'exit' to quit): ")
            # message += "\n" + input("Enter message to send (or 'exit' to quit): ")
            # if message.lower() == 'exit': 
            #     break
        print("connected")
        input()
        print("sending wifi detials")
        send_message(sock, "XY9FJBe2bADvNMduSwdzJXVlovzD12Vu\n")
        send_message(sock, "JioFiber-EgRrr\n")
        send_message(sock, "deva9840raj\n")
        # send_message(sock, "deva9840raj")
        print("Message sent.")

        # Close the socket
        sock.close()

    except bluetooth.BluetoothError as e:
        print("Error:", e)

if __name__ == '__main__':
    main()