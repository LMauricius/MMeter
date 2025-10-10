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

#include "MMeter.h"

#include <mutex>

using namespace std::chrono_literals;
using std::chrono::duration_cast;

namespace MMeter
{
FuncProfilerTree::FuncProfilerTree() : mDuration(0), mChoreDuration(0), mCount(0)
{
    mBranchPtrStack.push_back(this);
}

FuncProfilerTree &FuncProfilerTree::existingOrNewBranch(const String &branchName)
{
    return mBranches[branchName];
}

FuncProfilerTree &FuncProfilerTree::stackPush(const String &branchName)
{
    mBranchPtrStack.push_back(&(*mBranchPtrStack.back()).existingOrNewBranch(branchName));
    return *mBranchPtrStack.back();
}

void FuncProfilerTree::stackPop()
{
    mBranchPtrStack.pop_back();
}

void FuncProfilerTree::reset()
{
    mDuration = Duration::zero();
    mChoreDuration = Duration::zero();
    mCount = 0;
    mBranches.clear();
    mBranchPtrStack.clear();
    mBranchPtrStack.push_back(this);
}

void FuncProfilerTree::merge(const FuncProfilerTree &tree)
{
    mDuration += tree.mDuration;
    mChoreDuration += tree.mChoreDuration;
    mCount += tree.mCount;

    for (auto &nameBranchPair : tree.mBranches)
    {
        (*this).existingOrNewBranch(nameBranchPair.first).merge(nameBranchPair.second);
    }
}

std::map<StringView, Results> FuncProfilerTree::totals() const
{
    std::map<StringView, Results> ret;

    for (auto &[name, branch] : mBranches)
    {
        ret.emplace(name, Results(name, branch.realDuration(), branch.mCount));
    }
    if (mDuration.count() > 0)
    {
        ret.emplace("<body>", Results("<body>", realNodeDuration(), mCount));
    }

    for (auto &[name, branch] : mBranches)
    {
        auto subTot = branch.totals();

        for (auto &[subName, result] : subTot)
        {
            auto it = ret.find(subName);
            if (it == ret.end())
            {
                ret.emplace(subName, result);
            }
            else
            {
                it->second.realDuration += result.realDuration;
                it->second.callCount += result.callCount;
            }
        }
    }

    return ret;
}

std::set<std::pair<Duration, Results>> FuncProfilerTree::totalsByDuration() const
{
    std::set<std::pair<Duration, Results>> ret;

    for (auto &[name, result] : totals())
    {
        ret.emplace(result.realDuration, result);
    }

    return ret;
}

std::set<std::pair<std::size_t, Results>> FuncProfilerTree::totalsByCallCount() const
{
    std::set<std::pair<std::size_t, Results>> ret;

    for (auto &nameResultPair : totals())
    {
        ret.emplace(nameResultPair.second.callCount, nameResultPair.second);
    }

    return ret;
}

String FuncProfilerTree::totalsStr(size_t indent, size_t indentSpaces) const
{
    SStream ss;

    auto resMap = totals();
    auto resVector = std::vector(resMap.rbegin(), resMap.rend());

    for (auto &[name, result] : resVector)
    {
        for (size_t i = 0; i < indent; i++) {
            ss << '|';
            for (size_t j = 0; j < indentSpaces; j++) {
                ss << ' ';
            }
        }

        ss << '+' << name << ": " << result.realDuration.count() << "s /" << result.callCount << std::endl;
    }

    return ss.str();
}

String FuncProfilerTree::totalsByDurationStr(size_t indent, size_t indentSpaces) const
{
    SStream ss;

    auto resSet = totalsByDuration();
    auto resVector = std::vector(resSet.rbegin(), resSet.rend());

    for (auto &[duration, result] : resVector)
    {
        for (size_t i = 0; i < indent; i++)
        {
            ss << '|';
            for (size_t j = 0; j < indentSpaces; j++)
            {
                ss << ' ';
            }
        }

        ss << '+' << duration.count() << "s /" << result.callCount << " - " << result.branchName << std::endl;
    }

    return ss.str();
}

void FuncProfilerTree::outputBranchDurationsToOStream(std::ostream &out, size_t indent, size_t indentSpaces) const
{
    if (mBranches.size() > 0)
    {
        std::set<std::pair<Duration, const decltype(mBranches)::value_type *>,
                 std::greater<std::pair<Duration, const decltype(mBranches)::value_type *>>>
            durationPtrPairs;

        for (auto &nameBranchPair : mBranches)
        {
            durationPtrPairs.emplace(nameBranchPair.second.realDuration(), &nameBranchPair);
        }
        if (mDuration.count() > 0)
        {
            durationPtrPairs.emplace(realNodeDuration(), nullptr);
        }

        for (auto &[duration, nameBranchPairPtr] : durationPtrPairs)
        {
            for (size_t i = 0; i < indent; i++)
            {
                out << '|';
                for (size_t j = 0; j < indentSpaces; j++)
                {
                    out << ' ';
                }
            }

            out << '+' << duration.count() << "s /";
            if (nameBranchPairPtr == nullptr)
            {
                out << mCount << " - " << "<body>" << std::endl;
            }
            else
            {
                auto &name = nameBranchPairPtr->first;
                auto &branch = nameBranchPairPtr->second;

                out << branch.mCount << " - " << name << std::endl;
                branch.outputBranchDurationsToOStream(out, indent + 1, indentSpaces);
            }
        }
    }
}

void FuncProfilerTree::outputBranchPercentagesToOStream(std::ostream &out, size_t indent, size_t indentSpaces) const
{
    if (mBranches.size() > 0)
    {
        auto totalDur = realDuration();

        std::set<std::pair<Duration, const decltype(mBranches)::value_type *>,
                 std::greater<std::pair<Duration, const decltype(mBranches)::value_type *>>>
            durationPtrPairs;

        for (auto &nameBranchPair : mBranches)
        {
            durationPtrPairs.emplace(nameBranchPair.second.realDuration(), &nameBranchPair);
        }
        if (mDuration.count() > 0)
        {
            durationPtrPairs.emplace(realNodeDuration(), nullptr);
        }

        for (auto &[duration, nameBranchPairPtr] : durationPtrPairs)
        {
            for (size_t i = 0; i < indent; i++)
            {
                out << '|';
                for (size_t j = 0; j < indentSpaces; j++)
                {
                    out << ' ';
                }
            }

            if (mDuration.count() > 0)
            {
                out << '+' << (duration.count() / totalDur.count() * 100.0) << "% /";
            }
            else
            {
                out << '+' << duration.count() << "s /";
            }

            if (nameBranchPairPtr == nullptr)
            {
                out << "~" << 1.0 << " - <body>" << std::endl;
            }
            else
            {
                auto &name = nameBranchPairPtr->first;
                auto &branch = nameBranchPairPtr->second;

                if (mDuration.count() > 0)
                {
                    out << "~" << ((double)branch.mCount / (double)mCount) << " - ";
                }
                else
                {
                    out << branch.mCount << " - ";
                }
                out << name << std::endl;
                branch.outputBranchPercentagesToOStream(out, indent + 1, indentSpaces);
            }
        }
    }
}

std::ostream &operator<<(std::ostream &out, FuncProfilerTree &tree)
{
    // out << tree.realDuration().count() << "s" << std::endl;
    tree.outputBranchDurationsToOStream(out, 0);
    return out;
}

FuncProfiler::FuncProfiler(Time startTime, CString name, FuncProfilerTree *treePtr) : mTreePtr(treePtr)
{
    mStartTime = startTime;

    mBranchPtr = &treePtr->stackPush(name);
    mChoresDuration = std::chrono::system_clock::now() - mStartTime;
}

FuncProfiler::~FuncProfiler()
{
    auto endTime = std::chrono::system_clock::now();
    mBranchPtr->mDuration += endTime - mStartTime;
    mBranchPtr->mCount++;
    auto parentBranchPtr = mTreePtr->stack()[mTreePtr->stack().size() - 2];
    mTreePtr->stackPop();
    mChoresDuration += std::chrono::system_clock::now() - endTime;
    mBranchPtr->mChoreDuration += mChoresDuration;
}

namespace
{

FuncProfilerTree globalTree;
std::recursive_mutex globalTreeMutex;

class ThreadFuncProfilerTreeWrapper
{
  public:
    ThreadFuncProfilerTreeWrapper() = default;
    ~ThreadFuncProfilerTreeWrapper()
    {
        globalTreeMutex.lock();
        globalTree.merge(localTree);
        globalTreeMutex.unlock();
    }

    FuncProfilerTree localTree;
};

thread_local ThreadFuncProfilerTreeWrapper threadTreeWrapper;

} // namespace

GlobalFuncProfilerTreePtr::GlobalFuncProfilerTreePtr()
{
    globalTreeMutex.lock();
}

GlobalFuncProfilerTreePtr::~GlobalFuncProfilerTreePtr()
{
    globalTreeMutex.unlock();
}

FuncProfilerTree &GlobalFuncProfilerTreePtr::operator*() const
{
    return globalTree;
}

FuncProfilerTree *GlobalFuncProfilerTreePtr::operator->() const
{
    return &globalTree;
}

GlobalFuncProfilerTreePtr getGlobalTreePtr()
{
    return GlobalFuncProfilerTreePtr();
}

FuncProfilerTree *getThreadLocalTreePtr()
{
    return &threadTreeWrapper.localTree;
}

} // namespace MMeter