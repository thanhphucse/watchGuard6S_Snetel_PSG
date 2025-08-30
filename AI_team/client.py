import requests

url = "http://127.0.0.1:5000/predict"
files = {"image": open(r"C:\snetel-proj\snetel\smokee2.jpeg", "rb")}  # replace with your test image path

response = requests.post(url, files=files)

print(response.json())
