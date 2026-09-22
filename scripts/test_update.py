import os
import hashlib
import urllib.request
import urllib.error
import json
import time

root_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
env_path = os.path.join(root_dir, ".env")
exe_path = os.path.join(root_dir, "dist", "ZemoServer", "ZemoServer.exe")

# 1. Read token
token = None
with open(env_path, "r", encoding="utf-8") as f:
    for line in f:
        if line.startswith("SERVER_UPDATE_TOKEN="):
            token = line.strip().split("=", 1)[1]
            break

if not token:
    print("Error: SERVER_UPDATE_TOKEN not found in .env")
    exit(1)

# 2. Read binary and compute SHA256
with open(exe_path, "rb") as f:
    data = f.read()

sha256 = hashlib.sha256(data).hexdigest()
size_mb = len(data) / 1024 / 1024
print(f"==> Binary Path:   {exe_path}")
print(f"==> Binary Size:   {len(data)} bytes ({size_mb:.2f} MB)")
print(f"==> Binary SHA256: {sha256}")

# 3. Upload to appliance
print("\n==> Sending POST https://app.zemoserver.dev/api/update...")
req = urllib.request.Request(
    "https://app.zemoserver.dev/api/update",
    data=data,
    headers={
        "Authorization": f"Bearer {token}",
        "Content-Type": "application/octet-stream",
        "X-Update-SHA256": sha256,
        "User-Agent": "curl/8.4.0"
    },
    method="POST"
)

try:
    with urllib.request.urlopen(req, timeout=40) as resp:
        print(f"==> Update Response: HTTP {resp.status}")
        print(f"    Body: {resp.read().decode('utf-8')}")
except urllib.error.HTTPError as e:
    print(f"==> Update failed: HTTP {e.code} - {e.read().decode('utf-8')}")
    exit(1)
except Exception as e:
    print(f"==> Update failed with exception: {e}")
    exit(1)

# 4. Wait for appliance restart
print("\n==> Waiting 6 seconds for appliance restart and health checks...")
time.sleep(6)

print("==> Checking appliance status (GET /api/status)...")
status_req = urllib.request.Request(
    "https://app.zemoserver.dev/api/status",
    headers={"Authorization": f"Bearer {token}", "User-Agent": "curl/8.4.0"}
)
try:
    with urllib.request.urlopen(status_req, timeout=10) as resp:
        print(f"    Status: {resp.read().decode('utf-8')}")
except Exception as e:
    print(f"    Status error: {e}")

# 5. Comprehensive verification of public endpoints
base_url = "https://app.zemoserver.dev"

def test_http(method, path, body=None):
    url = f"{base_url}{path}"
    headers = {"User-Agent": "curl/8.4.0"}
    data_bytes = None
    if body:
        headers["Content-Type"] = "application/json"
        data_bytes = json.dumps(body).encode("utf-8")
    
    r = urllib.request.Request(url, data=data_bytes, headers=headers, method=method)
    try:
        with urllib.request.urlopen(r, timeout=10) as resp:
            print(f"[{method}] {path} -> HTTP {resp.status}: {resp.read().decode('utf-8').strip()}")
    except urllib.error.HTTPError as e:
        print(f"[{method}] {path} -> HTTP {e.code}: {e.read().decode('utf-8').strip()}")
    except Exception as e:
        print(f"[{method}] {path} -> Error: {e}")

print("\n==> Verifying Public Endpoints on Remote Appliance:")
test_http("GET", "/")
test_http("GET", "/hello")
test_http("GET", "/auth/status")

test_user = {
    "name": "Zemo User",
    "email": f"user_{int(time.time())}@zemoserver.dev",
    "password": "securepassword123"
}

print(f"\n==> Testing Signup with {test_user['email']}...")
test_http("POST", "/auth/signup", test_user)

print(f"\n==> Testing Duplicate Signup (should return 400)...")
test_http("POST", "/auth/signup", test_user)

print(f"\n==> Testing Login with correct password...")
test_http("POST", "/auth/login", {"email": test_user["email"], "password": "securepassword123"})

print(f"\n==> Testing Login with WRONG password (should return 401)...")
test_http("POST", "/auth/login", {"email": test_user["email"], "password": "wrong_password"})
