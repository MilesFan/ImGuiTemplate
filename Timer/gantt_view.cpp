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

	// 交互捕获:中键拖拽平移,左键选择任务,右键拖拽移动选中任务/单击取消选择
	ImGui::InvisibleButton("canvas", canvas_sz, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
	const bool is_active = ImGui::IsItemActive();

	// 记录右键按下时是否已有选择:决定松开时弹菜单还是本次点击已用于取消选择
	if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
		RightPressHadSelection = HasSelection;

	// 中键拖拽:平移(按网格吸附,顶部不超过第一行)
	if (is_active && ImGui::IsMouseDragging(ImGuiMouseButton_Middle, -1.0f))
	{
		ScrollingReal.x += io.MouseDelta.x;
		ScrollingReal.y += io.MouseDelta.y;
		if (ScrollingReal.y > 0) ScrollingReal.y = 0;
		Scrolling.x = roundf(ScrollingReal.x / GridH) * GridH;
		Scrolling.y = roundf(ScrollingReal.y / GridV) * GridV;
	}

	// 右键点击(无拖拽位移)且此前无选择时弹上下文菜单;有选择时本次点击用于取消选择
	ImVec2 drag_delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
	if (drag_delta.x == 0.0f && drag_delta.y == 0.0f && !RightPressHadSelection)
		ImGui::OpenPopupOnItemClick("context", ImGuiPopupFlags_MouseButtonRight);
	if (ImGui::BeginPopup("context"))
	{
		if (ImGui::MenuItem("Back to Today"))
		{
			Scrolling.x = 0;
			Scrolling.y = 0;
		}
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
		textPos.y = CanvasCursorPos.y + (GridV - textsize.y) * 0.5f;
		ImGui::SetCursorPos(textPos);
		if (cell_time == basetime)
			ImGui::TextColored(ImVec4(1, 0, 0, 1), buffer);
		else
			ImGui::Text(buffer);
	}
	ImGui::PopClipRect();
}

// ==================== 标签列 ====================

// 悬停行判定(绝对 Y 坐标,含画布原点与垂直平移)
static bool rowHovered(const ImVec2& mouse, const ImVec2& canvas_p0, float grid_v, int row, int count)
{
	return mouse.y > canvas_p0.y + grid_v * (1 + row) && mouse.y < canvas_p0.y + grid_v * (1 + count + row);
}

void GanttView::DrawPlanColumn()
{
	ImGuiIO& io = ImGui::GetIO();
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
		textPos.y = GridV * 1 + GridV * row + CanvasP0.y + Scrolling.y + (GridV - textsize.y) * 0.5f;
		ImGui::SetCursorPos(textPos);
		if (rowHovered(io.MousePos, CanvasP0, GridV, row, count))
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), Plans[plan].Name.c_str());
		else
			ImGui::Text(Plans[plan].Name.c_str());
		row = span_end;
	}
	ImGui::PopClipRect();
}

void GanttView::DrawGroupColumn()
{
	ImGuiIO& io = ImGui::GetIO();
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
		textPos.y = GridV * 1 + GridV * row + CanvasP0.y + Scrolling.y + (GridV - textsize.y) * 0.5f;
		ImGui::SetCursorPos(textPos);
		if (rowHovered(io.MousePos, CanvasP0, GridV, row, count))
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), Plans[plan].WorkGroups[group].Name.c_str());
		else
			ImGui::Text(Plans[plan].WorkGroups[group].Name.c_str());
		row = span_end;
	}
	ImGui::PopClipRect();
}

void GanttView::DrawSubGroupColumn()
{
	ImGuiIO& io = ImGui::GetIO();
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
		textPos.y = GridV * 1 + GridV * row + CanvasP0.y + Scrolling.y + (GridV - textsize.y) * 0.5f;
		ImGui::SetCursorPos(textPos);
		if (rowHovered(io.MousePos, CanvasP0, GridV, row, 1))
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), subgroup.Name.c_str());
		else
			ImGui::Text(subgroup.Name.c_str());
	}
	ImGui::PopClipRect();
}

