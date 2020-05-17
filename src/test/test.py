
import os
import subprocess

def run_tests():
    tests = [
        ("basic_function_test.sl", 13)
        ]

    for test_file, ret_code in tests:
        test_path = os.path.join("programs", test_file)
        print(f"    Test {test_path} with return code {ret_code}")
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