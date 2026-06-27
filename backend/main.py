from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.responses import StreamingResponse
import json
import asyncio

isClientConnected = False
isRobotConnected = False
robotWs = None
lastFrame = None

#client section
app = FastAPI()
@app.websocket("/ws/client")
async def client_endpoint(ws: WebSocket):
    global isClientConnected, isRobotConnected

    if isClientConnected:
        print('only 1 client can connect to server at one moment')
        return
    await ws.accept()
    isClientConnected = True

    try:
        await asyncio.gather( #receiving and sending data at the same time
            handleSendClient(ws),
            handleReceiveClient(ws)
        )
    except WebSocketDisconnect:
        isClientConnected = False
        print("client disconnected")

async def handleSendClient(ws: WebSocket):
    global isRobotConnected
    while True:
        await ws.send_text(json.dumps({"robotConnected": isRobotConnected}))
        await asyncio.sleep(0.033)

async def handleReceiveClient(ws: WebSocket):
    while True:
        input = await ws.receive_text()
        print("received " + input)
        await clientInput(input)
        
async def clientInput(input):
    global robotWs, isRobotConnected

    if isRobotConnected == False:
        return
    await robotWs.send_text(input)


#robot section
@app.websocket("/ws/robot")
async def robot_endpoint(ws: WebSocket):
    global isClientConnected, isRobotConnected, robotWs, lastFrame

    if isRobotConnected:
        return
    await ws.accept()
    isRobotConnected = True
    robotWs = ws

    try:
        while True:
            lastFrame = await ws.receive_bytes() #odbieram to co robot mi mowi
            print("got bytes from robot")
    except WebSocketDisconnect:
        isRobotConnected = False
        robotWs = None
        print("robot disconnected")
        
@app.get("/api/stream")
async def stream():
    async def generate():
        while True:
            if lastFrame:
                yield (
                    b"--frame\r\n"
                    b"Content-Type: image/jpeg\r\n\r\n"
                    + lastFrame +
                    b"\r\n"
                )
            await asyncio.sleep(0.033)  # ~30fps max
    
    return StreamingResponse(
        generate(),
        media_type="multipart/x-mixed-replace; boundary=frame"
    )