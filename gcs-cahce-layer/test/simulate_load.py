import requests
import threading
import time
import random
from concurrent.futures import ThreadPoolExecutor

SERVER_URL = "http://localhost:8080/object"
REQUESTS_PER_CLIENT = 50
CLIENT_COUNT = 10
NOISY_CLIENT_ID = "noisy_client"
POLITE_CLIENT_PREFIX = "polite_client"
BUCKET_NAME = "my_bucket"

def get_random_file():
    return f"random_file_{random.randint(1, 10)}.bin"

def fetch_object(client_id):
    try:
        file_name = get_random_file()
        params = {
            "bucket": BUCKET_NAME,
            "file": file_name,
            "client_id": client_id
        }
        start = time.time()
        r = requests.get(SERVER_URL, params=params)
        duration = time.time() - start

        if r.status_code == 200:
            print(f"[OK] {client_id} fetched {file_name} ({len(r.content)} bytes) in {duration:.2f}s")
        elif r.status_code == 429:
            print(f"[THROTTLED] {client_id} got 429 for {file_name} in {duration:.2f}s")
        else:
            print(f"[ERROR] {client_id} got {r.status_code} for {file_name} in {duration:.2f}s")
    except Exception as e:
        print(f"[FAIL] {client_id} exception: {e}")

def run_noisy_client():
    print("Starting noisy neighbor test...")
    for _ in range(REQUESTS_PER_CLIENT):
        fetch_object(NOISY_CLIENT_ID)
        time.sleep(0.01)  # hammer the server fast

def run_polite_clients():
    print("Starting max load test with polite clients...")
    with ThreadPoolExecutor(max_workers=CLIENT_COUNT) as executor:
        futures = []
        for i in range(CLIENT_COUNT):
            client_id = f"{POLITE_CLIENT_PREFIX}_{i}"
            for _ in range(REQUESTS_PER_CLIENT):
                futures.append(executor.submit(fetch_object, client_id))
        for future in futures:
            future.result()

def main():
    noisy_thread = threading.Thread(target=run_noisy_client)
    noisy_thread.start()

    run_polite_clients()
    noisy_thread.join()

if __name__ == "__main__":
    main()
