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
#include <utility>
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
 * A comparator for a value class
 */
template <typename ClassT> struct Compare
{
    using CompareTargetT = ClassT;

    virtual bool operator()(const ClassT &a, const ClassT &b) const = 0;
    virtual bool operator()(const ClassT *a, const ClassT *b) const = 0;
};

/**
 * A comparator by a member of a value class
 */
template <typename ClassT, auto p_ResultMember> struct MemberCompare : public Compare<ClassT>
{
    using is_transparent = void;
    using MemberT = decltype(std::declval<ClassT>().*p_ResultMember);
    bool operator()(const ClassT &a, const ClassT &b) const
    {
        return a.*p_ResultMember < b.*p_ResultMember;
    }
    template <typename CompT> bool operator()(const ClassT &a, CompT &&b) const
    {
        return a.*p_ResultMember < b;
    }
    template <typename CompT> bool operator()(CompT &&a, const ClassT &b) const
    {
        return a < b.*p_ResultMember;
    }

    bool operator()(const ClassT *a, const ClassT *b) const
    {
        return a->*p_ResultMember < b->*p_ResultMember;
    }
    template <typename CompT> bool operator()(const ClassT *a, CompT &&b) const
    {
        return a->*p_ResultMember < b;
    }
    template <typename CompT> bool operator()(CompT &&a, const ClassT *b) const
    {
        return a < b->*p_ResultMember;
    }
};

/**
 * An inverse order comparator
 */
template <typename Comparator> struct InverseOrder : public Compare<typename Comparator::CompareTargetT>
{
    using is_transparent = void;

    template <typename... ArgTs> InverseOrder(ArgTs &&...args) : m_comp(std::forward<ArgTs>(args)...)
    {
    }

    bool operator()(const typename Comparator::CompareTargetT &a, const typename Comparator::CompareTargetT &b) const
    {
        return m_comp(b, a);
    }
    bool operator()(const typename Comparator::CompareTargetT *a, const typename Comparator::CompareTargetT *b) const
    {
        return m_comp(b, a);
    }

    template <typename T1, typename T2> bool operator()(T1 &&a, T2 &&b) const
    {
        return m_comp(std::forward<T2>(b), std::forward<T1>(a));
    }
    template <typename T1, typename T2> bool operator()(T1 &&a, T2 &&b)
    {
        return m_comp(std::forward<T2>(b), std::forward<T1>(a));
    }

  protected:
    Comparator m_comp;
};

/**
 * A comparator by a member of a value class
 */
template <typename ClassT, auto p_ResultMethod> struct MethodCompare : public Compare<ClassT>
{
    using is_transparent = void;
    using MemberT = decltype((std::declval<ClassT>().*p_ResultMethod)());
    bool operator()(const ClassT &a, const ClassT &b) const
    {
        return (a.*p_ResultMethod)() < (b.*p_ResultMethod)();
    }
    template <typename CompT> bool operator()(const ClassT &a, CompT &&b) const
    {
        return (a.*p_ResultMethod)() < b;
    }
    template <typename CompT> bool operator()(CompT &&a, const ClassT &b) const
    {
        return a < (b.*p_ResultMethod)();
    }

    bool operator()(const ClassT *a, const ClassT *b) const
    {
        return (a->*p_ResultMethod)() < (b->*p_ResultMethod)();
    }
    template <typename CompT> bool operator()(const ClassT *a, CompT &&b) const
    {
        return (a->*p_ResultMethod)() < b;
    }
    template <typename CompT> bool operator()(CompT &&a, const ClassT *b) const
    {
        return a < (b->*p_ResultMethod)();
    }
};

/**
 * @brief a struct containing the results of a measurement
 */
struct Results
{
    StringView branchName;
    Duration realDuration;
    std::size_t callCount;

    using ByName = MemberCompare<Results, &Results::branchName>;
    using ByDuration = InverseOrder<MemberCompare<Results, &Results::realDuration>>;
    using ByCallCount = InverseOrder<MemberCompare<Results, &Results::callCount>>;
};

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

    StringView mName;
    Duration mDuration, mChoreDuration;
    std::size_t mCount;

  public:
    using ByName = MemberCompare<FuncProfilerTree, &FuncProfilerTree::mName>;
    using ByDuration = InverseOrder<MemberCompare<FuncProfilerTree, &FuncProfilerTree::mDuration>>;
    using ByCallCount = InverseOrder<MemberCompare<FuncProfilerTree, &FuncProfilerTree::mCount>>;

    /*
    Visualization and results
    */

    /**
     * @returns All the branches by their names
     */
    inline const std::set<FuncProfilerTree, ByName> &branches() const
    {
        return mBranches;
    }

    /**
     * @returns The name of this branch
     */
    inline StringView name() const
    {
        return mName;
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
        for (auto &branch : mBranches)
        {
            tot += branch.branchChoreDuration();
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
        for (auto &branch : mBranches)
        {
            subTotal += branch.realDuration();
        }

        return realDuration() - subTotal;
    }

    using ByRealDuration = InverseOrder<MethodCompare<FuncProfilerTree, &FuncProfilerTree::realDuration>>;
    using ByRealNodeDuration = InverseOrder<MethodCompare<FuncProfilerTree, &FuncProfilerTree::realNodeDuration>>;

    /**
     * @returns set of results, summing all references and reentries
     */
    std::set<Results, Results::ByName> totals() const;

    /**
     * @returns vector of results, ordered by the specified Compare, summing all references and reentries
     */
    std::vector<Results> totalsBy(const Compare<Results> &comp) const;

    /**
     * @returns string representation of the tree totals
     */
    String totalsStr(size_t indent = 0, size_t indentSpaces = 4) const;

    /**
     * @returns string representation of the tree totals, ordered by duration
     */
    String totalsStrBy(const Compare<Results> &comp, size_t indent = 0, size_t indentSpaces = 4) const;

    /**
     * @brief Outputs structured tree of branches to a stream, ordered by duration
     * @param out Output stream
     * @param indent Indentation level
     * @param indentSpaces Number of spaces per indentation
     */
    void outputBranchDurationsToOStream(std::ostream &out, size_t indent = 0, size_t indentSpaces = 4) const;

    /*
    Tree manipulation
    */

    /**
     * @brief Name constructor
     */
    explicit FuncProfilerTree(StringView name);

    /**
     * @brief Returns a reference to the branch with the given name, creating it if it doesn't exist
     */
    FuncProfilerTree &existingOrNewBranch(StringView branchName);

    /**
     * @brief simulates a stack frame push
     */
    FuncProfilerTree &stackPush(StringView branchName);

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
    String mBodyName;
    std::set<FuncProfilerTree, ByName> mBranches;
    std::vector<FuncProfilerTree *> mBranchPtrStack;
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