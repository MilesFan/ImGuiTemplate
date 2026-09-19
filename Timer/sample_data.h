#pragma once
#include <vector>
#include "../Model/plan.h"
// 随机生成演示用计划数据(日期均在 2026 年 9 月内,每次启动不同)
std::vector<Task::Plan> makeSamplePlans();
