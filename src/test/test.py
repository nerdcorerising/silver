
import os
import subprocess
import glob

def run_tests():
    tests = glob.glob("programs/*.sl")
    print(tests)

    ret_code = 50
    for test_path in tests:
        print(f"    Runng test {test_path}")
        response = subprocess.run(["silver", test_path], shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        if response.returncode != ret_code:
            print(f"    Test failed, expected code {ret_code} but got {response.returncode}")
        else:
            print("    Test passed!")

if __name__ == "__main__":
    print("")
    print("")
    print("")
    print("Running silver tests")
    run_tests()