#define IMGUI_DEFINE_MATH_OPERATORS
#include "gantt_view.h"
#include "../ImGuiScaffoldSDL3GL/imgui/imgui_internal.h"
#include <algorithm>

GanttView::GanttView(std::vector<Task::Plan>& plans) : Plans(plans) {}

void GanttView::Draw()
{
	RebuildRows();
	time_t basetime = TodayMidnight();
	UpdateDrag();
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

	// 交互捕获:左键拖拽平移,右键拖拽 DayWork,右键点击弹菜单
	ImGui::InvisibleButton("canvas", canvas_sz, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
	const bool is_active = ImGui::IsItemActive();

	// 左键拖拽:平移(按网格吸附,顶部不超过第一行)
	if (is_active && ImGui::IsMouseDragging(ImGuiMouseButton_Left, -1.0f))
	{
		ScrollingReal.x += io.MouseDelta.x;
		ScrollingReal.y += io.MouseDelta.y;
		if (ScrollingReal.y > 0) ScrollingReal.y = 0;
		Scrolling.x = roundf(ScrollingReal.x / GridH) * GridH;
		Scrolling.y = roundf(ScrollingReal.y / GridV) * GridV;
	}

	// 右键点击(无拖拽位移)时弹上下文菜单
	ImVec2 drag_delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
	if (drag_delta.x == 0.0f && drag_delta.y == 0.0f)
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

			// 拖拽集合成员:被拖的 DayWork 本身,或 Shift 组拖拽时与它日期相邻的 DayWork
			const bool is_drag_anchor = Dragging && r.Plan == DragPlanIdx && r.Group == DragGroupIdx && r.SubGroup == DragSubGroupIdx && l == DragWorkIdx;
			bool in_drag_set = is_drag_anchor;
			if (!in_drag_set && Dragging && r.Plan == DragPlanIdx && r.Group == DragGroupIdx && r.SubGroup == DragSubGroupIdx)
				for (int m : DragSet)
					if (m == l) { in_drag_set = true; break; }
			const int drag_days = in_drag_set ? DragOffsetDays : 0;
			// 拖拽预览绘制在目标行(吸附后的任务行),非拖拽时就是自身所在行
			const int cell_row = (in_drag_set && DragPreviewRow >= 0) ? DragPreviewRow : row;

			float cell_x = CanvasP0.x + Scrolling.x + GridH * (diff_days + drag_days + LabelCells()) + half_canvas_width;
			float cell_y = GridV * 1 + GridV * cell_row + CanvasP0.y + Scrolling.y;
			float x0 = cell_x + 3;
			float y0 = cell_y + 3;
			float x1 = cell_x + GridH - 5;
			float y1 = cell_y + GridV - 5;

			// 命中测试:鼠标(绝对坐标)是否位于该 DayWork 单元格内
			const bool cell_hovered = io.MousePos.x >= cell_x && io.MousePos.x < cell_x + GridH
				&& io.MousePos.y >= cell_y && io.MousePos.y < cell_y + GridV;
			// 右键按下:开始拖拽该 DayWork
			if (!Dragging && cell_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
			{
				Dragging = true;
				DragPlanIdx = r.Plan; DragGroupIdx = r.Group; DragSubGroupIdx = r.SubGroup; DragWorkIdx = l;
				DragAnchorX = io.MousePos.x;
				DragAnchorY = io.MousePos.y;
				DragOffsetDays = 0;
				DragOffsetRows = 0;
				DragPreviewRow = row;
				BuildDragSet(io.KeyShift);   // Shift 按住时连同相邻日期一起拖
			}

			if (in_drag_set)
			{
				// 原位置画虚影(记录原始行与原始日期)
				if (drag_days != 0 || DragOffsetRows != 0)
				{
					float ghost_x = cell_x - drag_days * GridH + 3;
					float ghost_y = GridV * 1 + GridV * row + CanvasP0.y + Scrolling.y + 3;
					draw_list->AddRect(ImVec2(ghost_x, ghost_y), ImVec2(ghost_x + GridH - 8, ghost_y + GridV - 5), IM_COL32(255, 255, 0, 90));
				}
				// 组成员高亮(被抓住的那个边框更亮)
				draw_list->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(130, 100, 20, 255), 0);
				draw_list->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(255, 255, 0, is_drag_anchor ? 255 : 160), 0);
				if (is_drag_anchor)
				{
					// 目标行提示带
					if (DragPreviewRow >= 0)
						draw_list->AddRectFilled(ImVec2(CanvasP0.x + GridH * LabelCells(), cell_y), ImVec2(CanvasP1.x, cell_y + GridV), IM_COL32(255, 255, 0, 25));
					ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
					// 垂直拖动时,在预览单元格右侧显示目标任务名
					if (DragOffsetRows != 0 && DragPreviewRow >= 0)
					{
						const RowRef& target = Rows[DragPreviewRow];
						draw_list->AddText(ImVec2(x1 + 6, cell_y + (GridV - ImGui::GetTextLineHeight()) * 0.5f), IM_COL32(255, 255, 0, 220),
							Plans[target.Plan].WorkGroups[target.Group].WorkSubGroups[target.SubGroup].Name.c_str());
					}
					// 水平拖动时,在集合最左预览单元格上方显示目标日期(组拖拽时为区间)
					if (drag_days != 0)
					{
						char date_buf[64];
						time_t nd0 = DragSetMinDate + (time_t)drag_days * 86400;
						struct tm tm0 = *localtime(&nd0);
						if (DragSet.size() > 1)
						{
							time_t nd1 = DragSetMaxDate + (time_t)drag_days * 86400;
							struct tm tm1 = *localtime(&nd1);
							sprintf_s(date_buf, sizeof(date_buf), "%d-%02d-%02d ~ %d-%02d-%02d",
								tm0.tm_year + 1900, tm0.tm_mon + 1, tm0.tm_mday,
								tm1.tm_year + 1900, tm1.tm_mon + 1, tm1.tm_mday);
						}
						else
						{
							sprintf_s(date_buf, sizeof(date_buf), "%d-%02d-%02d", tm0.tm_year + 1900, tm0.tm_mon + 1, tm0.tm_mday);
						}
						float label_x = CanvasP0.x + Scrolling.x + GridH * ((int)(difftime(DragSetMinDate, basetime) / (60 * 60 * 24)) + drag_days + LabelCells()) + half_canvas_width + 3;
						float text_y = y0 - 16.0f;
						if (text_y < CanvasP0.y + GridV + 2.0f) text_y = CanvasP0.y + GridV + 2.0f;
						draw_list->AddText(ImVec2(label_x, text_y), IM_COL32(255, 255, 0, 255), date_buf);
					}
				}
			}
			else
			{
				draw_list->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(100, 100, 100, 255), 0);
				// 悬停提示可拖拽
				if (cell_hovered && !Dragging)
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
}

