
#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>

#include "parser.h"
#include "tokenizer.h"
#include "codegen.h"

using namespace std;

struct ProgramOpts
{
public:
    inline ProgramOpts() :
        runJit(true),
        genByteCode(false)
    {

    }

    bool runJit;
    bool genByteCode;
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

    CodeGen gen(node);
    
    if (opt.genByteCode)
    {
        string out = "sampleoutput.bc";
        gen.Generate();
    }

    if (opt.runJit)
    {
        cout << "running as JIT" << endl;
        gen.RunJit();
    }
    else
    {
        // TODO: do the non-jit case
    }

    gen.FreeResources();
    
    waitForKey();
    return 0;
}
