#define IMGUI_DEFINE_MATH_OPERATORS
#include "gantt_view.h"
#include "../ImGuiScaffoldSDL3GL/imgui/imgui_internal.h"
#include <algorithm>

GanttView::GanttView(std::vector<Task::Plan>& plans) : Plans(plans) {}

void GanttView::Draw()
{
	RebuildRows();
	time_t basetime = TodayMidnight();
	UpdateInteraction();
	DrawCanvas();
	DrawDateHeader(basetime);
	DrawPlanColumn();
	DrawGroupColumn();
	DrawSubGroupColumn();
	DrawDayWorks(basetime);
}

// ==================== 行模型 ====================

// 甘特图任务行 = Plan/WorkGroup/WorkSubGroup 的扁平化;空 Plan 或空 WorkGroup 各占一个占位行
void GanttView::RebuildRows()
{
	Rows.clear();
	for (int i = 0; i < (int)Plans.size(); ++i)
	{
		if (Plans[i].WorkGroups.size() == 0)
		{
			Rows.push_back({ i, -1, -1 });
			continue;
		}
		for (int j = 0; j < (int)Plans[i].WorkGroups.size(); ++j)
		{
			if (Plans[i].WorkGroups[j].WorkSubGroups.size() == 0)
			{
				Rows.push_back({ i, j, -1 });
				continue;
			}
			for (int k = 0; k < (int)Plans[i].WorkGroups[j].WorkSubGroups.size(); ++k)
				Rows.push_back({ i, j, k });
		}
	}
}

int GanttView::RowOf(int plan, int group, int subgroup) const
{
	for (int r = 0; r < (int)Rows.size(); ++r)
		if (Rows[r].Plan == plan && Rows[r].Group == group && Rows[r].SubGroup == subgroup)
			return r;
	return -1;
}

// 从 want 行出发向两侧搜索最近的"实际 WorkSubGroup 行"(跳过空 Plan/空 WorkGroup 的占位行)
int GanttView::NearestSubgroupRow(int want) const
{
	int total = (int)Rows.size();
	if (total <= 0) return -1;
	if (want < 0) want = 0;
	if (want > total - 1) want = total - 1;
	if (Rows[want].SubGroup >= 0) return want;
	for (int d = 1; d < total; ++d)
	{
		if (want - d >= 0 && Rows[want - d].SubGroup >= 0) return want - d;
		if (want + d < total && Rows[want + d].SubGroup >= 0) return want + d;
	}
	return -1;
}

time_t GanttView::TodayMidnight()
{
	time_t now = time(NULL);
	struct tm* tm_info = localtime(&now);
	tm_info->tm_hour = 0;
	tm_info->tm_min = 0;
	tm_info->tm_sec = 0;
	return mktime(tm_info);
}

// ==================== 画布 ====================

