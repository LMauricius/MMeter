# MMeter
A simple WIP profiler library for c++. Include the 2 files and you're ready to go.

# Features
- Function measurement
- Scope measurement
- Thread-safety
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
    });
    t1.join();

    std::cout << std::fixed << std::setprecision(6) << MMeter::getGlobalTreePtr()->totalsByDurationStr() << std::endl;
    std::cout << *MMeter::getGlobalTreePtr();
}
```

Output:
```
0.0844672s - calcFloat
0.0845418s - calcInt
0.169012s - test
0.169013s - main function subthread

0.169013s - main function subthread
    0.169012s - test
        0.084467s - calcFloat
        0.084542s - calcInt
```
