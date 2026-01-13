
#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>

#include "parser/tokenizer.h"
#include "parser/parser.h"
#include "passes/analysispass.h"
#include "codegen/codegen.h"
#include "logger.h"

using namespace std;
using namespace analysis;
using namespace codegen;

void waitForKey();

struct ProgramOpts
{
public:
    inline ProgramOpts() :
        genByteCode(false),
        verbose(false),
        optimize(false),
        debugSymbols(false)
    {
        buildType = BuildType::Debug;
    }

    bool genByteCode;
    bool verbose;
    bool optimize;
    bool debugSymbols;
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
            transform(realArg.begin(), realArg.end(), realArg.begin(),
                [](unsigned char c) { return static_cast<char>(::tolower(c)); });

            if (realArg == "bytecode")
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
            else if (realArg.substr(0, 2) == "o:")
            {
                opt.outputName = realArg.substr(2);
            }
            else if (realArg == "verbose" || realArg == "v")
            {
                opt.verbose = true;
            }
            else if (realArg == "optimize" || realArg == "o")
            {
                opt.optimize = true;
            }
            else if (realArg == "g")
            {
                opt.debugSymbols = true;
            }
        }
    }

    return opt;
}

int main(int argc, char **argv)
{
    Opts opt = getOpts(argc, argv);

    // Configure logger based on verbose flag
    logging::Logger::setEnabled(opt.verbose);

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

        if (opt.verbose)
        {
            node->prettyPrint(cout);
        }

        string out = "";
        if (opt.genByteCode)
        {
            out = "sampleoutput.bc";
        }

        CodeGen gen(node, file, out);
        gen.setOptimize(opt.optimize);
        gen.setDebugSymbols(opt.debugSymbols);
        gen.generate();

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
