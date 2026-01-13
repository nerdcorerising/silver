
import os
import sys
import subprocess
import glob
import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed

def run_single_test(silver_exe, test_path, exe_ext, optimize, ret_code):
    """Run a single test with or without optimization. Returns (passed, error_msg, output)"""
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
        capture_output=True,
        text=True
    )
    compile_output = compile_result.stdout + compile_result.stderr

    if compile_result.returncode != 0:
        return (False, f"compilation failed ({mode}) with code {compile_result.returncode}", compile_output)

    # Check if executable was created
    if not os.path.exists(test_exe):
        return (False, f"compiled executable {test_exe} not found ({mode})", compile_output)

    # Step 2: Run the compiled executable
    run_result = subprocess.run(
        [test_exe],
        capture_output=True,
        text=True
    )
    run_output = run_result.stdout + run_result.stderr

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
        return (False, f"expected code {ret_code} but got {run_result.returncode} ({mode})", compile_output + run_output)

    return (True, None, None)

def get_expected_error(test_path):
    """Extract expected error message from first line comment in test file."""
    with open(test_path, 'r') as f:
        first_line = f.readline().strip()
        if first_line.startswith("# expect-error:"):
            return first_line[len("# expect-error:"):].strip()
    return None

def run_error_test(silver_exe, test_path, optimize):
    """Run a test that expects compilation to fail. Returns (passed, error_msg, output)"""
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
    compile_output = compile_result.stdout + compile_result.stderr

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
        return (False, f"compilation should have failed but succeeded ({mode})", compile_output)

    # Check for expected error message if specified
    expected_error = get_expected_error(test_path)
    if expected_error:
        if expected_error not in compile_output:
            return (False, f"expected error '{expected_error}' not found in output ({mode})", compile_output)

    return (True, None, None)

def run_complete_test(args):
    """Run a complete test (both optimized and unoptimized). Returns (test_path, passed, failures)"""
    silver_exe, test_path, exe_ext, is_error_test, ret_code = args
    failures = []

    if is_error_test:
        # Run unoptimized
        success, error, output = run_error_test(silver_exe, test_path, False)
        if not success:
            failures.append((error, output))

        # Run optimized
        success, error, output = run_error_test(silver_exe, test_path, True)
        if not success:
            failures.append((error, output))
    else:
        # Run unoptimized
        success, error, output = run_single_test(silver_exe, test_path, exe_ext, False, ret_code)
        if not success:
            failures.append((error, output))

        # Run optimized
        success, error, output = run_single_test(silver_exe, test_path, exe_ext, True, ret_code)
        if not success:
            failures.append((error, output))

    return (test_path, len(failures) == 0, failures)

def run_tests(parallel=False, num_threads=None):
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

    ret_code = 50

    # Build list of all test tasks
    test_tasks = []
    for test_path in normal_tests:
        test_tasks.append((silver_exe, test_path, exe_ext, False, ret_code))
    for test_path in error_tests:
        test_tasks.append((silver_exe, test_path, exe_ext, True, ret_code))

    if parallel:
        threads = num_threads if num_threads else os.cpu_count()
        print(f"Running silver tests in parallel ({threads} threads)...")
    else:
        print(f"Running silver tests (both optimized and unoptimized)...")

    passed = 0
    failed = 0
    results = []

    if parallel:
        # Run tests in parallel
        with ThreadPoolExecutor(max_workers=num_threads) as executor:
            futures = {executor.submit(run_complete_test, task): task for task in test_tasks}
            for future in as_completed(futures):
                test_path, test_passed, failures = future.result()
                results.append((test_path, test_passed, failures))
    else:
        # Run tests sequentially
        for task in test_tasks:
            test_path, test_passed, failures = run_complete_test(task)
            results.append((test_path, test_passed, failures))

    # Sort results by test path for consistent output
    results.sort(key=lambda x: x[0])

    # Print results
    for test_path, test_passed, failures in results:
        if test_passed:
            passed += 1
        else:
            failed += 1
            for error, output in failures:
                print(f"    {test_path} failed")
                print(f"        Reason: {error}")
                if output:
                    print(f"        Output: {output.strip()}")

    print(f"Results: {passed} passed, {failed} failed")
    return failed

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run Silver compiler tests")
    parser.add_argument("-j", "--parallel", nargs="?", const=0, type=int, metavar="N",
                        help="Run tests in parallel. Optionally specify number of threads (default: CPU count)")
    args = parser.parse_args()

    parallel = args.parallel is not None
    num_threads = args.parallel if args.parallel and args.parallel > 0 else None

    failed = run_tests(parallel=parallel, num_threads=num_threads)
    sys.exit(0 if failed == 0 else 1)
