#pragma once
#include <time.h>
#include <vector>
#include "../ImGuiScaffoldSDL3GL/imgui/imgui.h"
#include "../Model/plan.h"

// 甘特图视图:封装布局、视图状态、选择/拖动状态与全部绘制逻辑
// 数据通过构造注入(Plans 引用),视图不拥有数据
// 交互模型:左键在任务上按下并释放选中单个;按住未选任务横向拖动实时高亮区间,释放后选中;
// 双击合并显示块选择块内全部任务;按住已选任务左键拖动改变水平/垂直位置;
// 右键已选任务弹菜单(可删除任务,删除需确认),空白处左键或右键其他位置取消选择;中键拖动平移
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

	// ---- 左键按住拖动(按在已选任务上=移动,按在未选任务上=横向区间选择) ----
	bool LeftDraging = false;    // 左键在任务上按下未释放
	bool DragFromSelected = false;      // 按下时任务是否已选中(决定拖动语义)
	int DragPlanIdx = -1, DragGroupIdx = -1, DragSubGroupIdx = -1, DragWorkIdx = -1; // 按下的任务
	float DragAnchorX = 0.0f;    // 按下左键时的鼠标 X
	float DragAnchorY = 0.0f;    // 按下左键时的鼠标 Y
	time_t DragAnchorDate = 0;   // 按下任务的日期(区间选择锚点)
	int DragOffsetDays = 0;      // 移动模式:当前水平偏移(天)
	int DragOffsetRows = 0;      // 移动模式:当前垂直偏移(行)
	int DragPreviewRow = -1;     // 移动模式:吸附后的预览行(松手后落入的任务行)
	std::vector<int> PendingWorks;       // 区间模式:实时高亮的 DayWorks 索引

	bool RightPressHadSelection = false; // 右键按下时是否已有选择(决定松开时弹菜单还是本次点击已取消选择)
	bool PendingDeleteConfirm = false;   // 右键菜单选择删除后,下一帧弹确认对话框(避免在菜单弹窗内嵌套打开)

	// ---- 渲染 ----
	void DrawCanvas();                     // 画布、输入捕获、中键平移、右键菜单、网格线
	void DrawDateHeader(time_t basetime);  // 顶部日期行(今天标红)
	void DrawPlanColumn();                 // 计划名列(跨行标签)
	void DrawGroupColumn();                // 任务组列(跨行标签)
	void DrawSubGroupColumn();             // 子任务列(单行标签)
	void DrawDayWorks(time_t basetime);    // DayWork 单元格(同人水平相邻合并显示,仅显示效果)、选择/区间高亮、按下启动与移动预览

	// ---- 行模型 ----
	void RebuildRows();                    // 每帧重建扁平行表
	int RowOf(int plan, int group, int subgroup) const;
	int NearestSubgroupRow(int want) const;// 就近吸附到实际 WorkSubGroup 行(跳过占位行)

	// ---- 选择与拖动 ----
	void UpdateInteraction();              // 状态机入口
	void UpdateLeftDrag();                 // 按住左键期间:移动偏移或区间高亮;释放时提交(原地释放=单选)
	void CommitMove();                     // 提交选中集合的日期与所属任务修改,选择跟随到新位置
	void DeleteSelected();                 // 删除选中集合中的任务并清空选择
	void ClearSelection();                 // 清空选择
	void SelectSingle(int plan, int group, int subgroup, int work);
	void BuildPendingRange(time_t d0, time_t d1); // 重算区间高亮:按下行内日期落在 [d0,d1] 的所有任务
	bool InSelection(int plan, int group, int subgroup, int work) const;
	void SelectionDateRange(time_t& mn, time_t& mx) const; // 选中集合的最早/最晚日期

	static time_t TodayMidnight();         // 今天 0 点(时间轴基准)
};