// ==================== DayWork 网格 ====================

void GanttView::DrawDayWorks(time_t basetime)
{
	ImGuiIO& io = ImGui::GetIO();
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	float canvas_width = CanvasP1.x - CanvasP0.x - GridH * LabelCells();
	float half_canvas_width = ceil(canvas_width / 2 / GridH - 1) * GridH;

	const int sel_source_row = HasSelection ? RowOf(SelPlanIdx, SelGroupIdx, SelSubGroupIdx) : -1;
	time_t sel_min_date = 0, sel_max_date = 0;
	if (HasSelection)
		SelectionDateRange(sel_min_date, sel_max_date);
	bool left_press_hit_task = false;   // 本次左键按下是否命中了任务(未命中则按下空白)

	ImGui::PushClipRect(ImVec2(CanvasP0.x + GridH * LabelCells(), CanvasP0.y + GridV), CanvasP1, false);
	for (int row = 0; row < (int)Rows.size(); ++row)
	{
		const RowRef& r = Rows[row];
		if (r.SubGroup < 0)
			continue;
		auto& dayworks = Plans[r.Plan].WorkGroups[r.Group].WorkSubGroups[r.SubGroup].DayWorks;
		for (int l = 0; l < (int)dayworks.size(); ++l)
		{
			auto& daywork = dayworks[l];
			auto textsize = ImGui::CalcTextSize(daywork.Person.c_str());
			double diff_seconds = difftime(daywork.Date, basetime);
			int diff_days = (int)(diff_seconds / (60 * 60 * 24));

			// 移动预览:选中集合整体偏移(水平整天,垂直吸附到目标任务行)
			const bool selected = InSelection(r.Plan, r.Group, r.SubGroup, l);
			const bool is_move_anchor = Moving && selected && l == MoveAnchorWorkIdx;
			const int move_days = (selected && Moving) ? MoveOffsetDays : 0;
			const int cell_row = (selected && Moving && MovePreviewRow >= 0) ? MovePreviewRow : row;

			float cell_x = CanvasP0.x + Scrolling.x + GridH * (diff_days + move_days + LabelCells()) + half_canvas_width;
			float cell_y = GridV * 1 + GridV * cell_row + CanvasP0.y + Scrolling.y;
			float x0 = cell_x + 3;
			float y0 = cell_y + 3;
			float x1 = cell_x + GridH - 5;
			float y1 = cell_y + GridV - 5;

			// 命中测试:鼠标(绝对坐标)是否位于该 DayWork 单元格内
			const bool cell_hovered = io.MousePos.x >= cell_x && io.MousePos.x < cell_x + GridH
				&& io.MousePos.y >= cell_y && io.MousePos.y < cell_y + GridV;
			// 左键按下命中任务:单击选择单个;此前无选择时还可横向拖动扩展为多选
			if (!Moving && !RangeSelecting && cell_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				bool was_empty = !HasSelection;
				SelectSingle(r.Plan, r.Group, r.SubGroup, l);
				if (was_empty)
				{
					RangeSelecting = true;
					RangeAnchorDate = daywork.Date;
					RangeAnchorX = io.MousePos.x;
				}
				left_press_hit_task = true;
			}
			// 右键按下命中已选任务:开始拖动,移动整个选择集合
			if (!Moving && !RangeSelecting && cell_hovered && selected && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
			{
				Moving = true;
				MoveAnchorWorkIdx = l;
				MoveAnchorX = io.MousePos.x;
				MoveAnchorY = io.MousePos.y;
				MoveOffsetDays = 0;
				MoveOffsetRows = 0;
				MovePreviewRow = row;
			}

			if (selected)
			{
				// 原位置画虚影(记录原始行与原始日期)
				if (Moving && (move_days != 0 || MoveOffsetRows != 0))
				{
					float ghost_x = cell_x - move_days * GridH + 3;
					float ghost_y = GridV * 1 + GridV * row + CanvasP0.y + Scrolling.y + 3;
					draw_list->AddRect(ImVec2(ghost_x, ghost_y), ImVec2(ghost_x + GridH - 8, ghost_y + GridV - 5), IM_COL32(255, 255, 0, 90));
				}
				// 选中成员高亮(被抓住的那个边框更亮)
				draw_list->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(130, 100, 20, 255), 0);
				draw_list->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(255, 255, 0, is_move_anchor ? 255 : 160), 0);
				if (is_move_anchor)
				{
					// 目标行提示带
					if (MovePreviewRow >= 0)
						draw_list->AddRectFilled(ImVec2(CanvasP0.x + GridH * LabelCells(), cell_y), ImVec2(CanvasP1.x, cell_y + GridV), IM_COL32(255, 255, 0, 25));
					ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
					// 垂直拖动时,在预览单元格右侧显示目标任务名
					if (MovePreviewRow >= 0 && MovePreviewRow != sel_source_row)
					{
						const RowRef& target = Rows[MovePreviewRow];
						draw_list->AddText(ImVec2(x1 + 6, cell_y + (GridV - ImGui::GetTextLineHeight()) * 0.5f), IM_COL32(255, 255, 0, 220),
							Plans[target.Plan].WorkGroups[target.Group].WorkSubGroups[target.SubGroup].Name.c_str());
					}
					// 水平拖动时,在集合最左预览单元格上方显示目标日期(多选时为区间)
					if (move_days != 0)
					{
						char date_buf[64];
						time_t nd0 = sel_min_date + (time_t)move_days * 86400;
						struct tm tm0 = *localtime(&nd0);
						if (SelWorks.size() > 1)
						{
							time_t nd1 = sel_max_date + (time_t)move_days * 86400;
							struct tm tm1 = *localtime(&nd1);
							sprintf_s(date_buf, sizeof(date_buf), "%d-%02d-%02d ~ %d-%02d-%02d",
								tm0.tm_year + 1900, tm0.tm_mon + 1, tm0.tm_mday,
								tm1.tm_year + 1900, tm1.tm_mon + 1, tm1.tm_mday);
						}
						else
						{
							sprintf_s(date_buf, sizeof(date_buf), "%d-%02d-%02d", tm0.tm_year + 1900, tm0.tm_mon + 1, tm0.tm_mday);
						}
						float label_x = CanvasP0.x + Scrolling.x + GridH * ((int)(difftime(sel_min_date, basetime) / (60 * 60 * 24)) + move_days + LabelCells()) + half_canvas_width + 3;
						float text_y = y0 - 16.0f;
						if (text_y < CanvasP0.y + GridV + 2.0f) text_y = CanvasP0.y + GridV + 2.0f;
						draw_list->AddText(ImVec2(label_x, text_y), IM_COL32(255, 255, 0, 255), date_buf);
					}
				}
			}
			else
			{
				draw_list->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(100, 100, 100, 255), 0);
				// 悬停提示可点击选择
				if (cell_hovered && !Moving && !RangeSelecting)
				{
					draw_list->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(230, 230, 230, 220), 0);
					ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
				}
			}

			ImVec2 textPos;
			textPos.x = cell_x + (GridH - textsize.x) * 0.5f;
			textPos.y = cell_y + (GridV - textsize.y) * 0.5f;
			ImGui::SetCursorPos(textPos);
			ImGui::Text(daywork.Person.c_str());
		}
	}
	ImGui::PopClipRect();

	// 画布内按下但未命中任何任务:左键/右键均视为点击空白,取消选择
	const bool mouse_in_canvas = io.MousePos.x >= CanvasP0.x && io.MousePos.x < CanvasP1.x
		&& io.MousePos.y >= CanvasP0.y && io.MousePos.y < CanvasP1.y;
	if (mouse_in_canvas && !left_press_hit_task && !RangeSelecting && !Moving)
	{
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right))
			ClearSelection();
	}
}

