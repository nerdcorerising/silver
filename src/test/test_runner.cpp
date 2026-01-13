
#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>
#include <cstdlib>
#include <atomic>
#include <cstring>

namespace fs = std::filesystem;

const int EXPECTED_RETURN_CODE = 50;

struct TestFailure
{
    std::string reason;
    std::string output;
};

struct TestResult
{
    std::string path;
    bool passed;
    std::vector<TestFailure> failures;
};

struct TestTask
{
    std::string path;
    bool isErrorTest;
    bool isPdbTest;
};

// Run a process and capture its output, returns exit code
int runProcess(const std::string &cmd, std::string &output)
{
    output.clear();
    FILE *pipe = _popen(cmd.c_str(), "r");
    if (!pipe)
    {
        output = "Failed to run command";
        return -1;
    }

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        output += buffer;
    }

    int exitCode = _pclose(pipe);
    // On Windows, _pclose returns the exit code directly
    return exitCode;
}

// Extract expected error from "# expect-error:" comment on first line
std::string getExpectedError(const std::string &testPath)
{
    std::ifstream file(testPath);
    if (!file.is_open())
        return "";

    std::string firstLine;
    std::getline(file, firstLine);

    const std::string prefix = "# expect-error:";
    if (firstLine.substr(0, prefix.size()) == prefix)
    {
        std::string error = firstLine.substr(prefix.size());
        // Trim leading whitespace
        size_t start = error.find_first_not_of(" \t");
        if (start != std::string::npos)
        {
            return error.substr(start);
        }
    }
    return "";
}

// Extract expected symbols from "# expect-symbols:" comment on first line
// Returns comma-separated list of symbol names
std::string getExpectedSymbols(const std::string &testPath)
{
    std::ifstream file(testPath);
    if (!file.is_open())
        return "";

    std::string firstLine;
    std::getline(file, firstLine);

    const std::string prefix = "# expect-symbols:";
    if (firstLine.substr(0, prefix.size()) == prefix)
    {
        std::string symbols = firstLine.substr(prefix.size());
        // Trim leading/trailing whitespace
        size_t start = symbols.find_first_not_of(" \t");
        size_t end = symbols.find_last_not_of(" \t");
        if (start != std::string::npos && end != std::string::npos)
        {
            return symbols.substr(start, end - start + 1);
        }
    }
    return "";
}

// Cleanup generated files (.exe, .lib, .exp, .pdb, .obj)
void cleanup(const std::string &basePath)
{
    std::vector<std::string> extensions = {".exe", ".lib", ".exp", ".pdb", ".obj"};
    for (const auto &ext : extensions)
    {
        std::string filePath = basePath + ext;
        if (fs::exists(filePath))
        {
            try
            {
                fs::remove(filePath);
            }
            catch (...)
            {
            }
        }
    }
}

// Run a single normal test (compile + execute) in one mode
bool runSingleTest(const std::string &testPath, bool optimize, std::string &errorMsg, std::string &output)
{
    std::string mode = optimize ? "optimized" : "unoptimized";
    std::string baseName = testPath.substr(0, testPath.size() - 3); // Remove .sl
    std::string exePath = baseName + ".exe";

    // Build compile command
    std::string compileCmd = "silver.exe \"" + testPath + "\"";
    if (optimize)
    {
        compileCmd += " -optimize";
    }
    compileCmd += " 2>&1";

    // Compile
    std::string compileOutput;
    int compileResult = runProcess(compileCmd, compileOutput);
    output = compileOutput;

    if (compileResult != 0)
    {
        errorMsg = "compilation failed (" + mode + ") with code " + std::to_string(compileResult);
        return false;
    }

    // Check executable exists
    if (!fs::exists(exePath))
    {
        errorMsg = "compiled executable " + exePath + " not found (" + mode + ")";
        return false;
    }

    // Run executable
    std::string runCmd = "\"" + exePath + "\"";
    std::string runOutput;
    int runResult = runProcess(runCmd, runOutput);
    output += runOutput;

    // Cleanup
    cleanup(baseName);

    if (runResult != EXPECTED_RETURN_CODE)
    {
        errorMsg = "expected code " + std::to_string(EXPECTED_RETURN_CODE) + " but got " + std::to_string(runResult) + " (" + mode + ")";
        return false;
    }

    return true;
}

