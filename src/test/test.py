
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

def get_expected_error(test_path):
    """Extract expected error message from first line comment in test file."""
    with open(test_path, 'r') as f:
        first_line = f.readline().strip()
        if first_line.startswith("# expect-error:"):
            return first_line[len("# expect-error:"):].strip()
    return None

def run_error_test(silver_exe, test_path, optimize):
    """Run a test that expects compilation to fail. Returns (passed, error_msg)"""
    mode = "optimized" if optimize else "unoptimized"

    # Build compile command
    compile_cmd = [silver_exe, test_path]
    if optimize:
        compile_cmd.append("-optimize")

    # Compile the .sl file - we expect this to fail
    compile_result = subprocess.run(
        compile_cmd,
        capture_output=True,
        text=True
    )

    if compile_result.returncode == 0:
        # Compilation succeeded when it should have failed
        # Clean up any generated files
        test_name = os.path.splitext(test_path)[0]
        for ext in [".exe", ".lib", ".exp", ""]:
            try:
                f = test_name + ext
                if os.path.exists(f) and os.path.isfile(f):
                    os.remove(f)
            except:
                pass
        return (False, f"compilation should have failed but succeeded ({mode})")

    # Check for expected error message if specified
    expected_error = get_expected_error(test_path)
    if expected_error:
        output = compile_result.stdout + compile_result.stderr
        if expected_error not in output:
            return (False, f"expected error '{expected_error}' not found in output ({mode})")

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

    # Get all tests, separating normal tests from error tests
    all_tests = glob.glob("programs/*.sl")
    normal_tests = [t for t in all_tests if not t.endswith("_error.sl")]
    error_tests = [t for t in all_tests if t.endswith("_error.sl")]

    print(f"Running silver tests (both optimized and unoptimized)...")

    ret_code = 50
    passed = 0
    failed = 0

    # Run normal tests (expect success with return code 50)
    for test_path in normal_tests:
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

    # Run error tests (expect compilation to fail)
    for test_path in error_tests:
        test_passed = True

        # Run unoptimized
        success, error = run_error_test(silver_exe, test_path, False)
        if not success:
            print(f"    {test_path} failed")
            print(f"        Reason: {error}")
            test_passed = False

        # Run optimized
        success, error = run_error_test(silver_exe, test_path, True)
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
