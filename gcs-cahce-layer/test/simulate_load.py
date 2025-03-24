import requests
import threading
import time
from concurrent.futures import ThreadPoolExecutor

SERVER_URL = "http://localhost:8080/object/test_file.txt"
REQUESTS_PER_CLIENT = 50
CLIENT_COUNT = 10
NOISY_CLIENT_ID = "noisy"
POLITE_CLIENT_PREFIX = "client"

def fetch_object(client_id):
    try:
        params = {
            "bucket": "my_bucket",
            "file": "test_file.txt",
            "client_id": client_id
        }
        start = time.time()
        r = requests.get(SERVER_URL, params=params)
        duration = time.time() - start
        if r.status_code == 200:
            print(f"[OK] {client_id} received {len(r.content)} bytes in {duration:.2f}s")
        elif r.status_code == 429:
            print(f"[THROTTLED] {client_id} got 429 in {duration:.2f}s")
        else:
            print(f"[ERROR] {client_id} got {r.status_code} in {duration:.2f}s")
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
    # Run both tests in parallel
    noisy_thread = threading.Thread(target=run_noisy_client)
    noisy_thread.start()

    run_polite_clients()
    noisy_thread.join()

if __name__ == "__main__":
    main()