// Run a single error test (compile expecting failure) in one mode
bool runSingleErrorTest(const std::string &testPath, bool optimize, std::string &errorMsg, std::string &output)
{
    std::string mode = optimize ? "optimized" : "unoptimized";
    std::string baseName = testPath.substr(0, testPath.size() - 3);

    // Build compile command
    std::string compileCmd = "silver.exe \"" + testPath + "\"";
    if (optimize)
    {
        compileCmd += " -optimize";
    }
    compileCmd += " 2>&1";

    // Compile - we expect this to fail
    std::string compileOutput;
    int compileResult = runProcess(compileCmd, compileOutput);
    output = compileOutput;

    if (compileResult == 0)
    {
        // Compilation succeeded when it should have failed
        cleanup(baseName);
        errorMsg = "compilation should have failed but succeeded (" + mode + ")";
        return false;
    }

    // Check for expected error message if specified
    std::string expectedError = getExpectedError(testPath);
    if (!expectedError.empty())
    {
        if (compileOutput.find(expectedError) == std::string::npos)
        {
            errorMsg = "expected error '" + expectedError + "' not found in output (" + mode + ")";
            return false;
        }
    }

    return true;
}

// Run a PDB test (compile with -g, verify symbols in PDB)
bool runPdbTest(const std::string &testPath, std::string &errorMsg, std::string &output)
{
    std::string baseName = testPath.substr(0, testPath.size() - 3); // Remove .sl
    std::string pdbPath = baseName + ".pdb";

    // Compile with debug symbols
    std::string compileCmd = "silver.exe \"" + testPath + "\" -g 2>&1";
    std::string compileOutput;
    int compileResult = runProcess(compileCmd, compileOutput);
    output = compileOutput;

    if (compileResult != 0)
    {
        cleanup(baseName);
        errorMsg = "compilation with -g failed with code " + std::to_string(compileResult);
        return false;
    }

    // Check PDB was created
    if (!fs::exists(pdbPath))
    {
        cleanup(baseName);
        errorMsg = "PDB file not created: " + pdbPath;
        return false;
    }

    // Get expected symbols
    std::string expectedSymbols = getExpectedSymbols(testPath);
    if (expectedSymbols.empty())
    {
        cleanup(baseName);
        errorMsg = "no expected symbols specified (use # expect-symbols: comment)";
        return false;
    }

    // Build pdb_test command with expected symbols
    // Replace commas with spaces for command line args
    std::string symbolArgs = expectedSymbols;
    for (char &c : symbolArgs)
    {
        if (c == ',')
            c = ' ';
    }

    std::string pdbTestCmd = "pdb_test.exe \"" + pdbPath + "\" " + symbolArgs + " 2>&1";
    std::string pdbTestOutput;
    int pdbTestResult = runProcess(pdbTestCmd, pdbTestOutput);
    output += pdbTestOutput;

    // Cleanup
    cleanup(baseName);

    if (pdbTestResult != 0)
    {
        errorMsg = "PDB verification failed: " + pdbTestOutput;
        return false;
    }

    return true;
}

// Run a complete test (both optimized and unoptimized modes)
TestResult runCompleteTest(const TestTask &task)
{
    TestResult result;
    result.path = task.path;
    result.passed = true;

    std::string errorMsg, output;
    bool success;

    // PDB tests only run once (with -g flag, no optimization modes)
    if (task.isPdbTest)
    {
        success = runPdbTest(task.path, errorMsg, output);
        if (!success)
        {
            result.passed = false;
            result.failures.push_back({errorMsg, output});
        }
        return result;
    }

    // Run unoptimized
    if (task.isErrorTest)
    {
        success = runSingleErrorTest(task.path, false, errorMsg, output);
    }
    else
    {
        success = runSingleTest(task.path, false, errorMsg, output);
    }
    if (!success)
    {
        result.passed = false;
        result.failures.push_back({errorMsg, output});
    }

    // Run optimized
    if (task.isErrorTest)
    {
        success = runSingleErrorTest(task.path, true, errorMsg, output);
    }
    else
    {
        success = runSingleTest(task.path, true, errorMsg, output);
    }
    if (!success)
    {
        result.passed = false;
        result.failures.push_back({errorMsg, output});
    }

    return result;
}

