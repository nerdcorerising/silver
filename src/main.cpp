
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
        runJit(false)
    {

    }

    bool runJit;
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

    string out = "sampleoutput.bc";
    CodeGen gen(node, out);
    gen.Generate();

    if (opt.runJit || true /* //TODO: don't want to mess with configs for now, remove */)
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
