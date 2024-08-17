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

using namespace std::chrono_literals;
using std::chrono::duration_cast;

namespace MMeter
{
FuncProfilerTree::FuncProfilerTree() : mDuration(0), mChoreDuration(0)
{
    mBranchPtrStack.push_back(this);
}

FuncProfilerTree &FuncProfilerTree::operator[](const String &branchName)
{
    return mBranches[branchName];
}

FuncProfilerTree &FuncProfilerTree::stackPush(const String &branchName)
{
    mBranchPtrStack.push_back(&(*mBranchPtrStack.back())[branchName]);
    return *mBranchPtrStack.back();
}

void FuncProfilerTree::stackPop()
{
    mBranchPtrStack.pop_back();
}

std::map<StringView, Duration> FuncProfilerTree::totals() const
{
    std::map<StringView, Duration> ret;

    for (auto &nameBranchPair : mBranches)
    {
        ret[nameBranchPair.first] = nameBranchPair.second.realDuration();
    }

    for (auto &nameBranchPair : mBranches)
    {
        auto subTot = nameBranchPair.second.totals();

        for (auto &nameDurPair : subTot)
        {
            auto it = ret.find(nameDurPair.first);
            if (it == ret.end())
            {
                ret[nameDurPair.first] = nameDurPair.second;
            }
            else
            {
                it->second += nameDurPair.second;
            }
        }
    }

    return ret;
}

std::set<std::pair<Duration, StringView>> FuncProfilerTree::totalsByDuration() const
{
    std::set<std::pair<Duration, StringView>> ret;

    for (auto &nameDurPair : totals())
    {
        ret.emplace(nameDurPair.second, nameDurPair.first);
    }

    return ret;
}

String FuncProfilerTree::totalsStr(size_t indent, size_t indentSpaces) const
{
    auto tot = totals();
    SStream ss;

    /*for (size_t i = 0; i < indent; i++)
        ss << ' ';

    ss << realDuration().count() << "s" << std::endl;*/

    for (auto nameDurPair : tot)
    {
        for (size_t i = 0; i < indent * indentSpaces; i++)
            ss << ' ';

        ss << nameDurPair.first << ": " << nameDurPair.second.count() << "s" << std::endl;
    }

    return ss.str();
}

String FuncProfilerTree::totalsByDurationStr(size_t indent, size_t indentSpaces) const
{
    auto tot = totalsByDuration();
    SStream ss;

    for (auto nameDurPair : tot)
    {
        for (size_t i = 0; i < indent * indentSpaces; i++)
            ss << ' ';

        ss << nameDurPair.first.count() << "s - " << nameDurPair.second << std::endl;
    }

    return ss.str();
}

void FuncProfilerTree::outputBranchDurationsToOStream(std::ostream &out, size_t indent, size_t indentSpaces) const
{
    for (auto &nameBranchPair : mBranches)
    {
        for (size_t i = 0; i < indent * indentSpaces; i++)
            out << ' ';

        out << nameBranchPair.first << ": " << nameBranchPair.second.realDuration().count() << "s" << std::endl;
        nameBranchPair.second.outputBranchDurationsToOStream(out, indent + 1, indentSpaces);
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
    mBranchPtr->measuredDuration() += endTime - mStartTime;
    auto parentBranchPtr = mTreePtr->stack()[mTreePtr->stack().size() - 2];
    mChoresDuration += std::chrono::system_clock::now() - endTime;
    mTreePtr->stackPop();
    mBranchPtr->measuredNodeChoreDuration() += mChoresDuration;
}

FuncProfilerTree &getGlobalTree()
{
    static FuncProfilerTree tree;
    return tree;
}
} // namespace MMeter