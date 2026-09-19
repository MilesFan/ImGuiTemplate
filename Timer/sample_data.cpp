#include "sample_data.h"
#include "../Model/ganntview.h" // make_time
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <time.h>

// [min, max] 闭区间随机整数
static int randRange(int min, int max)
{
	return min + rand() % (max - min + 1);
}

std::vector<Task::Plan> makeSamplePlans()
{
	std::srand((unsigned int)std::time(nullptr));
	static const char* personNames[] = { "严", "李", "张", "王", "何", "刘", "陈", "赵" };
	static const char* groupNames[] = { "合同", "动力系统", "转向系统", "空调系统", "照明系统", "液压系统", "制动系统", "电气系统" };
	static const char* subGroupNames[] = { "设计", "检查", "解读", "评审", "测试", "标定" };
	constexpr int personCount = sizeof(personNames) / sizeof(personNames[0]);
	constexpr int groupCount = sizeof(groupNames) / sizeof(groupNames[0]);
	constexpr int subGroupCount = sizeof(subGroupNames) / sizeof(subGroupNames[0]);

	std::vector<Task::Plan> plans;
	int planTotal = randRange(20, 30);
	for (int p = 0; p < planTotal; ++p)
	{
		char planName[32];
		sprintf_s(planName, sizeof(planName), "Order%04d", randRange(1, 9999));

		std::vector<Task::WorkGroup> groups;
		int groupTotal = randRange(0, 5);   // 0 → 空 Plan 占位行
		for (int g = 0; g < groupTotal; ++g)
		{
			std::vector<Task::WorkSubGroup> subGroups;
			int subTotal = randRange(0, 3); // 0 → 空 WorkGroup 占位行
			for (int s = 0; s < subTotal; ++s)
			{
				std::string subName = subGroupNames[randRange(0, subGroupCount - 1)];
				if (randRange(1, 10) <= 2)
				{
					// 少量子任务暂无排班(保留空 DayWorks 情形)
					subGroups.push_back(Task::WorkSubGroup(std::move(subName), {}));
					continue;
				}
				// 同一子任务由同一人连续工作 1~5 天,日期落在 2026 年 9 月内
				int workDays = randRange(1, 5);
				int startDay = randRange(1, 30 - workDays + 1);
				std::string person = personNames[randRange(0, personCount - 1)];
				std::vector<Task::DayWork> dayWorks;
				for (int d = 0; d < workDays; ++d)
					dayWorks.push_back(Task::DayWork(person, make_time(2026, 9, startDay + d)));
				subGroups.push_back(Task::WorkSubGroup(std::move(subName), std::move(dayWorks)));
			}
			groups.push_back(Task::WorkGroup(groupNames[randRange(0, groupCount - 1)], std::move(subGroups)));
		}
		plans.push_back(Task::Plan(planName, std::move(groups)));
	}
	return plans;
}
