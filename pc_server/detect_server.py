import time
import threading
import requests
import cv2
import numpy as np

from fastapi import FastAPI
from fastapi.responses import Response, JSONResponse, HTMLResponse
from ultralytics import YOLO


ESP_FRAME_URL =  "http://10.38.98.185/frame"
MODEL_PATH = "yolo11n.pt"

app = FastAPI()
model = YOLO(MODEL_PATH)

state_lock = threading.Lock()
state = {
    "jpg": None,
    "result": {
        "status": "starting",
        "detections": []
    }
}


def detect_loop():
    while True:
        try:
            response = requests.get(
                ESP_FRAME_URL,
                timeout=1.0)

            response.raise_for_status()

            image_array = np.frombuffer(
                response.content,
                dtype=np.uint8)

            frame = cv2.imdecode(
                image_array,
                cv2.IMREAD_COLOR)

            if frame is None:
                continue

            result = model.predict(
                frame,
                imgsz=320,
                conf=0.30,
                verbose=False)[0]

            detections = []

            for box in result.boxes:
                xyxy = box.xyxy[0].cpu().numpy()
                class_id = int(box.cls[0].item())
                score = float(box.conf[0].item())

                x1, y1, x2, y2 = [
                    int(value) for value in xyxy
                ]

                label = result.names[class_id]

                detections.append({
                    "class_id": class_id,
                    "label": label,
                    "score": score,
                    "x1": x1,
                    "y1": y1,
                    "x2": x2,
                    "y2": y2
                })

                text = f"{label} {score:.2f}"

                cv2.rectangle(
                    frame,
                    (x1, y1),
                    (x2, y2),
                    (0, 255, 0),
                    2)

                cv2.putText(
                    frame,
                    text,
                    (x1, max(y1 - 8, 15)),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.5,
                    (0, 255, 0),
                    2)

            ok, encoded = cv2.imencode(".jpg", frame)
            if not ok:
                continue

            with state_lock:
                state["jpg"] = encoded.tobytes()
                state["result"] = {
                    "status": "ok",
                    "detections": detections
                }

        except Exception as error:
            with state_lock:
                state["result"] = {
                    "status": "error",
                    "message": str(error),
                    "detections": []
                }

            time.sleep(0.5)


@app.on_event("startup")
def start_detector():
    thread = threading.Thread(
        target=detect_loop,
        daemon=True)

    thread.start()


@app.get("/")
def index():
    return HTMLResponse("""
    <html>
    <body style="background:#222;color:white">
        <h2>PC YOLO Detection</h2>
        <img id="image" width="640">
        <pre id="result"></pre>

        <script>
        const image = document.getElementById("image");
        const result = document.getElementById("result");

        setInterval(() => {
            image.src = "/latest.jpg?t=" + Date.now();

            fetch("/latest.json")
                .then(r => r.json())
                .then(data => {
                    result.textContent =
                        JSON.stringify(data, null, 2);
                });
        }, 200);
        </script>
    </body>
    </html>
    """)


@app.get("/latest.jpg")
def latest_jpg():
    with state_lock:
        jpg = state["jpg"]

    if jpg is None:
        return Response(
            status_code=503,
            content=b"no frame")

    return Response(
        content=jpg,
        media_type="image/jpeg")


@app.get("/latest.json")
def latest_json():
    with state_lock:
        return JSONResponse(state["result"])