void GanttView::DrawCanvas()
{
	ImGuiIO& io = ImGui::GetIO();
	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	CanvasCursorPos = ImGui::GetCursorPos();
	CanvasP0 = ImGui::GetCursorScreenPos();          // ImDrawList API 使用屏幕坐标
	ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
	if (canvas_sz.x < 50.0f) canvas_sz.x = 50.0f;
	if (canvas_sz.y < 50.0f) canvas_sz.y = 50.0f;
	CanvasP1 = ImVec2(CanvasP0.x + canvas_sz.x, CanvasP0.y + canvas_sz.y);

	// 背景与边框
	draw_list->AddRectFilled(CanvasP0, CanvasP1, IM_COL32(50, 50, 50, 255));
	draw_list->AddRect(CanvasP0, CanvasP1, IM_COL32(255, 255, 255, 255));

	// 交互捕获:左键选择/拖动任务,右键单击取消选择,中键拖拽平移
	ImGui::InvisibleButton("canvas", canvas_sz, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
	const bool is_active = ImGui::IsItemActive();

	// 记录右键按下时是否已有选择:决定松开时弹菜单还是本次点击已取消选择
	if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
		RightPressHadSelection = HasSelection;

	// 垂直滚动范围:内容(表头 + 全部行)不高于画布时不可滚动
	const float content_h = GridV * ((int)Rows.size() + 1);
	const float min_scroll_y = fminf(0.0f, canvas_sz.y - content_h);

	// 鼠标滚轮:垂直滚动(一次三行)
	if (ImGui::IsItemHovered() && !LeftDraging)
		ScrollingReal.y += io.MouseWheel * GridV * 3.0f;

	// 中键拖拽:平移
	if (is_active && ImGui::IsMouseDragging(ImGuiMouseButton_Middle, -1.0f))
	{
		ScrollingReal.x += io.MouseDelta.x;
		ScrollingReal.y += io.MouseDelta.y;
	}
	// 垂直钳制(顶部不超过第一行,底部不超过最后一行)后按网格吸附
	if (ScrollingReal.y > 0.0f) ScrollingReal.y = 0.0f;
	if (ScrollingReal.y < min_scroll_y) ScrollingReal.y = min_scroll_y;
	Scrolling.x = roundf(ScrollingReal.x / GridH) * GridH;
	Scrolling.y = roundf(ScrollingReal.y / GridV) * GridV;
	if (Scrolling.y < min_scroll_y) Scrolling.y = ceilf(min_scroll_y / GridV) * GridV;

	// 右键点击(无拖拽位移)且此前无选择时弹上下文菜单(此时仅"回到今天");
	// 有选择时的右键由 DrawDayWorks 处理:命中已选任务弹菜单(含删除),否则取消选择
	ImVec2 drag_delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
	if (drag_delta.x == 0.0f && drag_delta.y == 0.0f && !RightPressHadSelection)
		ImGui::OpenPopupOnItemClick("context", ImGuiPopupFlags_MouseButtonRight);
	if (ImGui::BeginPopup("context"))
	{
		// 有选择时菜单提供删除(弹出时选择无法变化,内容按 HasSelection 分支即可)
		if (HasSelection)
		{
			char del_label[48];
			if (SelWorks.size() > 1)
				sprintf_s(del_label, sizeof(del_label), "Delete Tasks (%d)", (int)SelWorks.size());
			else
				sprintf_s(del_label, sizeof(del_label), "Delete Task");
			if (ImGui::MenuItem(del_label))
			{
				PendingDeleteConfirm = true;   // 下一帧在菜单外弹确认框
				ImGui::CloseCurrentPopup();
			}
		}
		if (ImGui::MenuItem("Back to Today"))
		{
			Scrolling.x = 0;
			Scrolling.y = 0;
		}
		ImGui::EndPopup();
	}

	// 删除确认对话框:确认后删除选中集合的任务
	if (PendingDeleteConfirm)
	{
		ImGui::OpenPopup("Confirm Delete");
		PendingDeleteConfirm = false;
	}
	if (ImGui::BeginPopupModal("Confirm Delete", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Delete %d selected task(s)?", (int)SelWorks.size());
		ImGui::Separator();
		if (ImGui::Button("Delete", ImVec2(110, 0)))
		{
			DeleteSelected();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(110, 0)))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}

	// 网格线:标签列边界三条加亮竖线 + 日期列竖线 + 行横线
	draw_list->PushClipRect(CanvasP0, CanvasP1, true);
	for (float x = fmodf(Scrolling.x, GridH); x < canvas_sz.x; x += GridH)
	{
		if (x < GridH * PlanNameCells)
			draw_list->AddLineV(CanvasP0.x + GridH * PlanNameCells, CanvasP0.y, CanvasP1.y, IM_COL32(200, 200, 200, 40));
		else if (x < GridH * (PlanNameCells + TaskGroupCells))
			draw_list->AddLineV(CanvasP0.x + GridH * (PlanNameCells + TaskGroupCells), CanvasP0.y, CanvasP1.y, IM_COL32(200, 200, 200, 40));
		else if (x < GridH * LabelCells())
			draw_list->AddLineV(CanvasP0.x + GridH * LabelCells(), CanvasP0.y, CanvasP1.y, IM_COL32(200, 200, 200, 40));
		else
			draw_list->AddLineV(CanvasP0.x + x, CanvasP0.y, CanvasP1.y, IM_COL32(200, 200, 200, 40));
	}
	for (float y = fmodf(Scrolling.y, GridV); y < canvas_sz.y; y += GridV)
	{
		if (y < GridV)
			draw_list->AddLineH(CanvasP0.x, CanvasP1.x, CanvasP0.y + GridV, IM_COL32(200, 200, 200, 80));
		else
			draw_list->AddLineH(CanvasP0.x, CanvasP1.x, CanvasP0.y + y, IM_COL32(200, 200, 200, 40));
	}
	draw_list->PopClipRect();

	ImGui::SetCursorPos(CanvasCursorPos);
}

// ==================== 日期表头 ====================

void GanttView::DrawDateHeader(time_t basetime)
{
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ImGui::PushClipRect(ImVec2(CanvasP0.x + GridH * LabelCells(), CanvasP0.y), CanvasP1, false);

	float fration = fmodf(Scrolling.x, GridH);
	int startN = (int)floor(-Scrolling.x / GridH);
	int days = (int)ceil((CanvasP1.x - CanvasP0.x) / GridH) + (fration != 0 ? 1 : 0) - LabelCells();

	struct tm date_info = *localtime(&basetime);
	date_info.tm_mday += startN - (int)ceil((float)days / 2);
	for (int i = 0; i < days; ++i)
	{
		date_info.tm_mday++;
		time_t cell_time = mktime(&date_info);
		struct tm* cell_tm = localtime(&cell_time);
		char buffer[5];
		sprintf_s(buffer, 5, "%d\0", cell_tm->tm_mday);
		auto textsize = ImGui::CalcTextSize(buffer);
		ImVec2 textPos;
		textPos.x = CanvasP0.x + fration + GridH * (i + LabelCells()) + (GridH - textsize.x) * 0.5f;
		textPos.y = CanvasP0.y + (GridV - textsize.y) * 0.5f;
		// 屏幕坐标直接绘制(与网格线同坐标系,不污染窗口布局)
		draw_list->AddText(textPos, cell_time == basetime ? IM_COL32(255, 0, 0, 255) : IM_COL32(255, 255, 255, 255), buffer);
	}
	ImGui::PopClipRect();
}

// ==================== 标签列 ====================

// 悬停行判定(绝对 Y 坐标,含画布原点与垂直平移)
static bool rowHovered(const ImVec2& mouse, const ImVec2& canvas_p0, float grid_v, float scrolling_y, int row, int count)
{
	float top = canvas_p0.y + grid_v * (1 + row) + scrolling_y;
	return mouse.y > top && mouse.y < top + grid_v * count;
}

void GanttView::DrawPlanColumn()
{
	ImGuiIO& io = ImGui::GetIO();
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ImGui::PushClipRect(ImVec2(CanvasP0.x, CanvasP0.y + GridV), CanvasP1, false);
	// 沿行模型聚合每个 Plan 的连续行跨度
	int row = 0;
	while (row < (int)Rows.size())
	{
		int plan = Rows[row].Plan;
		int span_end = row + 1;
		while (span_end < (int)Rows.size() && Rows[span_end].Plan == plan)
			++span_end;
		int count = span_end - row;

		auto textsize = ImGui::CalcTextSize(Plans[plan].Name.c_str());
		ImVec2 textPos;
		textPos.x = CanvasP0.x + (GridH * PlanNameCells - textsize.x) * 0.5f;
		textPos.y = CanvasP0.y + GridV * (1 + row) + Scrolling.y + (GridV - textsize.y) * 0.5f;
		// 屏幕坐标直接绘制(与格子同坐标系,不污染窗口布局)
		draw_list->AddText(textPos,
			rowHovered(io.MousePos, CanvasP0, GridV, Scrolling.y, row, count) ? IM_COL32(255, 255, 0, 255) : IM_COL32(255, 255, 255, 255),
			Plans[plan].Name.c_str());
		row = span_end;
	}
	ImGui::PopClipRect();
}

void GanttView::DrawGroupColumn()
{
	ImGuiIO& io = ImGui::GetIO();
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ImGui::PushClipRect(ImVec2(CanvasP0.x + GridH * PlanNameCells, CanvasP0.y + GridV), CanvasP1, false);
	// 沿行模型聚合每个 WorkGroup 的连续行跨度
	int row = 0;
	while (row < (int)Rows.size())
	{
		int plan = Rows[row].Plan;
		int group = Rows[row].Group;
		int span_end = row + 1;
		while (span_end < (int)Rows.size() && Rows[span_end].Plan == plan && Rows[span_end].Group == group)
			++span_end;
		int count = span_end - row;

		if (group < 0)   // 空 Plan 占位行:该列不显示
		{
			row = span_end;
			continue;
		}

		auto textsize = ImGui::CalcTextSize(Plans[plan].WorkGroups[group].Name.c_str());
		ImVec2 textPos;
		textPos.x = CanvasP0.x + GridH * PlanNameCells + (TaskGroupCells * GridH - textsize.x) * 0.5f;
		textPos.y = CanvasP0.y + GridV * (1 + row) + Scrolling.y + (GridV - textsize.y) * 0.5f;
		draw_list->AddText(textPos,
			rowHovered(io.MousePos, CanvasP0, GridV, Scrolling.y, row, count) ? IM_COL32(255, 255, 0, 255) : IM_COL32(255, 255, 255, 255),
			Plans[plan].WorkGroups[group].Name.c_str());
		row = span_end;
	}
	ImGui::PopClipRect();
}

void GanttView::DrawSubGroupColumn()
{
	ImGuiIO& io = ImGui::GetIO();
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ImGui::PushClipRect(ImVec2(CanvasP0.x + GridH * (PlanNameCells + TaskGroupCells), CanvasP0.y + GridV), CanvasP1, false);
	for (int row = 0; row < (int)Rows.size(); ++row)
	{
		const RowRef& r = Rows[row];
		if (r.SubGroup < 0)   // 占位行:该列不显示
			continue;
		auto& subgroup = Plans[r.Plan].WorkGroups[r.Group].WorkSubGroups[r.SubGroup];
		auto textsize = ImGui::CalcTextSize(subgroup.Name.c_str());
		ImVec2 textPos;
		textPos.x = CanvasP0.x + GridH * (PlanNameCells + TaskGroupCells) + (GridH - textsize.x) * 0.5f;
		textPos.y = CanvasP0.y + GridV * (1 + row) + Scrolling.y + (GridV - textsize.y) * 0.5f;
		draw_list->AddText(textPos,
			rowHovered(io.MousePos, CanvasP0, GridV, Scrolling.y, row, 1) ? IM_COL32(255, 255, 0, 255) : IM_COL32(255, 255, 255, 255),
			subgroup.Name.c_str());
	}
	ImGui::PopClipRect();
}

// ==================== DayWork 网格 ====================

// 显示层合并:同一行内水平相邻(列连号)且同人的单元格合成一个块绘制,人名在块中央只画一次;
// 仅是显示效果,每个 DayWork 仍是独立单元格(命中测试/选择/拖动逻辑不变),
// 块内被选中的单元格单独以琥珀底 + 黄框高亮(连续高亮段共用一个框)
void GanttView::DrawDayWorks(time_t basetime)
{
	ImGuiIO& io = ImGui::GetIO();
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	float canvas_width = CanvasP1.x - CanvasP0.x - GridH * LabelCells();
	float half_canvas_width = ceil(canvas_width / 2 / GridH - 1) * GridH;
	// 显示列 → 屏幕横坐标(含移动预览偏移的列号)
	auto CellX = [&](int day) { return CanvasP0.x + Scrolling.x + GridH * (day + LabelCells()) + half_canvas_width; };

	const bool move_mode = LeftDraging && DragFromSelected;
	const int sel_source_row = HasSelection ? RowOf(SelPlanIdx, SelGroupIdx, SelSubGroupIdx) : -1;
	time_t sel_min_date = 0, sel_max_date = 0;
	if (HasSelection)
		SelectionDateRange(sel_min_date, sel_max_date);
	bool left_press_hit_task = false;   // 本次左键按下是否命中了任务(未命中则按下空白)
	bool right_press_hit_selected = false; // 本次右键按下是否命中了已选任务(命中则弹菜单而非取消选择)

	// 单元格显示信息(按显示位置参与合并分组)
	struct CellVis
	{
		int l = 0;               // DayWorks 索引(逻辑单元格)
		int day = 0;             // 显示列(含移动预览的水平偏移)
		int row = 0;             // 显示行(含移动预览的垂直吸附)
		bool selected = false;   // 选中集合成员
		bool pending = false;    // 区间选择模式:被实时高亮的成员
	};
	std::vector<CellVis> cells;

	ImGui::PushClipRect(ImVec2(CanvasP0.x + GridH * LabelCells(), CanvasP0.y + GridV), CanvasP1, false);
	for (int row = 0; row < (int)Rows.size(); ++row)
	{
		const RowRef& r = Rows[row];
		if (r.SubGroup < 0)
			continue;
		auto& dayworks = Plans[r.Plan].WorkGroups[r.Group].WorkSubGroups[r.SubGroup].DayWorks;

		// 收集本行全部单元格的显示位置与状态(跨行移动后 DayWorks 未必按日期有序,先排序再分组)
		cells.clear();
		for (int l = 0; l < (int)dayworks.size(); ++l)
		{
			const bool selected = InSelection(r.Plan, r.Group, r.SubGroup, l);
			const int move_days = (selected && move_mode) ? DragOffsetDays : 0;
			const int cell_row = (selected && move_mode && DragPreviewRow >= 0) ? DragPreviewRow : row;
			double diff_seconds = difftime(dayworks[l].Date, basetime);
			CellVis c;
			c.l = l;
			c.day = (int)(diff_seconds / (60 * 60 * 24)) + move_days;
			c.row = cell_row;
			c.selected = selected;
			c.pending = LeftDraging && !DragFromSelected
				&& r.Plan == DragPlanIdx && r.Group == DragGroupIdx && r.SubGroup == DragSubGroupIdx
				&& std::find(PendingWorks.begin(), PendingWorks.end(), l) != PendingWorks.end();
			cells.push_back(c);
		}
		std::stable_sort(cells.begin(), cells.end(), [](const CellVis& a, const CellVis& b) { return a.day < b.day; });

		// 沿显示位置聚合"同显示行 + 同人 + 列连号"的连续段,每段合并为一个块绘制
		for (size_t i0 = 0; i0 < cells.size(); )
		{
			size_t i1 = i0 + 1;
			while (i1 < cells.size()
				&& cells[i1].row == cells[i0].row
				&& dayworks[cells[i1].l].Person == dayworks[cells[i1 - 1].l].Person
				&& cells[i1].day == cells[i1 - 1].day + 1)
				++i1;

			const float cell_y = GridV * 1 + GridV * cells[i0].row + CanvasP0.y + Scrolling.y;
			const float y0 = cell_y + 3;
			const float y1 = cell_y + GridV - 3;
			const float block_x0 = CellX(cells[i0].day) + 3;             // 块左边界(含左内边距)
			const float block_x1 = CellX(cells[i1 - 1].day) + GridH - 2; // 块右边界(含右内边距)
			auto BlockEdgeL = [&](size_t k) { return CellX(cells[k].day) + (k == i0 ? 3 : 0); };       // 高亮子段左缘(块首留内边距)
			auto BlockEdgeR = [&](size_t k) { return CellX(cells[k].day) + GridH - (k == i1 - 1 ? 2 : 0); }; // 高亮子段右缘(块尾留内边距)

			// 逐单元格命中测试与左键按下捕获(逻辑与未合并时完全一致)
			bool run_hover_plain = false;   // 悬停在块内未高亮的单元格上
			for (size_t i = i0; i < i1; ++i)
			{
				const CellVis& c = cells[i];
				const float cx = CellX(c.day);
				const bool cell_hovered = io.MousePos.x >= cx && io.MousePos.x < cx + GridH
					&& io.MousePos.y >= cell_y && io.MousePos.y < cell_y + GridV;
				// 左键按下命中任务:双击合并块选择块内全部任务;
				// 单击时按在已选任务上为"移动"拖动,按在未选任务上为"横向区间选择"拖动
				if (!LeftDraging && cell_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				{
					if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
					{
						// 双击:选择该显示块内全部单元格(仅显示合并,每个任务仍是独立单元格)
						HasSelection = true;
						SelPlanIdx = r.Plan; SelGroupIdx = r.Group; SelSubGroupIdx = r.SubGroup;
						SelWorks.clear();
						for (size_t k = i0; k < i1; ++k)
							SelWorks.push_back(cells[k].l);
					}
					else
					{
						LeftDraging = true;
						DragFromSelected = c.selected;
						DragPlanIdx = r.Plan; DragGroupIdx = r.Group; DragSubGroupIdx = r.SubGroup; DragWorkIdx = c.l;
						DragAnchorX = io.MousePos.x;
						DragAnchorY = io.MousePos.y;
						DragAnchorDate = dayworks[c.l].Date;
						DragOffsetDays = 0;
						DragOffsetRows = 0;
						DragPreviewRow = DragFromSelected ? row : -1;
						PendingWorks.assign(1, c.l);
					}
					left_press_hit_task = true;
				}
				// 右键命中已选任务:不取消选择,改为弹上下文菜单(含删除选项)
				if (!LeftDraging && cell_hovered && c.selected && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
					right_press_hit_selected = true;
				if (cell_hovered && !LeftDraging && !(c.selected || c.pending))
					run_hover_plain = true;
			}

			// 块内存在未高亮单元格时先铺灰底(高亮部分随后覆盖)
			bool any_plain = false;
			for (size_t i = i0; i < i1; ++i)
				if (!(cells[i].selected || cells[i].pending)) { any_plain = true; break; }
			if (any_plain)
				draw_list->AddRectFilled(ImVec2(block_x0, y0), ImVec2(block_x1, y1), IM_COL32(100, 100, 100, 255), 0);

			// 移动模式下被拖走的成员:原位置画虚影(记录原始行与原始日期)
			if (move_mode && (DragOffsetDays != 0 || DragOffsetRows != 0))
			{
				for (size_t i = i0; i < i1; ++i)
				{
					const CellVis& c = cells[i];
					if (!c.selected)
						continue;
					float ghost_x = CellX(c.day - DragOffsetDays) + 3;
					float ghost_y = GridV * 1 + GridV * row + CanvasP0.y + Scrolling.y + 3;
					draw_list->AddRect(ImVec2(ghost_x, ghost_y), ImVec2(ghost_x + GridH - 8, ghost_y + GridV - 5), IM_COL32(255, 255, 0, 90));
				}
			}

			// 高亮连续子段(选中/区间拖选):琥珀底 + 黄框(含被抓住单元格的子段边框更亮)
			for (size_t i = i0; i < i1; )
			{
				if (!(cells[i].selected || cells[i].pending)) { ++i; continue; }
				size_t j = i + 1;
				while (j < i1 && (cells[j].selected || cells[j].pending) && cells[j].day == cells[j - 1].day + 1)
					++j;
				bool anchor_here = false;
				for (size_t k = i; k < j; ++k)
					if ((move_mode ? cells[k].selected : cells[k].pending) && cells[k].l == DragWorkIdx)
						anchor_here = true;
				draw_list->AddRectFilled(ImVec2(BlockEdgeL(i), y0), ImVec2(BlockEdgeR(j - 1), y1), IM_COL32(130, 100, 20, 255), 0);
				draw_list->AddRect(ImVec2(BlockEdgeL(i), y0), ImVec2(BlockEdgeR(j - 1), y1), IM_COL32(255, 255, 0, anchor_here ? 255 : 160), 0);
				i = j;
			}

			// 悬停在块内未高亮单元格上:整块亮边框提示可点击选择
			if (run_hover_plain)
			{
				draw_list->AddRect(ImVec2(block_x0, y0), ImVec2(block_x1, y1), IM_COL32(230, 230, 230, 220), 0);
				ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
			}

			// 移动模式:被抓住的单元格附加提示(目标行提示带、目标行名、目标日期)
			if (move_mode)
			{
				for (size_t i = i0; i < i1; ++i)
				{
					const CellVis& c = cells[i];
					if (!c.selected || c.l != DragWorkIdx)
						continue;
					// 目标行提示带
					if (DragPreviewRow >= 0 && DragPreviewRow != sel_source_row)
						draw_list->AddRectFilled(ImVec2(CanvasP0.x + GridH * LabelCells(), cell_y), ImVec2(CanvasP1.x, cell_y + GridV), IM_COL32(255, 255, 0, 25));
					ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
					// 垂直拖动时,在预览单元格右侧显示目标任务名
					if (DragPreviewRow >= 0 && DragPreviewRow != sel_source_row)
					{
						const RowRef& target = Rows[DragPreviewRow];
						draw_list->AddText(ImVec2(BlockEdgeR(i) + 6, cell_y + (GridV - ImGui::GetTextLineHeight()) * 0.5f), IM_COL32(255, 255, 0, 220),
							Plans[target.Plan].WorkGroups[target.Group].WorkSubGroups[target.SubGroup].Name.c_str());
					}
					// 水平拖动时,在集合最左预览单元格上方显示目标日期(多选时为区间)
					if (DragOffsetDays != 0)
					{
						char date_buf[64];
						time_t nd0 = sel_min_date + (time_t)DragOffsetDays * 86400;
						struct tm tm0 = *localtime(&nd0);
						if (SelWorks.size() > 1)
						{
							time_t nd1 = sel_max_date + (time_t)DragOffsetDays * 86400;
							struct tm tm1 = *localtime(&nd1);
							sprintf_s(date_buf, sizeof(date_buf), "%d-%02d-%02d ~ %d-%02d-%02d",
								tm0.tm_year + 1900, tm0.tm_mon + 1, tm0.tm_mday,
								tm1.tm_year + 1900, tm1.tm_mon + 1, tm1.tm_mday);
						}
						else
						{
							sprintf_s(date_buf, sizeof(date_buf), "%d-%02d-%02d", tm0.tm_year + 1900, tm0.tm_mon + 1, tm0.tm_mday);
						}
						float label_x = CanvasP0.x + Scrolling.x + GridH * ((int)(difftime(sel_min_date, basetime) / (60 * 60 * 24)) + DragOffsetDays + LabelCells()) + half_canvas_width + 3;
						float text_y = y0 - 16.0f;
						if (text_y < CanvasP0.y + GridV + 2.0f) text_y = CanvasP0.y + GridV + 2.0f;
						draw_list->AddText(ImVec2(label_x, text_y), IM_COL32(255, 255, 0, 255), date_buf);
					}
				}
			}

			// 人名合并:整块中央只画一次
			const std::string& person = dayworks[cells[i0].l].Person;
			auto textsize = ImGui::CalcTextSize(person.c_str());
			ImVec2 textPos;
			textPos.x = (block_x0 + block_x1 - textsize.x) * 0.5f;
			textPos.y = cell_y + (GridV - textsize.y) * 0.5f;
			// 屏幕坐标直接绘制(与格子同坐标系,不污染窗口布局)
			draw_list->AddText(textPos, IM_COL32(255, 255, 255, 255), person.c_str());

			i0 = i1;
		}
	}
	ImGui::PopClipRect();

	// 画布内左键按下但未命中任何任务:点击空白,取消选择
	const bool mouse_in_canvas = io.MousePos.x >= CanvasP0.x && io.MousePos.x < CanvasP1.x
		&& io.MousePos.y >= CanvasP0.y && io.MousePos.y < CanvasP1.y;
	if (mouse_in_canvas && !left_press_hit_task && !LeftDraging && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		ClearSelection();
	// 右键单击(画布内):命中已选任务弹上下文菜单(含删除),其他位置取消选择
	if (mouse_in_canvas && !LeftDraging && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && HasSelection)
	{
		if (right_press_hit_selected)
			ImGui::OpenPopup("context");
		else
			ClearSelection();
	}
}

// ==================== 选择与拖动 ====================

// 每帧驱动左键拖动状态机
void GanttView::UpdateInteraction()
{
	if (LeftDraging)
		UpdateLeftDrag();
}

// 左键在任务上按住期间:
// 按在已选任务上 = 移动拖动(实时偏移 + 目标行吸附预览);
// 按在未选任务上 = 横向区间选择(实时高亮锚点日期 ± 水平偏移覆盖的任务);
// 释放时提交:移动生效 / 高亮集合成为选择;原地释放则选中按下的单个任务
void GanttView::UpdateLeftDrag()
{
	ImGuiIO& io = ImGui::GetIO();
	if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) && !ImGui::IsMouseReleased(ImGuiMouseButton_Left))
	{
		LeftDraging = false;   // 鼠标状态丢失(如窗口失焦),放弃本次拖动
		return;
	}
	if (DragFromSelected)
	{
		// 移动模式:按网格大小吸附为整天/整行偏移,行号吸附到最近的有效任务行
		DragOffsetDays = (int)roundf((io.MousePos.x - DragAnchorX) / GridH);
		DragOffsetRows = (int)roundf((io.MousePos.y - DragAnchorY) / GridV);
		DragPreviewRow = NearestSubgroupRow(RowOf(SelPlanIdx, SelGroupIdx, SelSubGroupIdx) + DragOffsetRows);
		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		{
			int source_row = RowOf(SelPlanIdx, SelGroupIdx, SelSubGroupIdx);
			if (DragOffsetDays == 0 && DragPreviewRow == source_row)
				SelectSingle(DragPlanIdx, DragGroupIdx, DragSubGroupIdx, DragWorkIdx);   // 原地释放 = 单选该任务
			else
				CommitMove();
			LeftDraging = false;
		}
	}
	else
	{
		// 区间模式:实时重算高亮集合(锚点必在区间端点上,集合非空)
		int offset_days = (int)roundf((io.MousePos.x - DragAnchorX) / GridH);
		BuildPendingRange(DragAnchorDate, DragAnchorDate + (time_t)offset_days * 86400);
		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		{
			// 释放:高亮集合成为选择
			HasSelection = true;
			SelPlanIdx = DragPlanIdx;
			SelGroupIdx = DragGroupIdx;
			SelSubGroupIdx = DragSubGroupIdx;
			SelWorks = PendingWorks;
			LeftDraging = false;
		}
	}
}

// 提交移动:选中集合整体改日期;若目标行不同则整组移动到目标行的 WorkSubGroup(按日期升序追加),
// 选择跟随到新位置
void GanttView::CommitMove()
{
	auto& srcDayWorks = Plans[SelPlanIdx].WorkGroups[SelGroupIdx].WorkSubGroups[SelSubGroupIdx].DayWorks;
	if (DragOffsetDays != 0)
		for (int idx : SelWorks)
			srcDayWorks[idx].Date += (time_t)DragOffsetDays * 86400;
	RowRef target = (DragPreviewRow >= 0 && DragPreviewRow < (int)Rows.size()) ? Rows[DragPreviewRow] : RowRef();
	if (target.SubGroup < 0 || (target.Plan == SelPlanIdx && target.Group == SelGroupIdx && target.SubGroup == SelSubGroupIdx))
		return;   // 同行:仅日期生效,选择索引不变
	// 整组移动到目标行的 WorkSubGroup
	std::vector<Task::DayWork> moved;
	std::vector<Task::DayWork> kept;
	moved.reserve(SelWorks.size());
	kept.reserve(srcDayWorks.size());
	for (int idx = 0; idx < (int)srcDayWorks.size(); ++idx)
	{
		bool is_member = false;
		for (int m : SelWorks)
			if (m == idx) { is_member = true; break; }
		if (is_member)
			moved.push_back(std::move(srcDayWorks[idx]));
		else
			kept.push_back(std::move(srcDayWorks[idx]));
	}
	std::sort(moved.begin(), moved.end(), [](const Task::DayWork& a, const Task::DayWork& b) { return a.Date < b.Date; });
	srcDayWorks = std::move(kept);
	auto& dstDayWorks = Plans[target.Plan].WorkGroups[target.Group].WorkSubGroups[target.SubGroup].DayWorks;
	int base = (int)dstDayWorks.size();
	for (auto& dw : moved)
		dstDayWorks.push_back(std::move(dw));
	// 选择跟随:移动后位于目标行末尾的连续区段
	SelPlanIdx = target.Plan;
	SelGroupIdx = target.Group;
	SelSubGroupIdx = target.SubGroup;
	SelWorks.clear();
	for (int i = 0; i < (int)moved.size(); ++i)
		SelWorks.push_back(base + i);
}

// 删除选中集合中的任务(保留同行其余 DayWorks),随后清空选择
void GanttView::DeleteSelected()
{
	if (!HasSelection)
		return;
	auto& dayworks = Plans[SelPlanIdx].WorkGroups[SelGroupIdx].WorkSubGroups[SelSubGroupIdx].DayWorks;
	std::vector<Task::DayWork> kept;
	kept.reserve(dayworks.size());
	for (int idx = 0; idx < (int)dayworks.size(); ++idx)
	{
		bool is_member = false;
		for (int m : SelWorks)
			if (m == idx) { is_member = true; break; }
		if (!is_member)
			kept.push_back(std::move(dayworks[idx]));
	}
	dayworks = std::move(kept);
	ClearSelection();
}

void GanttView::ClearSelection()
{
	HasSelection = false;
	SelPlanIdx = SelGroupIdx = SelSubGroupIdx = -1;
	SelWorks.clear();
}

void GanttView::SelectSingle(int plan, int group, int subgroup, int work)
{
	HasSelection = true;
	SelPlanIdx = plan;
	SelGroupIdx = group;
	SelSubGroupIdx = subgroup;
	SelWorks.assign(1, work);
}

// 重算区间高亮:按下行内日期落在 [d0,d1] 的所有任务(区间端点由锚点日期与拖动偏移得出,必含锚点)
void GanttView::BuildPendingRange(time_t d0, time_t d1)
{
	if (d0 > d1)
	{
		time_t t = d0;
		d0 = d1;
		d1 = t;
	}
	auto& dayworks = Plans[DragPlanIdx].WorkGroups[DragGroupIdx].WorkSubGroups[DragSubGroupIdx].DayWorks;
	PendingWorks.clear();
	for (int l = 0; l < (int)dayworks.size(); ++l)
		if (dayworks[l].Date >= d0 && dayworks[l].Date <= d1)
			PendingWorks.push_back(l);
}

bool GanttView::InSelection(int plan, int group, int subgroup, int work) const
{
	if (!HasSelection || plan != SelPlanIdx || group != SelGroupIdx || subgroup != SelSubGroupIdx)
		return false;
	for (int w : SelWorks)
		if (w == work)
			return true;
	return false;
}

void GanttView::SelectionDateRange(time_t& mn, time_t& mx) const
{
	mn = mx = 0;
	if (!HasSelection || SelWorks.empty())
		return;
	auto& dayworks = Plans[SelPlanIdx].WorkGroups[SelGroupIdx].WorkSubGroups[SelSubGroupIdx].DayWorks;
	mn = mx = dayworks[SelWorks[0]].Date;
	for (int idx : SelWorks)
	{
		if (dayworks[idx].Date < mn) mn = dayworks[idx].Date;
		if (dayworks[idx].Date > mx) mx = dayworks[idx].Date;
	}
}
