/*
Made by Mauricio Smit
repository: https://github.com/LegendaryMauricius/MMeter

A simple WIP profiler library for c++. Include the 2 files and you're ready to go.
Usage: To profile function execution time put the macro MMETER_FUNC_PROFILER
at the start of any function you want to measure.
To get the duration tree of measured functions use MMeter::getGlobalTree().
You can output the tree directly to any output stream, so you can retrieve
the measurements just by writing std::cout << MMeter::getGlobalTree();
*/

#include <string>
#include <sstream>
#include <ostream>
#include <map>
#include <vector>
#include <set>
#include <chrono>

namespace MMeter
{
	using String = std::string;
	using SStream = std::stringstream;
	using CString = char*;
	using Time = std::chrono::system_clock::time_point;
	using Duration = std::chrono::duration<double>;

	class FuncProfilerTree
	{
	public:
		FuncProfilerTree();

		FuncProfilerTree &operator[](const String& branchName);

		inline const std::map<String, FuncProfilerTree> &branches() const { return mBranches; }
		inline Duration& duration() { return mDuration; }
		inline const Duration& duration() const { return mDuration; }
		inline Duration& choreDuration() { return mChoreDuration; }
		inline const Duration& choreDuration() const { return mChoreDuration; }
		inline Duration realDuration() const { return mDuration - mChoreDuration; }

		FuncProfilerTree &stackPush(const String& branchName);
		void stackPop();
		inline const std::vector<FuncProfilerTree *> &stack() const { return mBranchPtrStack; }

		std::map<String, Duration> totals() const;
		std::set<std::pair<Duration, String>> totalsByDuration() const;
		String totalsStr(size_t indent = 0, size_t indentSpaces = 4) const;
		String totalsByDurationStr(size_t indent = 0, size_t indentSpaces = 4) const;

		void outputBranchDurationsToOStream(std::ostream& out, size_t indent = 0, size_t indentSpaces = 4) const;

	private:
		std::map<String, FuncProfilerTree> mBranches;
		std::vector<FuncProfilerTree *> mBranchPtrStack;
		Duration mDuration, mChoreDuration;
	};

	std::ostream& operator << (std::ostream& out, FuncProfilerTree& tree);

	template<class _OS_T>
	_OS_T& operator << (_OS_T& out, FuncProfilerTree& tree)
	{
		return static_cast<_OS_T &>(static_cast<std::ostream &>(out) << tree);
	}

	class FuncProfiler
	{
	public:
		FuncProfiler(const String& name, FuncProfilerTree* treePtr);
		~FuncProfiler();

	private:
		Time mStartTime;
		Duration mChoresDuration;
		FuncProfilerTree *mTreePtr, *mBranchPtr;
	};

	FuncProfilerTree &getGlobalTree();

} // namespace MMeter

#define MMETER_FUNC_NAME __func__
#define MMETER_FUNC_PROFILER MMeter::FuncProfiler _MMeterProfilerObject(MMETER_FUNC_NAME, &MMeter::getGlobalTree())