// ==================== 拖拽 ====================

// 右键拖拽 DayWork:水平拖动改变日期,垂直拖动移动到其他任务行(WorkSubGroup);
// 按住 Shift 拖拽时,日期连续相邻的 DayWork 作为一组一起移动
void GanttView::UpdateDrag()
{
	if (!Dragging)
		return;
	ImGuiIO& io = ImGui::GetIO();
	if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
	{
		CommitDrag();
		CancelDrag();
	}
	else if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
	{
		// 按网格大小吸附为整天/整行偏移,行号吸附到最近的有效任务行
		DragOffsetDays = (int)roundf((io.MousePos.x - DragAnchorX) / GridH);
		DragOffsetRows = (int)roundf((io.MousePos.y - DragAnchorY) / GridV);
		DragPreviewRow = NearestSubgroupRow(RowOf(DragPlanIdx, DragGroupIdx, DragSubGroupIdx) + DragOffsetRows);
		// 每帧重算集合:拖拽中按住/松开 Shift 可实时增减一起移动的成员
		BuildDragSet(io.KeyShift);
	}
	else
	{
		// 鼠标状态丢失(如窗口失焦),取消拖拽
		CancelDrag();
	}
}

// 松开右键:提交整组(Shift 时为相邻日期集合,否则仅单个)的日期与所属任务修改
void GanttView::CommitDrag()
{
	auto& srcDayWorks = Plans[DragPlanIdx].WorkGroups[DragGroupIdx].WorkSubGroups[DragSubGroupIdx].DayWorks;
	if (DragOffsetDays != 0)
		for (int idx : DragSet)
			srcDayWorks[idx].Date += (time_t)DragOffsetDays * 86400;
	RowRef target = (DragPreviewRow >= 0 && DragPreviewRow < (int)Rows.size()) ? Rows[DragPreviewRow] : RowRef();
	if (target.SubGroup < 0 || (target.Plan == DragPlanIdx && target.Group == DragGroupIdx && target.SubGroup == DragSubGroupIdx))
		return;
	// 整组移动到目标行的 WorkSubGroup(按日期升序追加到其 DayWorks 末尾)
	std::vector<Task::DayWork> moved;
	std::vector<Task::DayWork> kept;
	moved.reserve(DragSet.size());
	kept.reserve(srcDayWorks.size());
	for (int idx = 0; idx < (int)srcDayWorks.size(); ++idx)
	{
		bool is_member = false;
		for (int m : DragSet)
			if (m == idx) { is_member = true; break; }
		if (is_member)
			moved.push_back(std::move(srcDayWorks[idx]));
		else
			kept.push_back(std::move(srcDayWorks[idx]));
	}
	std::sort(moved.begin(), moved.end(), [](const Task::DayWork& a, const Task::DayWork& b) { return a.Date < b.Date; });
	srcDayWorks = std::move(kept);
	auto& dstDayWorks = Plans[target.Plan].WorkGroups[target.Group].WorkSubGroups[target.SubGroup].DayWorks;
	for (auto& dw : moved)
		dstDayWorks.push_back(std::move(dw));
}

