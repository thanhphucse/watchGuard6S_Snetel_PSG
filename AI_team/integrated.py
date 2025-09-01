from flask import Flask, render_template, Response
import cv2
from ultralytics import YOLO
import atexit

app = Flask(_name_)

# Load YOLOv8 model
model = YOLO(r"C:\My projects\SNETEL\best.pt")

# Open webcam
camera = cv2.VideoCapture(0)  # 0 = default webcam

# Ensure camera is released when the program exits
atexit.register(lambda: camera.release())

def generate_frames():
    while True:
        success, frame = camera.read()
        if not success:
            break

        # Optional: resize frame for faster processing
        frame = cv2.resize(frame, (640, 480))

        # Run YOLOv8 inference
        results = model(frame)

        # Plot detections (if any)
        annotated_frame = results[0].plot() if len(results[0].boxes) > 0 else frame

        # Encode frame as JPEG
        ret, buffer = cv2.imencode('.jpg', annotated_frame)
        frame_bytes = buffer.tobytes()

        # Stream frame to browser
        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n' + frame_bytes + b'\r\n')

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/video')
def video():
    return Response(generate_frames(),
                    mimetype='multipart/x-mixed-replace; boundary=frame')

if _name_ == "_main_":
    # Threaded=True for smooth video streaming
    app.run(debug=True, threaded=True)
