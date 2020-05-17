
#include <utility>
#include <vector>
#include <string>
#include <cstdio>

using std::vector;
using std::pair;
using std::string;

int main()
{
    vector<pair<string, int>> tests =
    {
        { "basic_function_test.sl", 13 }
    };

    for (auto &&test : tests)
    {
        printf("would run test programs\\%s\n", test.first.c_str());
    }

    return 0;
}