void GanttView::CancelDrag()
{
	Dragging = false;
	DragPlanIdx = DragGroupIdx = DragSubGroupIdx = DragWorkIdx = -1;
	DragOffsetDays = 0;
	DragOffsetRows = 0;
	DragPreviewRow = -1;
	DragSet.clear();
	DragSetMinDate = 0;
	DragSetMaxDate = 0;
}

int GanttView::FindWorkIndexByDate(int plan, int group, int subgroup, time_t date) const
{
	auto& dayworks = Plans[plan].WorkGroups[group].WorkSubGroups[subgroup].DayWorks;
	for (int l = 0; l < (int)dayworks.size(); ++l)
		if (dayworks[l].Date == date)
			return l;
	return -1;
}

// with_shift 为 true 时,从被拖的 DayWork 出发向日期两侧扩展连续相邻的成员
void GanttView::BuildDragSet(bool with_shift)
{
	DragSet.clear();
	DragSet.push_back(DragWorkIdx);
	auto& dayworks = Plans[DragPlanIdx].WorkGroups[DragGroupIdx].WorkSubGroups[DragSubGroupIdx].DayWorks;
	DragSetMinDate = dayworks[DragWorkIdx].Date;
	DragSetMaxDate = dayworks[DragWorkIdx].Date;
	if (!with_shift)
		return;
	for (time_t d = DragSetMinDate - 86400; ; d -= 86400)
	{
		int idx = FindWorkIndexByDate(DragPlanIdx, DragGroupIdx, DragSubGroupIdx, d);
		if (idx < 0) break;
		DragSet.push_back(idx);
		DragSetMinDate = d;
	}
	for (time_t d = DragSetMaxDate + 86400; ; d += 86400)
	{
		int idx = FindWorkIndexByDate(DragPlanIdx, DragGroupIdx, DragSubGroupIdx, d);
		if (idx < 0) break;
		DragSet.push_back(idx);
		DragSetMaxDate = d;
	}
}
