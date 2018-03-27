
#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>

#include "parser/tokenizer.h"
#include "parser/parser.h"
#include "passes/optimizationpass.h"
#include "codegen/codegen.h"

using namespace std;
using namespace optimization;
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
    if (fb.open("sampleinput.txt", ios::in))
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

    OptimizationPassManager optimizations(opt.buildType);
    optimizations.performPasses(node);

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
        gen.runJit();
    }

    gen.freeResources();
    
    return 0;
}

void waitForKey()
{
    std::cout << std::endl << "press any key to exit;";
    std::cin.ignore(std::numeric_limits <std::streamsize> ::max(), '\n');
}
