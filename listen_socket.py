import websocket
import _thread
import time
import rel
from websocket import create_connection


socketAddress = "ws://192.168.29.120/ws"

# def on_message(ws, message):
#     print(message)

# def on_error(ws, error):
#     print(error)

# def on_close(ws, close_status_code, close_msg):
#     print("### closed ###")

# def on_open(ws):
#     print("Opened connection")

# if __name__ == "__main__":
#     websocket.enableTrace(True)
#     ws = websocket.WebSocketApp(socketAddress,
#                               on_open=on_open,
#                               on_message=on_message,
#                               on_error=on_error,
#                               on_close=on_close)

#     ws.run_forever(dispatcher=rel, reconnect=5)  # Set dispatcher to automatic reconnection, 5 second reconnect delay if connection closed unexpectedly
#     rel.signal(2, rel.abort)  # Keyboard Interrupt
#     rel.dispatch()


ws = create_connection(socketAddress)
while True:
    print(ws.recv())
ws.close()