
import os
import subprocess

def run_tests():
    tests = [
        ("basic_function_test.sl", 13)
        ]

    for test_file, ret_code in tests:
        test_path = os.path.join("programs", test_file)
        print(f"Running {test_path} and expecting return code {ret_code}")
        response = subprocess.run(["silver", test_path], shell=True)
        if response.returncode != ret_code:
            print(f"Test failed, expected code {ret_code} but got {response.returncode}")
        else:
            print("Test passed!")

if __name__ == "__main__":
    run_tests()