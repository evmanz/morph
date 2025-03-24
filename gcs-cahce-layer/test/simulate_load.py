import requests
import threading
import time
import random
import argparse
from concurrent.futures import ThreadPoolExecutor

# === Config ===
SERVER_URL = "http://localhost:8080/object"
BUCKET_NAME = "my_bucket"
REQUESTS_PER_CLIENT = 50
CLIENT_COUNT = 10
NOISY_CLIENT_ID = "noisy"
POLITE_CLIENT_PREFIX = "client"

# === File Pool ===
def get_random_file():
    return f"random_file_{random.randint(1, 10)}.bin"

# === Request Logic ===
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

# === Load Patterns ===
def run_noisy_client():
    print("🚨 Running noisy neighbor test...")
    for _ in range(REQUESTS_PER_CLIENT):
        fetch_object(NOISY_CLIENT_ID)
        time.sleep(0.01)

def run_polite_clients():
    print("🤝 Running polite clients test...")
    with ThreadPoolExecutor(max_workers=CLIENT_COUNT) as executor:
        futures = []
        for i in range(CLIENT_COUNT):
            client_id = f"{POLITE_CLIENT_PREFIX}_{i}"
            for _ in range(REQUESTS_PER_CLIENT):
                futures.append(executor.submit(fetch_object, client_id))
        for future in futures:
            future.result()

# === Main CLI Control ===
def main():
    parser = argparse.ArgumentParser(description="Simulate GCS cache load")
    parser.add_argument(
        "-s", "--scenario",
        choices=["noisy", "polite", "both"],
        required=True,
        help="Test scenario to run: 'noisy', 'polite', or 'both'"
    )
    args = parser.parse_args()

    if args.scenario == "noisy":
        run_noisy_client()
    elif args.scenario == "polite":
        run_polite_clients()
    elif args.scenario == "both":
        noisy_thread = threading.Thread(target=run_noisy_client)
        noisy_thread.start()
        run_polite_clients()
        noisy_thread.join()

if __name__ == "__main__":
    main()
