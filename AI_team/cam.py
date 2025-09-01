import cv2
import tensorflow as tf
import numpy as np

# Initialize webcam (0 is the default camera)
cap = cv2.VideoCapture(0)

if not cap.isOpened():
    print("Error: Cannot open webcam.")
    exit()

print("Webcam opened successfully.")

while True:
    ret, frame = cap.read()
    if not ret:
        print("Failed to grab frame.")
        break

    # Resize frame if needed
    resized_frame = cv2.resize(frame, (224, 224))

    # Convert to tensor (optional - for TensorFlow model)
    input_tensor = tf.convert_to_tensor(resized_frame, dtype=tf.uint8)
    input_tensor = tf.expand_dims(input_tensor, axis=0)  # (1, 224, 224, 3)

    # Optional: You can pass input_tensor to a model here

    # Show the frame
    cv2.imshow("Webcam Feed", frame)

    # Exit when 'q' key is pressed
    if cv2.waitKey(1) & 0xFF == ord('q'):
        print("Exiting...")
        break

# Release and cleanup
cap.release()
cv2.destroyAllWindows()
