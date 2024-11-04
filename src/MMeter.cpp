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

    for (auto &nameBranchPair : mBranches)
    {
        ret.emplace(nameBranchPair.first,
                    Results(nameBranchPair.first, nameBranchPair.second.realDuration(), nameBranchPair.second.mCount));
    }
    if (mDuration.count() > 0)
    {
        ret.emplace("<body>", Results("<body>", realNodeDuration(), mCount));
    }

    for (auto &nameBranchPair : mBranches)
    {
        auto subTot = nameBranchPair.second.totals();

        for (auto &nameResultPair : subTot)
        {
            auto it = ret.find(nameResultPair.first);
            if (it == ret.end())
            {
                ret.emplace(nameResultPair.first, nameResultPair.second);
            }
            else
            {
                it->second.realDuration += nameResultPair.second.realDuration;
                it->second.callCount += nameResultPair.second.callCount;
            }
        }
    }

    return ret;
}

std::set<std::pair<Duration, Results>> FuncProfilerTree::totalsByDuration() const
{
    std::set<std::pair<Duration, Results>> ret;

    for (auto [name, result] : totals())
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

    for (auto [name, result] : totals())
    {
        for (size_t i = 0; i < indent * indentSpaces; i++)
            ss << ' ';

        ss << name << ": " << result.realDuration.count() << "s /#" << result.callCount << std::endl;
    }

    return ss.str();
}

void FuncProfilerTree::outputTotalsCSVToOStream(std::ostream &out) const
{
    SStream ss;
    out << "Function Name,Time (s),Call Number\n";

    for (auto [name, result] : totals())
    {
        out << name << "," << result.realDuration.count() << "," << result.callCount << std::endl;
    }
}

String FuncProfilerTree::totalsByDurationStr(size_t indent, size_t indentSpaces) const
{
    SStream ss;

    for (auto [duration, result] : totalsByDuration())
    {
        for (size_t i = 0; i < indent * indentSpaces; i++)
            ss << ' ';

        ss << duration.count() << "s /#" << result.callCount << " - " << result.branchName << std::endl;
    }

    return ss.str();
}

void FuncProfilerTree::outputTotalsByDurationCSVToOStream(std::ostream &out) const
{
    out << "Time (s),Call Number,Function Name\n";

    for (auto [duration, result] : totalsByDuration())
    {
        out << duration.count() << "," << result.callCount << "," << result.branchName << std::endl;
    }
}

void FuncProfilerTree::outputBranchDurationsToOStream(std::ostream &out, size_t indent, size_t indentSpaces) const
{
    if (mBranches.size() > 0)
    {
        std::set<std::pair<Duration, const decltype(mBranches)::value_type *>> durationPtrPairs;

        for (auto &nameBranchPair : mBranches)
        {
            durationPtrPairs.emplace(nameBranchPair.second.realDuration(), &nameBranchPair);
        }
        if (mDuration.count() > 0)
        {
            durationPtrPairs.emplace(realNodeDuration(), nullptr);
        }

        for (auto &durationPtrPair : durationPtrPairs)
        {
            for (size_t i = 0; i < indent * indentSpaces; i++)
                out << ' ';

            out << durationPtrPair.first.count() << "s /#";
            if (durationPtrPair.second == nullptr)
            {
                out << mCount << " - "
                    << "<body>" << std::endl;
            }
            else
            {
                out << durationPtrPair.second->second.mCount << " - " << durationPtrPair.second->first << std::endl;
                durationPtrPair.second->second.outputBranchDurationsToOStream(out, indent + 1, indentSpaces);
            }
        }
    }
}

void FuncProfilerTree::outputBranchPercentagesToOStream(std::ostream &out, size_t indent, size_t indentSpaces) const
{
    if (mBranches.size() > 0)
    {
        auto totalDur = realDuration();

        std::set<std::pair<Duration, const decltype(mBranches)::value_type *>> durationPtrPairs;

        for (auto &nameBranchPair : mBranches)
        {
            durationPtrPairs.emplace(nameBranchPair.second.realDuration(), &nameBranchPair);
        }
        if (mDuration.count() > 0)
        {
            durationPtrPairs.emplace(realNodeDuration(), nullptr);
        }

        for (auto &durationPtrPair : durationPtrPairs)
        {
            for (size_t i = 0; i < indent * indentSpaces; i++)
                out << ' ';

            if (mDuration.count() > 0)
            {
                out << (durationPtrPair.first.count() / totalDur.count() * 100.0) << "% /";
            }
            else
            {
                out << durationPtrPair.first.count() << "s /";
            }

            if (durationPtrPair.second == nullptr)
            {
                out << "^" << 1.0 << " - <body>" << std::endl;
            }
            else
            {
                if (mDuration.count() > 0)
                {
                    out << "^" << ((double)durationPtrPair.second->second.mCount / (double)mCount) << " - ";
                }
                else
                {
                    out << "#" << durationPtrPair.second->second.mCount << " - ";
                }
                out << durationPtrPair.second->first << std::endl;
                durationPtrPair.second->second.outputBranchPercentagesToOStream(out, indent + 1, indentSpaces);
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
