
#pragma once

#include <assert.h>
#include <iostream>

#ifdef _DEBUG
#define ASSERT(X) assert(X)
#else
#define ASSERT(X)
#endif // _DEBUG

#define UNREFERENCED(X) (void)(X)

#define CONSISTENCY_CHECK(COND, X) do { if (!(COND)) { cout << X; throw "Error handling not implemented"; } } while (false)

