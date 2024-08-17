# MMeter
A simple WIP profiler library for c++. Include the 2 files and you're ready to go.

# Features
- Function measurement
- Scope measurement
- Thread-safety
- Structured output

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