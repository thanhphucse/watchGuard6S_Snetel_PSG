from flask import Flask, render_template, Response
import cv2
import tensorflow as tf
import numpy as np

app = Flask(_name_)

# Load pre-trained MobileNetV2
model = tf.keras.applications.MobileNetV2(weights='imagenet')

def generate_frames():
    cap = cv2.VideoCapture(0)  # Open camera here
    try:
        while True:
            success, frame = cap.read()
            if not success:
                break

            # Preprocess for MobileNetV2
            resized_frame = cv2.resize(frame, (224, 224))
            resized_frame = resized_frame / 255.0  # normalize
            input_tensor = tf.convert_to_tensor(resized_frame, dtype=tf.float32)
            input_tensor = tf.expand_dims(input_tensor, axis=0)

            # Optional: Run inference
            preds = model.predict(input_tensor)
            decoded = tf.keras.applications.mobilenet_v2.decode_predictions(preds, top=1)[0]
            label = decoded[0][1]  # class name

            # Draw label on frame
            cv2.putText(frame, label, (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)

            # Encode frame for streaming
            ret, buffer = cv2.imencode('.jpg', frame)
            frame = buffer.tobytes()

            yield (b'--frame\r\n'
                   b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')
    finally:
        cap.release()  # Always release the camera

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/video')
def video():
    return Response(generate_frames(), mimetype='multipart/x-mixed-replace; boundary=frame')

if _name_ == "_main_":
    app.run(debug=True)
