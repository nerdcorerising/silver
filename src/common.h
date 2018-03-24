
#pragma once

#include <assert.h>
#include <iostream>

#ifdef _DEBUG
#define ASSERT(X) assert(X)
#else
#define ASSERT(X)
#endif // _DEBUG

#define UNREFERENCED(X) (void)(X)

#define INTERNAL_ERROR(X) cout << X; throw "Error handling not implemented"

inline void waitForKey()
{
    std::cout << std::endl << "press any key to exit;";
    std::cin.ignore(std::numeric_limits <std::streamsize> ::max(), '\n');
}
