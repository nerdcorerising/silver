
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
        runJit(true),
        genByteCode(false)
    {
        buildType = BuildType::Debug;
    }

    bool runJit;
    bool genByteCode;
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
        }
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
            node->prettyPrint(cout);
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

        string out = "";
        if (opt.genByteCode)
        {
            out = "sampleoutput.bc";
        }

        CodeGen gen(node, out);
        gen.generate();

        cout << "running as JIT" << endl;
        result = gen.runJit();

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
