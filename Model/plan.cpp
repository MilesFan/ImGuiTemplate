#include "plan.h"
namespace Task
{
	Plan::Plan()
	{
		std::srand((unsigned int)std::time(nullptr));
		ulid::EncodeNowRand(Id);
	}
	/*int Plan::Rows()
	{
		int rows = 0;
		for (const auto& wg : WorkGroups) {
			if (wg.WorkSubGroups.size() == 0)
				rows++;
			else
				rows += (int)wg.WorkSubGroups.size();
		}
		if (rows == 0)
			return 1;
		else
			return rows;
	}
	int WorkGroup::Rows()
	{
		if (WorkSubGroups.size() == 0)
			return 1;
		else
			return (int)WorkSubGroups.size();
	}*/
}