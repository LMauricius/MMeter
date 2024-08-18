# MMeter
A simple WIP profiler library for c++. Include the 2 files and you're ready to go.

# Features
- Function measurement
- Scope measurement
- Thread-safety
- Duration
- Call counts
- Structured output

# Why not use valgrind?
Originally, I made this tool while waiting for a system update on a rolling-release OS,
because I couldn't install valgrind until everything else was up-to-date.

The tool ended up as a standalone thing with a few specific pros compared to mature industry-standard profilers.
- It is simple and minimal
- You can simply choose the areas of interest for profiling
- Completely portable. It relies only on standard C++17
- It is MUCH faster. since it profiles only select areas of interest, it shouldn't interfere with non-profiled code performance,
  aside from possible cache misses. Chore timing is taken into account, of course.

# Installation?
No installation is required. Just include the `<MMeter repo>/include` directory, and add the `<MMeter repo>/src/MMeter.cpp` file to your build system.

Note: always depend on a specific release. The API might not be stable between releases.
If you want a specific 'unreleased' functionality, dependency of a specific commit is also ok.

# Usage
Usage is simple. Documentation for the functions is in the code comments

Example:
```cpp
#include "MMeter.h"
#include <iomanip>
#include <iostream>
#include <thread>

int calcInt(int ctr)
{
    MMETER_FUNC_PROFILER;

    int val = 0;
    for (int i = 0; i <= ctr; i++)
        val += i;

    return val;
}

int calcFloat(int ctr)
{
    MMETER_FUNC_PROFILER;

    float val = 0;
    for (int i = 0; i <= ctr; i++)
        val += i;

    return val;
}

void test()
{
    MMETER_FUNC_PROFILER;
    calcInt(50000000);
    calcFloat(50000000);
}

int main()
{
    std::thread t1([]() {
        MMETER_SCOPE_PROFILER("main function subthread");
        test();
        test();
    });
    t1.join();

    std::cout << std::fixed << std::setprecision(6) << MMeter::getGlobalTreePtr()->totalsByDurationStr() << std::endl;
    std::cout << *MMeter::getGlobalTreePtr() << std::endl;
    MMeter::getGlobalTreePtr()->outputBranchPercentagesToOStream(std::cout);
}
```

Output:
```
0.186687s /#2 - calcInt
0.213233s /#2 - calcFloat
0.399922s /#2 - test
0.399923s /#7 - <body>
0.399923s /#1 - main function subthread

0.399923s /#1 - main function subthread
    0.000001s /#1 - <body>
    0.399922s /#2 - test
        0.000003s /#2 - <body>
        0.186687s /#2 - calcInt
        0.213233s /#2 - calcFloat

0.399923s /#1 - main function subthread
    0.000183% /^1.000000 - <body>
    99.999817% /^2.000000 - test
        0.000673% /^1.000000 - <body>
        46.680728% /^1.000000 - calcInt
        53.318599% /^1.000000 - calcFloat
```
