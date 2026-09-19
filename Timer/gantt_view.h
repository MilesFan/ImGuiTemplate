#pragma once
#include <time.h>
#include <vector>
#include "../ImGuiScaffoldSDL3GL/imgui/imgui.h"
#include "../Model/plan.h"

// 甘特图视图:封装布局、视图状态、选择/移动状态与全部绘制逻辑
// 数据通过构造注入(Plans 引用),视图不拥有数据
// 交互模型:左键单击选择单个任务;无选择时左键横向拖动选择同行多个任务;
// 在选中任务上右键拖动,水平改变日期/垂直改变任务行;空白处左键或任意右键单击取消选择
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

	// ---- 选择状态(单行 WorkSubGroup 内的一个或多个 DayWork) ----
	bool HasSelection = false;   // 当前是否有选中的任务
	int SelPlanIdx = -1, SelGroupIdx = -1, SelSubGroupIdx = -1; // 选择集合所在位置: Plans[i].WorkGroups[j].WorkSubGroups[k]
	std::vector<int> SelWorks;   // 选中的 DayWorks 索引

	// ---- 左键横向拖选 ----
	bool RangeSelecting = false; // 正在左键横向拖动扩展选择
	time_t RangeAnchorDate = 0;  // 锚点(按下时起始任务)的日期
	float RangeAnchorX = 0.0f;   // 按下左键时的鼠标 X

	// ---- 右键拖动(移动选中集合) ----
	bool Moving = false;         // 正在右键拖动选中集合
	int MoveAnchorWorkIdx = -1;  // 被抓住的 DayWork 索引(提示标注的锚点)
	float MoveAnchorX = 0.0f;    // 按下右键时的鼠标 X
	float MoveAnchorY = 0.0f;    // 按下右键时的鼠标 Y
	int MoveOffsetDays = 0;      // 当前拖动的水平偏移(天)
	int MoveOffsetRows = 0;      // 当前拖动的垂直偏移(行)
	int MovePreviewRow = -1;     // 吸附后的预览行(松手后落入的任务行)
	bool RightPressHadSelection = false; // 右键按下时是否已有选择(决定松开时弹菜单还是本次点击已取消选择)

	// ---- 渲染 ----
	void DrawCanvas();                     // 画布、输入捕获、中键平移、右键菜单、网格线
	void DrawDateHeader(time_t basetime);  // 顶部日期行(今天标红)
	void DrawPlanColumn();                 // 计划名列(跨行标签)
	void DrawGroupColumn();                // 任务组列(跨行标签)
	void DrawSubGroupColumn();             // 子任务列(单行标签)
	void DrawDayWorks(time_t basetime);    // DayWork 单元格、选择高亮、点击/拖动启动与移动预览

	// ---- 行模型 ----
	void RebuildRows();                    // 每帧重建扁平行表
	int RowOf(int plan, int group, int subgroup) const;
	int NearestSubgroupRow(int want) const;// 就近吸附到实际 WorkSubGroup 行(跳过占位行)

	// ---- 选择与移动 ----
	void UpdateInteraction();              // 状态机:拖选扩展/拖动移动/提交/取消
	void UpdateRangeSelect();              // 左键拖动中:按锚点与当前鼠标扩展同行日期区间选择
	void UpdateMove();                     // 右键拖动中:更新偏移;松手提交移动,原地松手则取消选择
	void CommitMove();                     // 提交选中集合的日期与所属任务修改,选择跟随到新位置
	void ClearSelection();                 // 清空选择(连同拖选状态)
	void SelectSingle(int plan, int group, int subgroup, int work);
	void SelectDateRange(time_t d0, time_t d1); // 选择当前行内日期落在 [d0,d1] 的所有任务
	bool InSelection(int plan, int group, int subgroup, int work) const;
	void SelectionDateRange(time_t& mn, time_t& mx) const; // 选中集合的最早/最晚日期

	static time_t TodayMidnight();         // 今天 0 点(时间轴基准)
};