// Worker thread function for parallel execution
void workerThread(std::vector<TestTask> &tasks,
                  std::atomic<size_t> &nextTask,
                  std::vector<TestResult> &results,
                  std::mutex &resultsMutex)
{
    while (true)
    {
        size_t index = nextTask.fetch_add(1);
        if (index >= tasks.size())
            break;

        TestResult result = runCompleteTest(tasks[index]);

        std::lock_guard<std::mutex> lock(resultsMutex);
        results.push_back(result);
    }
}

void printUsage(const char *programName)
{
    std::cout << "Usage: " << programName << " [-j [N]]\n";
    std::cout << "  -j     Run tests in parallel (default: CPU count threads)\n";
    std::cout << "  -j N   Run tests in parallel with N threads\n";
}

int main(int argc, char *argv[])
{
    bool parallel = false;
    int numThreads = 0;

    // Parse arguments
    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "-j") == 0 || strcmp(argv[i], "--parallel") == 0)
        {
            parallel = true;
            // Check if next argument is a number
            if (i + 1 < argc)
            {
                char *end;
                long n = strtol(argv[i + 1], &end, 10);
                if (*end == '\0' && n > 0)
                {
                    numThreads = static_cast<int>(n);
                    ++i;
                }
            }
        }
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
        {
            printUsage(argv[0]);
            return 0;
        }
    }

    // Check if silver.exe exists
    if (!fs::exists("silver.exe"))
    {
        std::cerr << "Error: silver.exe not found in current directory\n";
        return 1;
    }

    // Discover test files
    std::vector<TestTask> tasks;
    for (const auto &entry : fs::directory_iterator("programs"))
    {
        if (entry.path().extension() == ".sl")
        {
            TestTask task;
            task.path = entry.path().string();
            task.isErrorTest = task.path.find("_error.sl") != std::string::npos;
            task.isPdbTest = task.path.find("_pdb.sl") != std::string::npos;
            tasks.push_back(task);
        }
    }

    // Sort tasks by path for consistent ordering
    std::sort(tasks.begin(), tasks.end(), [](const TestTask &a, const TestTask &b)
              { return a.path < b.path; });

    if (tasks.empty())
    {
        std::cout << "No test files found in programs/\n";
        return 0;
    }

    // Print header
    if (parallel)
    {
        if (numThreads == 0)
        {
            numThreads = std::thread::hardware_concurrency();
            if (numThreads == 0)
                numThreads = 4;
        }
        std::cout << "Running silver tests in parallel (" << numThreads << " threads)...\n";
    }
    else
    {
        std::cout << "Running silver tests (both optimized and unoptimized)...\n";
    }

    std::vector<TestResult> results;

    if (parallel)
    {
        std::atomic<size_t> nextTask(0);
        std::mutex resultsMutex;
        std::vector<std::thread> threads;

        for (int i = 0; i < numThreads; ++i)
        {
            threads.emplace_back(workerThread, std::ref(tasks), std::ref(nextTask),
                                 std::ref(results), std::ref(resultsMutex));
        }

        for (auto &t : threads)
        {
            t.join();
        }
    }
    else
    {
        for (const auto &task : tasks)
        {
            results.push_back(runCompleteTest(task));
        }
    }

    // Sort results by path for consistent output
    std::sort(results.begin(), results.end(), [](const TestResult &a, const TestResult &b)
              { return a.path < b.path; });

    // Print results
    int passed = 0;
    int failed = 0;
    for (const auto &result : results)
    {
        if (result.passed)
        {
            ++passed;
        }
        else
        {
            ++failed;
            for (const auto &failure : result.failures)
            {
                std::cout << "    " << result.path << " failed\n";
                std::cout << "        Reason: " << failure.reason << "\n";
                if (!failure.output.empty())
                {
                    // Trim trailing whitespace from output
                    std::string output = failure.output;
                    size_t end = output.find_last_not_of(" \t\n\r");
                    if (end != std::string::npos)
                    {
                        output = output.substr(0, end + 1);
                    }
                    std::cout << "        Output: " << output << "\n";
                }
            }
        }
    }

    std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
