
#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>

#include "parser/tokenizer.h"
#include "parser/parser.h"
#include "passes/analysispass.h"
#include "codegen/codegen.h"

using namespace std;
using namespace analysis;
using namespace codegen;

void waitForKey();

struct ProgramOpts
{
public:
    inline ProgramOpts() :
        runJit(false),
        genByteCode(false),
        compile(false)
    {
        buildType = BuildType::Debug;
    }

    bool runJit;
    bool genByteCode;
    bool compile;
    string outputName;
    BuildType buildType;
};

typedef ProgramOpts Opts;

Opts getOpts(int argc, char **argv)
{
    Opts opt;

    if (argc <= 1)
    {
        return opt;
    }

    for (int i = 1; i < argc; ++i)
    {
        string arg(argv[i]);
        if (arg[0] == '-' || arg[0] == '/')
        {
            string realArg = arg.substr(1, string::npos);
            transform(realArg.begin(), realArg.end(), realArg.begin(), ::tolower);

            if (realArg == "jit")
            {
                opt.runJit = true;
            }
            else if (realArg == "bytecode")
            {
                opt.genByteCode = true;
            }
            else if (realArg == "debug")
            {
                opt.buildType = BuildType::Debug;
            }
            else if (realArg == "release")
            {
                opt.buildType = BuildType::Release;
            }
            else if (realArg == "compile" || realArg == "c")
            {
                opt.compile = true;
            }
            else if (realArg.substr(0, 2) == "o:")
            {
                opt.outputName = realArg.substr(2);
            }
        }
    }

    // Default to compile if neither jit nor compile specified
    if (!opt.runJit && !opt.compile)
    {
        opt.compile = true;
    }

    return opt;
}

int main(int argc, char **argv)
{
    Opts opt = getOpts(argc, argv);

    filebuf fb;
    shared_ptr<ast::Assembly> node;

    if (argc <= 1)
    {
        cout << "No input file provided! " << endl;
        return -1;
    }

    const char *file = argv[1];
    cout << "Compiling file " << file << endl;
    int result = -1;
    try
    {
        if (fb.open(file, ios::in))
        {
            istream input(&fb);

            tok::Tokenizer tok;
            parse::Parser parser("sample", tok, input);

            node = parser.parse();
            fb.close();
        }
        else
        {
            cerr << "Failed to open input file." << endl;
            waitForKey();
            return -1;
        }

        AnalysisPassManager analysis(opt.buildType);
        analysis.performPasses(node);

        node->prettyPrint(cout);

        string out = "";
        if (opt.genByteCode)
        {
            out = "sampleoutput.bc";
        }

        CodeGen gen(node, out);
        gen.generate();

        if (opt.runJit)
        {
            cout << "running as JIT" << endl;
            result = gen.runJit();
        }
        else if (opt.compile)
        {
            // Determine output name
            string outputName = opt.outputName;
            if (outputName.empty())
            {
                // Use input filename without extension
                outputName = file;
                size_t dotPos = outputName.rfind('.');
                if (dotPos != string::npos)
                {
                    outputName = outputName.substr(0, dotPos);
                }
            }

            cout << "Compiling to executable..." << endl;
            if (gen.compileToExecutable(outputName))
            {
                result = 0;
            }
            else
            {
                result = -1;
            }
        }

        gen.freeResources();
    }
    catch (string message)
    {
        // TODO: better error handling
        printf("Caught message=\"%s\" during compilation\n", message.c_str());
    }
    return result;
}

void waitForKey()
{
    cout << endl << "press any key to exit;";
    cin.ignore(numeric_limits <streamsize> ::max(), '\n');
}
