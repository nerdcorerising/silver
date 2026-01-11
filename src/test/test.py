
import os
import sys
import subprocess
import glob

def run_single_test(silver_exe, test_path, exe_ext, optimize, ret_code):
    """Run a single test with or without optimization. Returns (passed, error_msg)"""
    test_name = os.path.splitext(test_path)[0]
    test_exe = test_name + exe_ext
    mode = "optimized" if optimize else "unoptimized"

    # Build compile command
    compile_cmd = [silver_exe, test_path]
    if optimize:
        compile_cmd.append("-optimize")

    # Step 1: Compile the .sl file to an executable
    compile_result = subprocess.run(
        compile_cmd,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL
    )

    if compile_result.returncode != 0:
        return (False, f"compilation failed ({mode}) with code {compile_result.returncode}")

    # Check if executable was created
    if not os.path.exists(test_exe):
        return (False, f"compiled executable {test_exe} not found ({mode})")

    # Step 2: Run the compiled executable
    run_result = subprocess.run(
        [test_exe],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL
    )

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

    if run_result.returncode != ret_code:
        return (False, f"expected code {ret_code} but got {run_result.returncode} ({mode})")

    return (True, None)

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
    print(f"Running silver tests (both optimized and unoptimized)...")

    ret_code = 50
    passed = 0
    failed = 0

    for test_path in tests:
        test_passed = True

        # Run unoptimized
        success, error = run_single_test(silver_exe, test_path, exe_ext, False, ret_code)
        if not success:
            print(f"    {test_path} failed")
            print(f"        Reason: {error}")
            test_passed = False

        # Run optimized
        success, error = run_single_test(silver_exe, test_path, exe_ext, True, ret_code)
        if not success:
            print(f"    {test_path} failed")
            print(f"        Reason: {error}")
            test_passed = False

        if test_passed:
            passed += 1
        else:
            failed += 1

    print(f"Results: {passed} passed, {failed} failed")
    return failed

if __name__ == "__main__":
    failed = run_tests()
    sys.exit(0 if failed == 0 else 1)
