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
#include <string>
#include <thread>

using namespace std::chrono_literals;
using std::chrono::duration_cast;

namespace MMeter
{

// Utility
namespace
{
struct StringFormatter
{
    StringView newlineStr = "\n";
};

struct FmtStringView
{
    StringView view;
    const StringFormatter &formatter;
};

template <typename OSS> OSS &operator<<(OSS &oss, const FmtStringView &fmtStr)
{
    for (auto c : fmtStr.view)
    {
        if (c == '\n')
            oss << fmtStr.formatter.newlineStr;
        else
            oss << c;
    }
    return oss;
}

} // namespace

FuncProfilerTree::FuncProfilerTree(StringView name) : mName(name), mDuration(0), mChoreDuration(0), mCount(0)
{
    mBranchPtrStack.push_back(this);
}

FuncProfilerTree &FuncProfilerTree::existingOrNewBranch(StringView branchName)
{
    auto it = mBranches.find(branchName);

    if (it == mBranches.end())
        it = mBranches.emplace(branchName).first;

    return const_cast<FuncProfilerTree &>(*it);
}

FuncProfilerTree &FuncProfilerTree::stackPush(StringView branchName)
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

    for (auto &branch : tree.mBranches)
    {
        (*this).existingOrNewBranch(branch.name()).merge(branch);
    }
}

std::set<Results, Results::ByName> FuncProfilerTree::totals() const
{
    std::set<Results, Results::ByName> ret;

    for (auto &branch : mBranches)
    {
        ret.emplace(branch.name(), branch.realDuration(), branch.mCount);
    }
    if (mDuration.count() > 0)
    {
        ret.emplace("<body>", realNodeDuration(), mCount);
    }

    for (auto &branch : mBranches)
    {
        auto subTot = branch.totals();

        for (auto &result : subTot)
        {
            auto it = ret.find(result.branchName);
            if (it == ret.end())
            {
                ret.emplace(result);
            }
            else
            {
                const_cast<Results &>(*it).realDuration += result.realDuration;
                const_cast<Results &>(*it).callCount += result.callCount;
            }
        }
    }

    return ret;
}

std::vector<Results> FuncProfilerTree::totalsBy(const Compare<Results> &comp) const
{
    auto res = totals();
    std::vector<Results> ret{res.begin(), res.end()};

    std::sort(ret.begin(), ret.end(), [&](const Results &a, const Results &b) { return comp(a, b); });

    return ret;
}

String FuncProfilerTree::totalsStr(size_t indent, size_t indentSpaces) const
{
    SStream ss;

    auto resMap = totals();
    auto resVector = std::vector(resMap.rbegin(), resMap.rend());

    for (auto &result : resVector)
    {
        for (size_t i = 0; i < indent; i++) {
            ss << '|';
            for (size_t j = 0; j < indentSpaces; j++) {
                ss << ' ';
            }
        }

        ss << '+' << result.branchName << ": " << result.realDuration.count() << "s /" << result.callCount << std::endl;
    }

    return ss.str();
}

String FuncProfilerTree::totalsStrBy(const Compare<Results> &comp, size_t indent, size_t indentSpaces) const
{
    SStream indents;
    for (size_t i = 0; i < indent; i++)
    {
        indents << '|';
        for (size_t j = 0; j < indentSpaces; j++)
        {
            indents << ' ';
        }
    }
    String indentstr = indents.str();
    String newlinestr = "\n" + indentstr + "| ";
    StringFormatter fmtr{newlinestr};

    SStream ss;

    auto resVector = totalsBy(comp);

    for (auto &result : resVector)
    {
        ss << indentstr << '+' << result.realDuration.count() << "s /" << result.callCount << " - "
           << FmtStringView{result.branchName, fmtr} << std::endl;
    }

    return ss.str();
}

void FuncProfilerTree::outputBranchDurationsToOStream(std::ostream &out, size_t indent, size_t indentSpaces) const
{
    if (mBranches.size() > 0)
    {
        auto totalDur = realDuration();

        std::set<const FuncProfilerTree *, FuncProfilerTree::ByRealDuration> branchPtrs;

        for (auto &branch : mBranches)
        {
            branchPtrs.emplace(&branch);
        }
        if (mDuration.count() > 0)
        {
            branchPtrs.emplace(this);
        }

        SStream indents;
        for (size_t i = 0; i < indent; i++)
        {
            indents << '|';
            for (size_t j = 0; j < indentSpaces; j++)
            {
                indents << ' ';
            }
        }
        String indentstr = indents.str();
        String newlinestr = "\n" + indentstr + "| ";
        StringFormatter fmtr{newlinestr};

        for (auto p_branch : branchPtrs)
        {
            // Entry start
            out << indentstr << '+';

            // Duration (s)
            out << p_branch->realDuration().count() << "s ";

            // Duration (relative)
            if (mDuration.count() > 0)
            {
                out << (p_branch->realDuration().count() / totalDur.count() * 100.0) << "% ";
            }

            // Count (total)
            out << "/" << p_branch->mCount << " ";

            // Count (relative)
            if (mDuration.count() > 0)
            {
                out << "~" << ((double)p_branch->mCount / (double)mCount) << " ";
            }

            // Name
            out << "- ";

            if (p_branch == this)
            {
                out << "<body>" << std::endl;
            }
            else
            {
                out << FmtStringView{p_branch->name(), fmtr} << std::endl;
                p_branch->outputBranchDurationsToOStream(out, indent + 1, indentSpaces);
            }
        }
    }
}

/*void FuncProfilerTree::outputBranchPercentagesToOStream(std::ostream &out, size_t indent, size_t indentSpaces) const
{
    if (mBranches.size() > 0)
    {
        auto totalDur = realDuration();

        std::set<std::pair<Duration, const FuncProfilerTree *>,
                 std::greater<std::pair<Duration, const FuncProfilerTree *>>>
            durationPtrPairs;

        for (auto &branch : mBranches)
        {
            durationPtrPairs.emplace(branch.realDuration(), &branch);
        }
        if (mDuration.count() > 0)
        {
            durationPtrPairs.emplace(realNodeDuration(), nullptr);
        }

        SStream indents;
        for (size_t i = 0; i < indent; i++)
        {
            indents << '|';
            for (size_t j = 0; j < indentSpaces; j++)
            {
                indents << ' ';
            }
        }
        String indentstr = indents.str();
        String newlinestr = "\n" + indentstr + "| ";
        StringFormatter fmtr{newlinestr};

        for (auto &[duration, nameBranchPairPtr] : durationPtrPairs)
        {
            if (mDuration.count() > 0)
            {
                out << indentstr << '+' << (duration.count() / totalDur.count() * 100.0) << "% /";
            }
            else
            {
                out << indentstr << '+' << duration.count() << "s /";
            }

            if (nameBranchPairPtr == nullptr)
            {
                out << "~" << 1.0 << " - <body>" << name << std::endl;
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
                out << FmtStringView{name, fmtr} << std::endl;
                branch.outputBranchPercentagesToOStream(out, indent + 1, indentSpaces);
            }
        }
    }
}*/

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

FuncProfilerTree globalTree{"Global"};
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

    FuncProfilerTree localTree{"This thread"};
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