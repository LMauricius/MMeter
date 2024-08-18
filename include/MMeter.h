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

#pragma once
#ifndef INCLUDED_MMETER_H
#define INCLUDED_MMETER_H

#include <chrono>
#include <map>
#include <ostream>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#ifndef MMETER_ENABLE
/**
 * Whether to enable the profiler for this unit
 * 0 = disable
 * 1 = enable
 * @note if undefined, it is enabled by default
 */
#define MMETER_ENABLE 1
#endif

/**
 * @brief The name of the function being measured
 */
#define MMETER_FUNC_NAME __func__

#if MMETER_ENABLE == 1

/**
 * @brief A scope guard macro for function execution timing measurement
 * @note put this at the beginning of a function scope
 * @warning only one such guard can be used per scope
 */
#define MMETER_FUNC_PROFILER                                                                                           \
    MMeter::FuncProfiler _MMeterProfilerObject(std::chrono::system_clock::now(), MMETER_FUNC_NAME,                     \
                                               MMeter::getThreadLocalTreePtr())

/**
 * @brief A scope guard macro for block execution timing measurement
 * @param name the name of the block
 * @note put this at the beginning of a function scope
 * @warning only one such guard can be used per scope
 */
#define MMETER_SCOPE_PROFILER(name)                                                                                    \
    MMeter::FuncProfiler _MMeterProfilerObject(std::chrono::system_clock::now(), name, MMeter::getThreadLocalTreePtr())

#else

#define MMETER_FUNC_PROFILER
#define MMETER_SCOPE_PROFILER(name)

#endif

namespace MMeter
{
using String = std::string;
using StringView = std::string_view;
using SStream = std::stringstream;
using CString = const char *;
using Time = std::chrono::system_clock::time_point;
using Duration = std::chrono::duration<double>;

class FuncProfilerTree;

/**
 * @brief prints a FuncProfilerTree to the output stream
 * @param out the output stream
 * @param tree the tree to print
 * @returns the output stream
 * @note behaves the same as FuncProfilerTree::outputBranchDurationsToOStream
 */
std::ostream &operator<<(std::ostream &out, FuncProfilerTree &tree);

/**
 * @brief prints a FuncProfilerTree to the output stream
 * @param out the output stream
 * @param tree the tree to print
 * @returns the output stream
 * @note behaves the same as FuncProfilerTree::outputBranchDurationsToOStream
 */
template <class _OS_T> _OS_T &operator<<(_OS_T &out, FuncProfilerTree &tree)
{
    return static_cast<_OS_T &>(static_cast<std::ostream &>(out) << tree);
}

/**
 * @returns a pointer to this thread's FuncProfilerTree
 */
FuncProfilerTree *getThreadLocalTreePtr();

/**
 * @brief a thread-safe pointer to the global FuncProfilerTree
 */
class GlobalFuncProfilerTreePtr
{
  public:
    GlobalFuncProfilerTreePtr();
    ~GlobalFuncProfilerTreePtr();

    FuncProfilerTree &operator*() const;
    FuncProfilerTree *operator->() const;
};

/**
 * @returns a thread-safe pointer to the global FuncProfilerTree
 * @note the tree access is locked by this thread as long as any GlobalFuncProfilerTreePtr exists
 */
GlobalFuncProfilerTreePtr getGlobalTreePtr();

/**
 * @brief A branch of scope tree execution timing measurements
 */
class FuncProfilerTree
{
    friend class FuncProfiler;

  public:
    /*
    Visualization and results
    */

    /**
     * @returns All the branches by their names
     */
    inline const std::map<String, FuncProfilerTree> &branches() const
    {
        return mBranches;
    }
    /**
     * @returns duration of this branch, including the chores and subbranches
     */
    inline Duration measuredDuration() const
    {
        return mDuration;
    }

    /**
     * @returns duration of the chores in this branch, excluding the subbranches
     */
    inline Duration measuredNodeChoreDuration() const
    {
        return mChoreDuration;
    }

    /**
     * @returns duration of the chores in this branch, including the subbranches
     */
    inline Duration branchChoreDuration() const
    {
        Duration tot = mChoreDuration;
        for (auto &nameBranchPair : mBranches)
        {
            tot += nameBranchPair.second.branchChoreDuration();
        }
        return tot;
    }

    /**
     * @returns duration of code execution in this branch, without chores, including the subbranches
     */
    inline Duration realDuration() const
    {
        return mDuration - branchChoreDuration();
    }

    /**
     * @returns duration of code execution in this branch, without chores, excluding the subbranches
     */
    inline Duration realNodeDuration() const
    {
        Duration subTotal = Duration::zero();
        for (auto &nameBranchPair : mBranches)
        {
            subTotal += nameBranchPair.second.realDuration();
        }

        return realDuration() - subTotal;
    }

    /**
     * @returns map of branch names to their real durations, summing all references and reentries
     */
    std::map<StringView, Duration> totals() const;

    /**
     * @returns pairs of real durations and branch names, ordered by duration, summing all references and reentries
     */
    std::set<std::pair<Duration, StringView>> totalsByDuration() const;

    /**
     * @returns string representation of the tree totals
     */
    String totalsStr(size_t indent = 0, size_t indentSpaces = 4) const;

    /**
     * @returns string representation of the tree totals, ordered by duration
     */
    String totalsByDurationStr(size_t indent = 0, size_t indentSpaces = 4) const;

    /**
     * @brief Outputs structured tree of branches to a stream, ordered by duration
     * @param out Output stream
     * @param indent Indentation level
     * @param indentSpaces Number of spaces per indentation
     */
    void outputBranchDurationsToOStream(std::ostream &out, size_t indent = 0, size_t indentSpaces = 4) const;

    /**
     * @brief Outputs structured tree of branches to a stream, ordered by duration. Duration expressed relatively.
     * @param out Output stream
     * @param indent Indentation level
     * @param indentSpaces Number of spaces per indentation
     */
    void outputBranchPercentagesToOStream(std::ostream &out, size_t indent = 0, size_t indentSpaces = 4) const;

    /*
    Tree manipulation
    */

    /**
     * @brief Default constructor
     */
    FuncProfilerTree();

    /**
     * @brief Returns a reference to the branch with the given name, creating it if it doesn't exist
     */
    FuncProfilerTree &existingOrNewBranch(const String &branchName);

    /**
     * @brief simulates a stack frame push
     */
    FuncProfilerTree &stackPush(const String &branchName);

    /**
     * @brief simulates a stack frame pop
     */
    void stackPop();

    /**
     * @returns the stack of currently called branch pointers
     */
    inline const std::vector<FuncProfilerTree *> &stack() const
    {
        return mBranchPtrStack;
    }

    /**
     * @brief resets the tree to its initial state, before tracking anything
     */
    void reset();

    /**
     * @brief Merges another tree into this one
     */
    void merge(const FuncProfilerTree &tree);

  private:
    std::map<String, FuncProfilerTree> mBranches;
    std::vector<FuncProfilerTree *> mBranchPtrStack;
    Duration mDuration, mChoreDuration;
    std::size_t mCount;
};

/**
 * @brief A scope guard class for measurements
 */
class FuncProfiler
{
  public:
    FuncProfiler(Time startTime, CString name, FuncProfilerTree *treePtr);
    ~FuncProfiler();

  private:
    Time mStartTime;
    Duration mChoresDuration;
    FuncProfilerTree *mTreePtr, *mBranchPtr;
};

} // namespace MMeter

#endif // INCLUDED_MMETER_H