from flask import Flask, render_template, Response, jsonify
import cv2
from ultralytics import YOLO
import atexit
app = Flask(__name__)
# Load YOLOv8 model
model = YOLO(r"C:\My projects\SNETEL\best.pt")
# Open webcam
camera = cv2.VideoCapture(0)  # 0 = default webcam
# Ensure camera is released when the program exits
atexit.register(lambda: camera.release())
# Store latest detection globally
latest_detection = {"class": None, "confidence": None}
def generate_frames():
    global latest_detection
    while True:
        success, frame = camera.read()
        if not success:
            break
        # Resize frame for speed
        frame = cv2.resize(frame, (640, 480))
        # Run YOLOv8 inference
        results = model(frame)
        if len(results[0].boxes) > 0:
            # Get the highest-confidence detection
            best_box = results[0].boxes[0]
            cls_id = int(best_box.cls)
            conf = float(best_box.conf)
            class_name = model.names[cls_id]
            latest_detection = {
                "class": class_name,
                "confidence": round(conf, 2)
            }
        # Draw detections on frame
        annotated_frame = results[0].plot()
        # Encode frame
        ret, buffer = cv2.imencode('.jpg', annotated_frame)
        frame_bytes = buffer.tobytes()
        # Stream to client
        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n' + frame_bytes + b'\r\n')
@app.route('/')
def index():
    return render_template('text.html')
@app.route('/video')
def video():
    return Response(generate_frames(),
                    mimetype='multipart/x-mixed-replace; boundary=frame')


@app.route('/latest_detection')
def latest_detection_api():
    return jsonify(latest_detection)


if __name__ == "__main__":
    # Threaded=True for smoother streaming
    app.run(debug=True, threaded=True)
