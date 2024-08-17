#include "MMeter.h"

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

    std::cout << std::fixed << std::setprecision(6) << MMeter::getGlobalTreePtr()->totalsByDurationStr() << std::endl;
    std::cout << *MMeter::getGlobalTreePtr();
}