#include "MMeter.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

int calcInt(int ctr)
{
    MMETER_FUNC_PROFILER;

    int val = 0;
    for (int i = 0; i <= ctr; i++)
    {
        val += i;
    }
    return val;
}

int calcFloat(int ctr)
{
    MMETER_FUNC_PROFILER;

    float val = 0;
    for (int i = 0; i <= ctr; i++)
    {
        val += i;
    }
    return val;
}

std::string calcString(int ctr)
{
    MMETER_FUNC_PROFILER;

    std::stringstream ss;
    for (int i = 0; i <= ctr; i++)
    {
        ss << i << ' ';
    }
    return ss.str();
}

double calcNumbers(int ctr)
{
    MMETER_FUNC_PROFILER;

    return (calcInt(ctr) + calcFloat(ctr));
}

double calcAll(int ctr)
{
    MMETER_FUNC_PROFILER;

    calcString(ctr);
    return (calcInt(ctr) + calcFloat(ctr));
}

void test()
{
    MMETER_FUNC_PROFILER;

    int COUNT = 50000000;

    calcNumbers(COUNT);
    calcAll(COUNT);
}

int main()
{
    std::thread t1([]() {
        MMETER_SCOPE_PROFILER("main function subthread");
        test();
    });
    t1.join();

    std::ofstream ofs("test.csv", std::ios::out | std::ios::trunc);
    if (!ofs)
    {
        throw std::ios_base::failure("Failed to open the file for writing.");
    }

    std::cout << std::fixed << std::setprecision(6) << MMeter::getGlobalTreePtr()->totalsByDurationStr() << std::endl;
    std::cout << *MMeter::getGlobalTreePtr() << std::endl;
    MMeter::getGlobalTreePtr()->outputBranchPercentagesToOStream(std::cout);
    MMeter::getGlobalTreePtr()->outputBranchDurationsToOStream(std::cout);
    MMeter::getGlobalTreePtr()->outputTotalsCSVToOStream(ofs);
    MMeter::getGlobalTreePtr()->outputTotalsByDurationCSVToOStream(ofs);
}
