#include "MMeter.h"

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>

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
	{
		MMETER_FUNC_PROFILER;
		test();
	}
	std::cout << std::fixed << std::setprecision(6) << MMeter::getGlobalTree().totalsByDurationStr();
	MMeter::getGlobalTree().outputBranchDurationsToOStream(std::cout);
}