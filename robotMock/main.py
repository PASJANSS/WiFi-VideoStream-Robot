import asyncio
import websockets
import json

async def send(ws):
    while True:
        await ws.send(bts)
        await asyncio.sleep(0.033)

async def receive(ws):
    while True:
        msg = await ws.recv()
        data = json.loads(msg)
        print('dostalem info zeby ruszyc ' + data["key"])

async def connect():
    global bts

    ws = await websockets.connect("ws://localhost:8080/ws/robot", ping_interval=None)
    try:
        await asyncio.gather(
            send(ws),
            receive(ws)
        )
    except:
        await ws.close()
        await asyncio.sleep(2)
        print("disconnected\n")
        await connect()

photo = open("img.jpg", "rb")
bts = photo.read()
asyncio.run(connect())