
import os
import sys
import subprocess
import glob

def run_tests():
    # Determine the correct executable name based on platform
    if sys.platform == "win32":
        silver_exe = "silver.exe"
        exe_ext = ".exe"
    else:
        silver_exe = "./silver"
        exe_ext = ""

    # Check if compiler exists
    if not os.path.exists(silver_exe):
        print(f"Error: {silver_exe} not found in current directory")
        sys.exit(1)

    tests = glob.glob("programs/*.sl")
    print(f"Running silver tests...")

    ret_code = 50
    passed = 0
    failed = 0
    for test_path in tests:
        # Get the output executable path (same name as .sl but with .exe)
        test_name = os.path.splitext(test_path)[0]
        test_exe = test_name + exe_ext

        # Step 1: Compile the .sl file to an executable
        compile_result = subprocess.run(
            [silver_exe, test_path],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL
        )

        if compile_result.returncode != 0:
            print(f"    {test_path} failed")
            print(f"        Reason: compilation failed with code {compile_result.returncode}")
            failed += 1
            continue

        # Check if executable was created
        if not os.path.exists(test_exe):
            print(f"    {test_path} failed")
            print(f"        Reason: compiled executable {test_exe} not found")
            failed += 1
            continue

        # Step 2: Run the compiled executable
        run_result = subprocess.run(
            [test_exe],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL
        )

        if run_result.returncode != ret_code:
            print(f"    {test_path} failed")
            print(f"        Reason: expected code {ret_code} but got {run_result.returncode}")
            failed += 1
        else:
            passed += 1

        # Clean up the compiled executable
        try:
            os.remove(test_exe)
            # Also clean up .lib and .exp files that link.exe creates
            lib_file = test_name + ".lib"
            exp_file = test_name + ".exp"
            if os.path.exists(lib_file):
                os.remove(lib_file)
            if os.path.exists(exp_file):
                os.remove(exp_file)
        except:
            pass

    print(f"Results: {passed} passed, {failed} failed")
    return failed

if __name__ == "__main__":
    failed = run_tests()
    sys.exit(0 if failed == 0 else 1)