// ==================== 选择与拖动移动 ====================

// 每帧驱动左键拖选与右键拖动两个状态机
void GanttView::UpdateInteraction()
{
	if (RangeSelecting)
		UpdateRangeSelect();
	if (Moving)
		UpdateMove();
}

// 左键拖动中:以锚点日期与当前鼠标水平偏移确定同行日期区间,实时重算选择集合
void GanttView::UpdateRangeSelect()
{
	ImGuiIO& io = ImGui::GetIO();
	if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) && !ImGui::IsMouseReleased(ImGuiMouseButton_Left))
	{
		RangeSelecting = false;   // 鼠标状态丢失(如窗口失焦),结束拖选
		return;
	}
	int offset_days = (int)roundf((io.MousePos.x - RangeAnchorX) / GridH);
	SelectDateRange(RangeAnchorDate, RangeAnchorDate + (time_t)offset_days * 86400);
	if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		RangeSelecting = false;
}

// 右键拖动中:按网格大小吸附为整天/整行偏移,行号吸附到最近的有效任务行;
// 松开右键:有位移则提交移动,原地松手则视为右键单击取消选择
void GanttView::UpdateMove()
{
	ImGuiIO& io = ImGui::GetIO();
	if (!ImGui::IsMouseDown(ImGuiMouseButton_Right) && !ImGui::IsMouseReleased(ImGuiMouseButton_Right))
	{
		Moving = false;   // 鼠标状态丢失(如窗口失焦),放弃本次拖动(保留选择)
		return;
	}
	// 按网格大小吸附为整天/整行偏移,行号吸附到最近的有效任务行
	MoveOffsetDays = (int)roundf((io.MousePos.x - MoveAnchorX) / GridH);
	MoveOffsetRows = (int)roundf((io.MousePos.y - MoveAnchorY) / GridV);
	MovePreviewRow = NearestSubgroupRow(RowOf(SelPlanIdx, SelGroupIdx, SelSubGroupIdx) + MoveOffsetRows);
	if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
	{
		int source_row = RowOf(SelPlanIdx, SelGroupIdx, SelSubGroupIdx);
		if (MoveOffsetDays == 0 && MovePreviewRow == source_row)
			ClearSelection();
		else
			CommitMove();
		Moving = false;
	}
}

