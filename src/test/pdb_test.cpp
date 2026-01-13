
#include <iostream>
#include <vector>
#include <set>
#include <string>

#pragma warning(push)
#pragma warning(disable:4996)
#pragma warning(disable:4146)
#pragma warning(disable:4244)
#pragma warning(disable:4267)
#pragma warning(disable:4624)
#pragma warning(disable:4100)
#pragma warning(disable:4245)
#pragma warning(disable:4458)
#pragma warning(disable:4324)
#include "llvm/DebugInfo/PDB/PDB.h"
#include "llvm/DebugInfo/PDB/IPDBSession.h"
#include "llvm/DebugInfo/PDB/PDBSymbolExe.h"
#include "llvm/DebugInfo/PDB/PDBSymbolCompiland.h"
#include "llvm/DebugInfo/PDB/PDBSymbolFunc.h"
#include "llvm/DebugInfo/PDB/PDBSymbolPublicSymbol.h"
#include "llvm/DebugInfo/PDB/PDBSymbolTypeFunctionSig.h"
#include "llvm/DebugInfo/PDB/PDBSymbolData.h"
#include "llvm/DebugInfo/PDB/ConcreteSymbolEnumerator.h"
#include "llvm/Support/Error.h"
#pragma warning(pop)

using namespace llvm;
using namespace llvm::pdb;

// Usage: pdb_test <pdb_file> <expected_func1> [expected_func2] ...
// Returns 0 if all expected functions found, 1 otherwise

void printUsage(const char* programName)
{
    std::cerr << "Usage: " << programName << " <pdb_file> [-list] [func1] [func2...]\n";
    std::cerr << "Verifies that the specified functions exist in the PDB file.\n";
    std::cerr << "  -list  List all symbols found in the PDB\n";
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printUsage(argv[0]);
        return 1;
    }

    std::string pdbPath = argv[1];
    bool listMode = false;
    std::set<std::string> expectedFuncs;
    for (int i = 2; i < argc; ++i)
    {
        if (std::string(argv[i]) == "-list")
        {
            listMode = true;
        }
        else
        {
            expectedFuncs.insert(argv[i]);
        }
    }

    // Load PDB - try DIA reader first (better Windows support), fall back to native
    std::unique_ptr<IPDBSession> session;
    Error err = loadDataForPDB(PDB_ReaderType::DIA, pdbPath, session);
    if (err)
    {
        // DIA not available, try native reader
        consumeError(std::move(err));
        err = loadDataForPDB(PDB_ReaderType::Native, pdbPath, session);
    }

    if (err)
    {
        std::cerr << "Failed to load PDB: " << toString(std::move(err)) << "\n";
        return 1;
    }

    if (!session)
    {
        std::cerr << "Failed to create PDB session\n";
        return 1;
    }

    // Get the global scope
    auto globalScope = session->getGlobalScope();
    if (!globalScope)
    {
        std::cerr << "Failed to get global scope\n";
        return 1;
    }

    std::set<std::string> foundFuncs;

    // Look for functions in public symbols
    auto publicSymbols = globalScope->findAllChildren<PDBSymbolPublicSymbol>();
    if (publicSymbols)
    {
        while (auto symbol = publicSymbols->getNext())
        {
            std::string name = symbol->getName();
            foundFuncs.insert(name);
        }
    }

    // Also look for function symbols directly
    auto funcSymbols = globalScope->findAllChildren<PDBSymbolFunc>();
    if (funcSymbols)
    {
        while (auto func = funcSymbols->getNext())
        {
            std::string name = func->getName();
            foundFuncs.insert(name);
        }
    }

    // Look in compilands for functions
    auto compilands = globalScope->findAllChildren<PDBSymbolCompiland>();
    if (compilands)
    {
        while (auto compiland = compilands->getNext())
        {
            if (listMode)
            {
                std::cout << "Compiland: " << compiland->getName() << "\n";
            }

            // Look for functions in each compiland
            auto compilandFuncs = compiland->findAllChildren<PDBSymbolFunc>();
            if (compilandFuncs)
            {
                while (auto func = compilandFuncs->getNext())
                {
                    std::string name = func->getName();
                    foundFuncs.insert(name);
                    if (listMode)
                    {
                        std::cout << "  Function in compiland: " << name << "\n";
                    }
                }
            }
        }
    }

    // List mode: print all found symbols
    if (listMode)
    {
        std::cout << "Total symbols found in PDB (" << foundFuncs.size() << "):\n";
        for (const auto& name : foundFuncs)
        {
            std::cout << "  " << name << "\n";
        }
        if (expectedFuncs.empty())
        {
            return 0;
        }
    }

    // Check all expected functions were found
    bool allFound = true;
    for (const auto& expected : expectedFuncs)
    {
        bool found = false;
        for (const auto& funcName : foundFuncs)
        {
            // Check for exact match or if the function name contains the expected name
            // (handles decorated names like _main or main@0)
            if (funcName == expected ||
                funcName.find(expected) != std::string::npos)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            std::cerr << "Missing function: " << expected << "\n";
            allFound = false;
        }
    }

    if (allFound)
    {
        std::cout << "All expected symbols found in PDB\n";
    }

    return allFound ? 0 : 1;
}
