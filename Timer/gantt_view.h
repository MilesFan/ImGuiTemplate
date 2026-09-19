#pragma once
#include <time.h>
#include <vector>
#include "../ImGuiScaffoldSDL3GL/imgui/imgui.h"
#include "../Model/plan.h"

// 甘特图视图:封装布局、视图状态、拖拽状态与全部绘制逻辑
// 数据通过构造注入(Plans 引用),视图不拥有数据
class GanttView
{
public:
	explicit GanttView(std::vector<Task::Plan>& plans);
	void Draw();   // 每帧唯一入口:画布 → 日期表头 → 标签列 → DayWork 网格
private:
	// ---- 布局 ----
	float GridH = 70.0f;                        // 单元格宽(对应一天)
	float GridV = 32.0f;                        // 单元格高(对应一行任务)
	static constexpr int PlanNameCells = 2;     // 计划名列宽(格)
	static constexpr int TaskGroupCells = 2;    // 任务组列宽(格)
	static constexpr int TaskSubGroupCells = 1; // 子任务列宽(格)
	int LabelCells() const { return PlanNameCells + TaskGroupCells + TaskSubGroupCells; }

	// ---- 数据与行模型 ----
	std::vector<Task::Plan>& Plans;
	struct RowRef                               // 扁平化后的任务行;Group<0 为空 Plan 占位行,SubGroup<0 为占位行(空 Plan 或空 WorkGroup)
	{
		int Plan = -1;
		int Group = -1;
		int SubGroup = -1;
	};
	std::vector<RowRef> Rows;

	// ---- 视图状态 ----
	ImVec2 Scrolling = {};       // 按网格吸附后的平移量
	ImVec2 ScrollingReal = {};   // 平移量(未吸附)
	ImVec2 CanvasP0 = {};        // 画布左上角(屏幕坐标)
	ImVec2 CanvasP1 = {};        // 画布右下角(屏幕坐标)
	ImVec2 CanvasCursorPos = {};// 画布起始处的窗口光标位置(绘制后恢复)

	// ---- 拖拽状态(右键拖 DayWork) ----
	bool Dragging = false;       // 正在拖拽
	int DragPlanIdx = -1, DragGroupIdx = -1, DragSubGroupIdx = -1, DragWorkIdx = -1; // 拖拽目标: Plans[i].WorkGroups[j].WorkSubGroups[k].DayWorks[l]
	float DragAnchorX = 0.0f;    // 按下右键时的鼠标 X
	float DragAnchorY = 0.0f;    // 按下右键时的鼠标 Y
	int DragOffsetDays = 0;      // 当前拖拽的水平偏移(天)
	int DragOffsetRows = 0;      // 当前拖拽的垂直偏移(行)
	int DragPreviewRow = -1;     // 吸附后的预览行(松手后落入的任务行)
	std::vector<int> DragSet;    // 一起移动的 DayWorks 索引(Shift 拖拽时为日期连续相邻集合,否则仅单个)
	time_t DragSetMinDate = 0;   // 集合最早日期(标注用)
	time_t DragSetMaxDate = 0;   // 集合最晚日期(标注用)

	// ---- 渲染 ----
	void DrawCanvas();                     // 画布、输入捕获、左键平移、右键菜单、网格线
	void DrawDateHeader(time_t basetime);  // 顶部日期行(今天标红)
	void DrawPlanColumn();                 // 计划名列(跨行标签)
	void DrawGroupColumn();                // 任务组列(跨行标签)
	void DrawSubGroupColumn();             // 子任务列(单行标签)
	void DrawDayWorks(time_t basetime);    // DayWork 单元格、悬停反馈与拖拽预览

	// ---- 行模型 ----
	void RebuildRows();                    // 每帧重建扁平行表
	int RowOf(int plan, int group, int subgroup) const;
	int NearestSubgroupRow(int want) const;// 就近吸附到实际 WorkSubGroup 行(跳过占位行)

	// ---- 拖拽 ----
	void UpdateDrag();                     // 状态机:更新偏移/提交/取消
	void CommitDrag();                     // 松手:提交日期与所属任务的修改
	void CancelDrag();                     // 清空拖拽状态
	void BuildDragSet(bool with_shift);    // 重算一起移动的成员集合
	int FindWorkIndexByDate(int plan, int group, int subgroup, time_t date) const;

	static time_t TodayMidnight();         // 今天 0 点(时间轴基准)
};