// 提交:选中集合整体改日期;若目标行不同则整组移动到目标行的 WorkSubGroup(按日期升序追加),
// 选择跟随到新位置
void GanttView::CommitMove()
{
	auto& srcDayWorks = Plans[SelPlanIdx].WorkGroups[SelGroupIdx].WorkSubGroups[SelSubGroupIdx].DayWorks;
	if (MoveOffsetDays != 0)
		for (int idx : SelWorks)
			srcDayWorks[idx].Date += (time_t)MoveOffsetDays * 86400;
	RowRef target = (MovePreviewRow >= 0 && MovePreviewRow < (int)Rows.size()) ? Rows[MovePreviewRow] : RowRef();
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

void GanttView::ClearSelection()
{
	HasSelection = false;
	SelPlanIdx = SelGroupIdx = SelSubGroupIdx = -1;
	SelWorks.clear();
	RangeSelecting = false;
	RangeAnchorDate = 0;
	RangeAnchorX = 0.0f;
}

void GanttView::SelectSingle(int plan, int group, int subgroup, int work)
{
	HasSelection = true;
	SelPlanIdx = plan;
	SelGroupIdx = group;
	SelSubGroupIdx = subgroup;
	SelWorks.assign(1, work);
}

// 选择当前行内日期落在 [d0,d1] 的所有任务(区间端点由锚点日期与拖动偏移得出,必含锚点)
void GanttView::SelectDateRange(time_t d0, time_t d1)
{
	if (d0 > d1)
	{
		time_t t = d0;
		d0 = d1;
		d1 = t;
	}
	auto& dayworks = Plans[SelPlanIdx].WorkGroups[SelGroupIdx].WorkSubGroups[SelSubGroupIdx].DayWorks;
	SelWorks.clear();
	for (int l = 0; l < (int)dayworks.size(); ++l)
		if (dayworks[l].Date >= d0 && dayworks[l].Date <= d1)
			SelWorks.push_back(l);
	if (!SelWorks.empty())
		HasSelection = true;
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
