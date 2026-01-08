
import os
import sys
import subprocess
import glob

def run_tests():
    # Determine the correct executable name based on platform
    if sys.platform == "win32":
        silver_exe = "silver.exe"
    else:
        silver_exe = "./silver"

    # Check if executable exists
    exe_path = "silver.exe" if sys.platform == "win32" else "silver"
    if not os.path.exists(exe_path):
        print(f"Error: {exe_path} not found in current directory")
        sys.exit(1)

    tests = glob.glob("programs/*.sl")
    print(f"Running silver tests...")

    ret_code = 50
    passed = 0
    failed = 0
    for test_path in tests:
        # Use list form to avoid shell issues across platforms
        response = subprocess.run([silver_exe, test_path], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        if response.returncode != ret_code:
            print(f"    {test_path} failed")
            print(f"        Reason: expected code {ret_code} but got {response.returncode}")
            failed += 1
        else:
            passed += 1

    print(f"Results: {passed} passed, {failed} failed")

if __name__ == "__main__":
    run_tests()