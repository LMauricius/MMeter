#include "MMeter.h"

using namespace std::chrono_literals;
using std::chrono::duration_cast;

namespace MMeter
{
	FuncProfilerTree::FuncProfilerTree():
		mDuration(0),
		mChoreDuration(0)
	{
		mBranchPtrStack.push_back(this);
	}

	FuncProfilerTree &FuncProfilerTree::operator[](const String& branchName)
	{
		return mBranches[branchName];
	}

	FuncProfilerTree &FuncProfilerTree::stackPush(const String& branchName)
	{
		mBranchPtrStack.push_back(&(*mBranchPtrStack.back())[branchName]);
		return *mBranchPtrStack.back();
	}

	void FuncProfilerTree::stackPop()
	{
		mBranchPtrStack.pop_back();
	}
	
	std::map<String, Duration> FuncProfilerTree::totals() const
	{
		std::map<String, Duration> ret;

		for (auto& nameBranchPair : mBranches)
		{
			ret[nameBranchPair.first] = nameBranchPair.second.realDuration();
		}

		for (auto& nameBranchPair : mBranches)
		{
			auto subTot = nameBranchPair.second.totals();

			for (auto& nameDurPair : subTot)
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

	std::set<std::pair<Duration, String>> FuncProfilerTree::totalsByDuration() const
	{
		std::set<std::pair<Duration, String>> ret;

		for (auto& nameDurPair : totals())
		{
			ret.insert({nameDurPair.second, nameDurPair.first});
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
			for (size_t i = 0; i < indent*indentSpaces; i++)
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
			for (size_t i = 0; i < indent*indentSpaces; i++)
				ss << ' ';
			
			ss << nameDurPair.first.count() << "s - " << nameDurPair.second << std::endl;
		}

		return ss.str();
	}

	void FuncProfilerTree::outputBranchDurationsToOStream(std::ostream& out, size_t indent, size_t indentSpaces) const
	{
		for (auto& nameBranchPair : mBranches)
		{
			for (size_t i = 0; i < indent*indentSpaces; i++)
				out << ' ';
			
			out << nameBranchPair.first << ": " << nameBranchPair.second.realDuration().count() << "s" << std::endl;
			nameBranchPair.second.outputBranchDurationsToOStream(out, indent + 1, indentSpaces);
		}
	}

	std::ostream& operator << (std::ostream& out, FuncProfilerTree& tree)
	{
		//out << tree.realDuration().count() << "s" << std::endl;
		tree.outputBranchDurationsToOStream(out, 0);
		return out;
	}


	FuncProfiler::FuncProfiler(const String& name, FuncProfilerTree* treePtr):
		mTreePtr(treePtr)
	{
		mStartTime = std::chrono::system_clock::now();
		
		mBranchPtr = &treePtr->stackPush(name);
		mChoresDuration = std::chrono::system_clock::now() - mStartTime;
		for (auto ptr : treePtr->stack())
		{
			ptr->choreDuration() += mChoresDuration;
		}
	}

	FuncProfiler::~FuncProfiler()
	{
		mBranchPtr->duration() += std::chrono::system_clock::now() - mStartTime;
		mTreePtr->stackPop();
	}

	FuncProfilerTree &getGlobalTree()
	{
		static FuncProfilerTree tree;
		return tree;
	}
}