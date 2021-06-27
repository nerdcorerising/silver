
import os
import subprocess
import glob

def run_tests():
    tests = glob.glob("programs/*.sl")
    print(f"Running silver tests...")

    ret_code = 50
    for test_path in tests:
        test = f'./silver {test_path}'
        # print(f'Running test {test}')
        response = subprocess.run([test], shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        # response = os.system(test)
        if response.returncode != ret_code:
            print(f"    {test_path} failed")
            print(f"        Reason: expected code {ret_code} but got {response.returncode}")
        # else:
        #     print("    Test passed!")

if __name__ == "__main__":
    print("")
    print("")
    print("")
    run_tests()