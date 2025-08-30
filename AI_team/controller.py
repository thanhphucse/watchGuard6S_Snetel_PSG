from flask import Flask, request, jsonify, render_template, send_from_directory
from ultralytics import YOLO
import os

# Load YOLO model
model = YOLO(r"C:\snetel-proj\snetel\runs\detect\train4\weights\best.pt")

# Flask app
app = Flask(__name__)

# Ensure output folder exists
OUTPUT_FOLDER = os.path.join("static", "outputs")
os.makedirs(OUTPUT_FOLDER, exist_ok=True)

@app.route('/')
def home():
    return render_template("index.html")

@app.route('/predict', methods=['POST'])
def predict():
    if 'image' not in request.files:
        return "No file uploaded", 400
    
    file = request.files['image']
    filepath = os.path.join(OUTPUT_FOLDER, file.filename)
    file.save(filepath)

    # Run YOLO inference & save result
    results = model.predict(filepath, save=True, project=OUTPUT_FOLDER, name="results", exist_ok=True)

    # Get YOLO’s saved image path
    result_path = os.path.join(OUTPUT_FOLDER, "results", file.filename)

    return render_template("index.html", prediction_result=result_path)

@app.route('/static/outputs/results/<filename>')
def send_image(filename):
    return send_from_directory(os.path.join(OUTPUT_FOLDER, "results"), filename)

if __name__ == "__main__":
    app.run(debug=True)
