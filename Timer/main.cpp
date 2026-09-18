#define IMGUI_DEFINE_MATH_OPERATORS
#include "main.h"
#include "custommath.h"
#include <algorithm>
#include "./../ImGuiScaffoldSDL3GL/imgui/imgui_internal.h"
//const float GRID_SIZE_H = 32;
const float GRID_SIZE_H = 70;
const float GRID_SIZE_V = 32;
const int PLAN_NAME_CELLS = 2;
const int TASK_GROUP_CELLS = 2;
const int TASK_SUBGROUP_CELLS = 1;
//static ImVec2 scrolling_real(0.0f, 0.0f);
static ImVec2 scrolling(0.0f, 0.0f);
//static ImVec2 grid_offsetcells = {};
static ImVec2 canvas_p0 = {};
static ImVec2 canvas_p1 = {};
//static Task::GanntView view(make_time(2026, 8, 1), make_time(2026, 8, 30));
//static Task::Plan plan1(
//	"Plan 1",
//	{
//		make_time(2026, 8, 1),
//		make_time(2026, 8, 2),
//		make_time(2026, 8, 3)
//	});
//static Task::Plan plan2(
//	"Plan 2",
//	{
//		make_time(2026, 8, 10),
//		make_time(2026, 8, 11),
//		make_time(2026, 8, 12)
//	});
static std::vector<Task::Plan> plans = {
	Task::Plan(
		"Order0001",
		{
			Task::WorkGroup(
				"合同",
				{
					Task::WorkSubGroup(
					"解读",
						{
							Task::DayWork("严", make_time(2026, 9, 2)),
							Task::DayWork("严", make_time(2026, 9, 3)),
							Task::DayWork("严", make_time(2026, 9, 4))
						}
					),
				}
			),
			Task::WorkGroup(
				"动力系统",
				{
					Task::WorkSubGroup(
					"设计",
						{
							Task::DayWork("李", make_time(2026, 9, 11)),
							Task::DayWork("李", make_time(2026, 9, 12)),
							Task::DayWork("李", make_time(2026, 9, 13))
						}
					),
					Task::WorkSubGroup(
					"检查",
						{
							Task::DayWork("张", make_time(2026, 9, 15)),
							Task::DayWork("张", make_time(2026, 9, 16)),
							Task::DayWork("张", make_time(2026, 9, 17))
						}
					),
				}
			),
			Task::WorkGroup(
				"转向系统",
				{
					Task::WorkSubGroup(
					"设计",
						{
							Task::DayWork("王", make_time(2026, 9, 21)),
							Task::DayWork("王", make_time(2026, 9, 22)),
							Task::DayWork("王", make_time(2026, 9, 23))
						}
					),
					Task::WorkSubGroup(
					"检查",
						{
							Task::DayWork("何", make_time(2026, 10, 8)),
							Task::DayWork("何", make_time(2026, 10, 9)),
							Task::DayWork("何", make_time(2026, 10, 10))
						}
					),
				}
			)
		}
	),
	Task::Plan("Order0009", { }),
	Task::Plan(
		"Order0008",
		{
			Task::WorkGroup(
				"空调系统",
				{
				}
			),
			Task::WorkGroup(
				"照明系统",
				{
					Task::WorkSubGroup(
					"设计",
						{
							Task::DayWork("王", make_time(2026, 9, 6)),
							Task::DayWork("王", make_time(2026, 9, 7)),
							Task::DayWork("王", make_time(2026, 9, 8))
						}
					),
					Task::WorkSubGroup(
					"检查",
						{
						}
					),
				}
			),
			Task::WorkGroup(
				"xx系统",
				{
				}
			),
		}
	),
	Task::Plan(
		"Order0002",
		{
			Task::WorkGroup(
				"空调系统",
				{
					Task::WorkSubGroup(
					"设计",
						{
							Task::DayWork("张", make_time(2026, 9, 26)),
							Task::DayWork("张", make_time(2026, 9, 27)),
							Task::DayWork("张", make_time(2026, 9, 28))
						}
					),
					Task::WorkSubGroup(
					"检查",
						{
							Task::DayWork("李", make_time(2026, 9, 27)),
							Task::DayWork("李", make_time(2026, 9, 28)),
							Task::DayWork("李", make_time(2026, 9, 29))
						}
					),
				}
			),
			Task::WorkGroup(
				"照明系统",
				{
					Task::WorkSubGroup(
					"设计",
						{
							Task::DayWork("王", make_time(2026, 9, 6)),
							Task::DayWork("王", make_time(2026, 9, 7)),
							Task::DayWork("王", make_time(2026, 9, 8))
						}
					),
					Task::WorkSubGroup(
					"检查",
						{
							Task::DayWork("何", make_time(2026, 9, 9)),
							Task::DayWork("何", make_time(2026, 9, 10)),
							Task::DayWork("何", make_time(2026, 9, 11))
						}
					),
				}
			)
		}
	)
};
static void drawCanvas()
{
	static ImVector<ImVec2> points;
	static bool opt_enable_context_menu = true;

	static ImVec2 rect_min = {};
	static ImVec2 rect_max = {};
	static bool draw_rect = false;

	//ImGui::Checkbox("Enable grid", &opt_enable_grid);
	//ImGui::Checkbox("Enable context menu", &opt_enable_context_menu);
	//ImGui::Text("Mouse Left: drag to add lines,\nMouse Right: drag to scroll, click for context menu.");
	static ImVec2 cursorPos = ImGui::GetCursorPos();
	// Typically you would use a BeginChild()/EndChild() pair to benefit from a clipping region + own scrolling.
	// Here we demonstrate that this can be replaced by simple offsetting + custom drawing + PushClipRect/PopClipRect() calls.
	// To use a child window instead we could use, e.g:
	//      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));      // Disable padding
	//      ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(50, 50, 50, 255));  // Set a background color
	//      ImGui::BeginChild("canvas", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoMove);
	//      ImGui::PopStyleColor();
	//      ImGui::PopStyleVar();
	//      [...]
	//      ImGui::EndChild();

	// Using InvisibleButton() as a convenience 1) it will advance the layout cursor and 2) allows us to use IsItemHovered()/IsItemActive()
	canvas_p0 = ImGui::GetCursorScreenPos();      // ImDrawList API uses screen coordinates!
	ImVec2 canvas_sz = ImGui::GetContentRegionAvail();   // Resize canvas to what's available
	if (canvas_sz.x < 50.0f) canvas_sz.x = 50.0f;
	if (canvas_sz.y < 50.0f) canvas_sz.y = 50.0f;
	canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);

	// Draw border and background color
	ImGuiIO& io = ImGui::GetIO();
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(50, 50, 50, 255));
	draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(255, 255, 255, 255));

	// This will catch our interactions
	ImGui::InvisibleButton("canvas", canvas_sz, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
	const bool is_hovered = ImGui::IsItemHovered(); // Hovered
	const bool is_active = ImGui::IsItemActive();   // Held
	//const ImVec2 origin(canvas_p0.x + scrolling.x, canvas_p0.y + scrolling.y); // Lock scrolled origin
	//const ImVec2 mouse_pos_in_canvas(io.MousePos.x - origin.x, io.MousePos.y - origin.y);

	//// Add first and second point
	//if (is_hovered && !draw_rect && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	//{
	//	points.push_back(mouse_pos_in_canvas);
	//	points.push_back(mouse_pos_in_canvas);
	//	draw_rect = true;
	//}
	//if (draw_rect)
	//{
	//	points.back() = mouse_pos_in_canvas;
	//	if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
	//		draw_rect = false;
	//}

	// Pan (we use a zero mouse threshold when there's no context menu)
	// You may decide to make that threshold dynamic based on whether the mouse is hovering something etc.
	const float mouse_threshold_for_pan = opt_enable_context_menu ? -1.0f : 0.0f;
	if (is_active && ImGui::IsMouseDragging(ImGuiMouseButton_Left, mouse_threshold_for_pan))
	{
		static ImVec2 scrolling_real(0.0f, 0.0f);
		scrolling_real.x += io.MouseDelta.x;
		scrolling_real.y += io.MouseDelta.y;
		if (scrolling_real.y > 0) scrolling_real.y = 0;
		scrolling.x = roundf(scrolling_real.x / GRID_SIZE_H) * GRID_SIZE_H;
		scrolling.y = roundf(scrolling_real.y / GRID_SIZE_V) * GRID_SIZE_V;
	}
	//static int wheel_counter = 0;
	//if (is_active && ImGui::IsItemHovered()) {
	//	wheel_counter += ImGui::GetIO().MouseWheel;
	//}
	// Context menu (under default mouse threshold)
	ImVec2 drag_delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
	if (opt_enable_context_menu && drag_delta.x == 0.0f && drag_delta.y == 0.0f)
		ImGui::OpenPopupOnItemClick("context", ImGuiPopupFlags_MouseButtonRight);
	if (ImGui::BeginPopup("context"))
	{
		if (draw_rect)
			points.resize(points.size() - 2);
		draw_rect = false;
		if (ImGui::MenuItem("Remove one", NULL, false, points.Size > 0)) { points.resize(points.size() - 2); }
		if (ImGui::MenuItem("Remove all", NULL, false, points.Size > 0)) { points.clear(); }
		if (ImGui::MenuItem("Back to Today"))
		{
			scrolling.x = 0;
			scrolling.y = 0;
		}
		ImGui::EndPopup();
	}

	// Draw grid + all lines in the canvas
	draw_list->PushClipRect(canvas_p0, canvas_p1, true);

	//draw grid vertical lines
	for (float x = fmodf(scrolling.x, GRID_SIZE_H); x < canvas_sz.x; x += GRID_SIZE_H)
	{
		if (x < GRID_SIZE_H * PLAN_NAME_CELLS)
			draw_list->AddLineV(canvas_p0.x + GRID_SIZE_H * PLAN_NAME_CELLS, canvas_p0.y, canvas_p1.y, IM_COL32(200, 200, 200, 40));
		else if (x < GRID_SIZE_H * (PLAN_NAME_CELLS + TASK_GROUP_CELLS))
			draw_list->AddLineV(canvas_p0.x + GRID_SIZE_H * (PLAN_NAME_CELLS + TASK_GROUP_CELLS), canvas_p0.y, canvas_p1.y, IM_COL32(200, 200, 200, 40));
		else if (x < GRID_SIZE_H * (PLAN_NAME_CELLS + TASK_GROUP_CELLS + TASK_SUBGROUP_CELLS))
			draw_list->AddLineV(canvas_p0.x + GRID_SIZE_H * (PLAN_NAME_CELLS + TASK_GROUP_CELLS + TASK_SUBGROUP_CELLS), canvas_p0.y, canvas_p1.y, IM_COL32(200, 200, 200, 40));
		else
			draw_list->AddLineV(canvas_p0.x + x, canvas_p0.y, canvas_p1.y, IM_COL32(200, 200, 200, 40));

	}

	//draw grid horizontal lines
	for (float y = fmodf(scrolling.y, GRID_SIZE_V); y < canvas_sz.y; y += GRID_SIZE_V)
	{
		if (y < GRID_SIZE_V)
			draw_list->AddLineH(canvas_p0.x, canvas_p1.x, canvas_p0.y + GRID_SIZE_V, IM_COL32(200, 200, 200, 80));
		else
			draw_list->AddLineH(canvas_p0.x, canvas_p1.x, canvas_p0.y + y, IM_COL32(200, 200, 200, 40));
	}

	//for (int n = 0; n < points.Size; n += 2)
	//	draw_list->AddRect(ImVec2(origin.x + points[n].x, origin.y + points[n].y), ImVec2(origin.x + points[n + 1].x, origin.y + points[n + 1].y), IM_COL32(255, 255, 0, 255), 2.0f);
	draw_list->PopClipRect();
	ImGui::SetCursorPos(cursorPos);
}
// ---- 行号映射辅助:甘特图任务行 = Plan/WorkGroup/WorkSubGroup 的扁平化,空 Plan 或空 WorkGroup 各占一个占位行 ----
// 把扁平行号映射到 plans[i].WorkGroups[j].WorkSubGroups[k];占位行返回 false
static bool taskRowAt(int row, int& out_i, int& out_j, int& out_k)
{
	int r = 0;
	for (int i = 0; i < (int)plans.size(); ++i)
	{
		if (plans[i].WorkGroups.size() == 0) { if (r == row) return false; ++r; continue; }
		for (int j = 0; j < (int)plans[i].WorkGroups.size(); ++j)
		{
			if (plans[i].WorkGroups[j].WorkSubGroups.size() == 0) { if (r == row) return false; ++r; continue; }
			for (int k = 0; k < (int)plans[i].WorkGroups[j].WorkSubGroups.size(); ++k)
			{
				if (r == row) { out_i = i; out_j = j; out_k = k; return true; }
				++r;
			}
		}
	}
	return false;
}

static int taskRowCount()
{
	int r = 0;
	for (int i = 0; i < (int)plans.size(); ++i)
	{
		if (plans[i].WorkGroups.size() == 0) { ++r; continue; }
		for (int j = 0; j < (int)plans[i].WorkGroups.size(); ++j)
		{
			if (plans[i].WorkGroups[j].WorkSubGroups.size() == 0) { ++r; continue; }
			r += (int)plans[i].WorkGroups[j].WorkSubGroups.size();
		}
	}
	return r;
}

static int taskRowOf(int ti, int tj, int tk)
{
	int r = 0;
	for (int i = 0; i < (int)plans.size(); ++i)
	{
		if (plans[i].WorkGroups.size() == 0) { ++r; continue; }
		for (int j = 0; j < (int)plans[i].WorkGroups.size(); ++j)
		{
			if (plans[i].WorkGroups[j].WorkSubGroups.size() == 0) { ++r; continue; }
			for (int k = 0; k < (int)plans[i].WorkGroups[j].WorkSubGroups.size(); ++k)
			{
				if (i == ti && j == tj && k == tk) return r;
				++r;
			}
		}
	}
	return -1;
}

// 从 want 行出发向两侧搜索最近的"实际 WorkSubGroup 行"(跳过空 Plan/空 WorkGroup 的占位行)
static int nearestSubgroupRow(int want, int total)
{
	if (total <= 0) return -1;
	if (want < 0) want = 0;
	if (want > total - 1) want = total - 1;
	int i, j, k;
	if (taskRowAt(want, i, j, k)) return want;
	for (int d = 1; d < total; ++d)
	{
		if (want - d >= 0 && taskRowAt(want - d, i, j, k)) return want - d;
		if (want + d < total && taskRowAt(want + d, i, j, k)) return want + d;
	}
	return -1;
}

// ---- Shift 组拖拽:按住 Shift 时,同 WorkSubGroup 内与被拖 DayWork 日期连续相邻的 DayWork 一起移动 ----
static std::vector<int> drag_group_ls = {};   // 组成员(DayWorks 索引,含被拖的那个)
static time_t drag_group_min_date = 0;        // 组内最早日期(标注用)
static time_t drag_group_max_date = 0;        // 组内最晚日期(标注用)

static int findDayWorkIndexByDate(int i, int j, int k, time_t date)
{
	auto& dws = plans[i].WorkGroups[j].WorkSubGroups[k].DayWorks;
	for (int l = 0; l < (int)dws.size(); ++l)
		if (dws[l].Date == date)
			return l;
	return -1;
}

// with_shift 为 true 时,从被拖的 DayWork 出发向日期两侧扩展连续相邻的成员
static void buildDragGroup(bool with_shift, int i, int j, int k, int l)
{
	drag_group_ls.clear();
	drag_group_ls.push_back(l);
	auto& dws = plans[i].WorkGroups[j].WorkSubGroups[k].DayWorks;
	drag_group_min_date = dws[l].Date;
	drag_group_max_date = dws[l].Date;
	if (!with_shift)
		return;
	for (time_t d = dws[l].Date - 86400; ; d -= 86400)
	{
		int idx = findDayWorkIndexByDate(i, j, k, d);
		if (idx < 0) break;
		drag_group_ls.push_back(idx);
		drag_group_min_date = d;
	}
	for (time_t d = dws[l].Date + 86400; ; d += 86400)
	{
		int idx = findDayWorkIndexByDate(i, j, k, d);
		if (idx < 0) break;
		drag_group_ls.push_back(idx);
		drag_group_max_date = d;
	}
}

static void drawGanntView(time_t basetime)
{
	static struct tm* tm_info;
	static ImVec2 textPos = {};
	static char buffer[50];
	float canvas_width = canvas_p1.x - canvas_p0.x - GRID_SIZE_H * (PLAN_NAME_CELLS + TASK_GROUP_CELLS + TASK_SUBGROUP_CELLS);
	float half_canvas_width = ceil(canvas_width / 2 / GRID_SIZE_H - 1) * GRID_SIZE_H;
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ImGuiIO& io = ImGui::GetIO();
	auto mousePos = io.MousePos - canvas_p0;

	// ---- 右键拖拽 DayWork:水平拖动改变日期,垂直拖动移动到其他任务行(WorkSubGroup);
	//      按住 Shift 拖拽时,日期连续相邻的 DayWork 作为一组一起移动 ----
	static bool dragging_daywork = false;   // 正在拖拽某个 DayWork
	static int drag_i = -1, drag_j = -1, drag_k = -1, drag_l = -1; // 拖拽目标: plans[i].WorkGroups[j].WorkSubGroups[k].DayWorks[l]
	static float drag_anchor_x = 0.0f;      // 按下右键时的鼠标 X
	static float drag_anchor_y = 0.0f;      // 按下右键时的鼠标 Y
	static int drag_offset_days = 0;        // 当前拖拽的水平偏移(天)
	static int drag_offset_rows = 0;        // 当前拖拽的垂直偏移(行)
	static int drag_preview_row = -1;       // 吸附后的预览行(松手后落入的任务行)

	if (dragging_daywork)
	{
		if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
		{
			// 松开右键:提交整组(Shift 时为相邻日期集合,否则仅单个)的日期与所属任务修改
			auto& srcDayWorks = plans[drag_i].WorkGroups[drag_j].WorkSubGroups[drag_k].DayWorks;
			if (drag_offset_days != 0)
				for (int idx : drag_group_ls)
					srcDayWorks[idx].Date += (time_t)drag_offset_days * 86400;
			int ti, tj, tk;
			if (drag_preview_row >= 0 && taskRowAt(drag_preview_row, ti, tj, tk)
				&& (ti != drag_i || tj != drag_j || tk != drag_k))
			{
				// 整组移动到目标行的 WorkSubGroup(按日期升序追加到其 DayWorks 末尾)
				std::vector<Task::DayWork> moved;
				std::vector<Task::DayWork> kept;
				moved.reserve(drag_group_ls.size());
				kept.reserve(srcDayWorks.size());
				for (int idx = 0; idx < (int)srcDayWorks.size(); ++idx)
				{
					bool is_member = false;
					for (int m : drag_group_ls)
						if (m == idx) { is_member = true; break; }
					if (is_member)
						moved.push_back(std::move(srcDayWorks[idx]));
					else
						kept.push_back(std::move(srcDayWorks[idx]));
				}
				std::sort(moved.begin(), moved.end(), [](const Task::DayWork& a, const Task::DayWork& b) { return a.Date < b.Date; });
				srcDayWorks = std::move(kept);
				auto& dstDayWorks = plans[ti].WorkGroups[tj].WorkSubGroups[tk].DayWorks;
				for (auto& dw : moved)
					dstDayWorks.push_back(std::move(dw));
			}
			dragging_daywork = false;
			drag_i = drag_j = drag_k = drag_l = -1;
			drag_offset_days = 0;
			drag_offset_rows = 0;
			drag_preview_row = -1;
			drag_group_ls.clear();
			drag_group_min_date = 0;
			drag_group_max_date = 0;
		}
		else if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
		{
			// 按网格大小吸附为整天/整行偏移,行号吸附到最近的有效任务行
			drag_offset_days = (int)roundf((io.MousePos.x - drag_anchor_x) / GRID_SIZE_H);
			drag_offset_rows = (int)roundf((io.MousePos.y - drag_anchor_y) / GRID_SIZE_V);
			drag_preview_row = nearestSubgroupRow(taskRowOf(drag_i, drag_j, drag_k) + drag_offset_rows, taskRowCount());
			// 每帧重算组:拖拽中按住/松开 Shift 可实时增减一起移动的成员
			buildDragGroup(io.KeyShift, drag_i, drag_j, drag_k, drag_l);
		}
		else
		{
			// 鼠标状态丢失(如窗口失焦),取消拖拽
			dragging_daywork = false;
			drag_i = drag_j = drag_k = drag_l = -1;
			drag_offset_days = 0;
			drag_offset_rows = 0;
			drag_preview_row = -1;
			drag_group_ls.clear();
			drag_group_min_date = 0;
			drag_group_max_date = 0;
		}
	}

	{
		ImGui::PushClipRect(ImVec2(canvas_p0.x, canvas_p0.y + GRID_SIZE_V), canvas_p1, false);
		int row = 0;
		for (int i = 0; i < plans.size(); ++i)
		{
			auto textsize = ImGui::CalcTextSize(plans[i].Name.c_str());
			textPos.x = canvas_p0.x  + (GRID_SIZE_H * PLAN_NAME_CELLS - textsize.x) * 0.5f;
			textPos.y = GRID_SIZE_V * 1 + GRID_SIZE_V * row + canvas_p0.y + scrolling.y + (GRID_SIZE_V - textsize.y) * 0.5f;
			ImGui::SetCursorPos(textPos);
			int count = 0;
			for (const auto& wg : plans[i].WorkGroups) {
				if (wg.WorkSubGroups.size() == 0)
					count++;
				else
					count += (int)wg.WorkSubGroups.size();
			}
			if (count == 0) count = 1;
			if (mousePos.y > GRID_SIZE_V * (1 + row) && mousePos.y < GRID_SIZE_V * (1 + count + row))
			{
				ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), plans[i].Name.c_str());
			}
			else
			{
				ImGui::Text(plans[i].Name.c_str());
			}
			row += count;
		}
		ImGui::PopClipRect();
	}


	{
		ImGui::PushClipRect(ImVec2(canvas_p0.x + GRID_SIZE_H * PLAN_NAME_CELLS, canvas_p0.y + GRID_SIZE_V), canvas_p1, false);
		int row = 0;
		for (int i = 0; i < plans.size(); ++i)
		{
			if (plans[i].WorkGroups.size() == 0)
			{
				row++;
				continue;
			}
			for (int j = 0; j < plans[i].WorkGroups.size(); ++j)
			{
				auto textsize = ImGui::CalcTextSize(plans[i].WorkGroups[j].Name.c_str());
				static double diff_seconds;
				static int diff_days;
				textPos.x = canvas_p0.x + GRID_SIZE_H * PLAN_NAME_CELLS + (PLAN_NAME_CELLS * GRID_SIZE_H - textsize.x) * 0.5f;
				textPos.y = GRID_SIZE_V * 1 + GRID_SIZE_V * row + canvas_p0.y + scrolling.y + (GRID_SIZE_V - textsize.y) * 0.5f;
				ImGui::SetCursorPos(textPos);
				//ImGui::Text(plans[i].WorkGroups[j].Name.c_str());
				int count = (int)plans[i].WorkGroups[j].WorkSubGroups.size();
				if (count == 0) count = 1;
				if (mousePos.y > GRID_SIZE_V * (1 + row) && mousePos.y < GRID_SIZE_V * (1 + count + row))
				{
					ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), plans[i].WorkGroups[j].Name.c_str());
				}
				else
				{
					ImGui::Text(plans[i].WorkGroups[j].Name.c_str());
				}
				row += count;
			}
		}
		ImGui::PopClipRect();
	}

	{
		ImGui::PushClipRect(ImVec2(canvas_p0.x + GRID_SIZE_H * (PLAN_NAME_CELLS + TASK_GROUP_CELLS), canvas_p0.y + GRID_SIZE_V), canvas_p1, false);
		int row = 0;
		for (int i = 0; i < plans.size(); ++i)
		{
			if (plans[i].WorkGroups.size() == 0)
			{
				row++;
				continue;
			}
			for (int j = 0; j < plans[i].WorkGroups.size(); ++j)
			{
				if (plans[i].WorkGroups[j].WorkSubGroups.size() == 0)
				{
					row++;
					continue;
				}
				for (int k = 0; k < plans[i].WorkGroups[j].WorkSubGroups.size(); ++k)
				{
					auto textsize = ImGui::CalcTextSize(plans[i].WorkGroups[j].WorkSubGroups[k].Name.c_str());
					static double diff_seconds;
					static int diff_days;
					textPos.x = canvas_p0.x + GRID_SIZE_H * (PLAN_NAME_CELLS + TASK_GROUP_CELLS) + (GRID_SIZE_H - textsize.x) * 0.5f;
					textPos.y = GRID_SIZE_V * 1 + GRID_SIZE_V * row + canvas_p0.y + scrolling.y + (GRID_SIZE_V - textsize.y) * 0.5f;
					ImGui::SetCursorPos(textPos);
					//ImGui::Text(plans[i].WorkGroups[j].WorkSubGroups[k].Name.c_str());

					if (mousePos.y > GRID_SIZE_V * (1 + row) && mousePos.y < GRID_SIZE_V * (2 + row))
					{
						ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), plans[i].WorkGroups[j].WorkSubGroups[k].Name.c_str());
					}
					else
					{
						ImGui::Text(plans[i].WorkGroups[j].WorkSubGroups[k].Name.c_str());
					}
					++row;
				}
			}
		}
		ImGui::PopClipRect();
	}

	{
		ImGui::PushClipRect(ImVec2(canvas_p0.x + GRID_SIZE_H * (PLAN_NAME_CELLS + TASK_GROUP_CELLS + TASK_SUBGROUP_CELLS), canvas_p0.y + GRID_SIZE_V), canvas_p1, false);
		int row = 0;
		for (int i = 0; i < plans.size(); ++i)
		{
			if (plans[i].WorkGroups.size() == 0)
			{
				row++;
				continue;
			}
			for (int j = 0; j < plans[i].WorkGroups.size(); ++j)
			{
				if (plans[i].WorkGroups[j].WorkSubGroups.size() == 0)
				{
					row++;
					continue;
				}
				//for (int k = 0; k < plans[i].WorkGroups[j].DayWorks.size(); ++k)
				//{
				//	auto textsize = ImGui::CalcTextSize(plans[i].WorkGroups[j].DayWorks[k].Person.c_str());
				//	static double diff_seconds;
				//	static int diff_days;
				//	diff_seconds = difftime(plans[i].WorkGroups[j].DayWorks[k].Date, basetime);
				//	diff_days = (int)(diff_seconds / (60 * 60 * 24));
				//	textPos.x = canvas_p0.x + scrolling.x + grid_size * diff_days + (grid_size - textsize.x) * 0.5f + canvas_width * 0.5f;
				//	textPos.y = grid_size * 1 + grid_size * i + canvas_p0.y + scrolling.y + (grid_size - textsize.y) * 0.5f;
				//	ImGui::SetCursorPos(textPos);
				//	int x0 = canvas_p0.x + scrolling.x + grid_size * diff_days + 3;
				//	int y0 = grid_size * 1 + grid_size * i + canvas_p0.y + scrolling.y + 3;
				//	int x1 = x0 + grid_size - 5;
				//	int y1 = y0 + grid_size - 5;
				//	//draw_list->AddRectFilled(ImVec2(x0,y0), ImVec2(x1,y1), IM_COL32(100, 100, 100, 255), 8);
				//	ImGui::Text(plans[i].WorkGroups[j].DayWorks[k].Person.c_str());
				//}
				//if (plans[i].WorkGroups[j].DayWorks.size() == 0) continue;

				for (int k = 0; k < plans[i].WorkGroups[j].WorkSubGroups.size(); ++k)
				{
					if (plans[i].WorkGroups[j].WorkSubGroups[k].DayWorks.size() == 0)
					{
						row++;
						continue;
					}
					for (int l = 0; l < plans[i].WorkGroups[j].WorkSubGroups[k].DayWorks.size(); ++l)
					{
						auto& daywork = plans[i].WorkGroups[j].WorkSubGroups[k].DayWorks[l];
						auto textsize = ImGui::CalcTextSize(daywork.Person.c_str());
						double diff_seconds = difftime(daywork.Date, basetime);
						int diff_days = (int)(diff_seconds / (60 * 60 * 24));

						// 拖拽集合成员:被拖的 DayWork 本身,或 Shift 组拖拽时与它日期相邻的 DayWork
						const bool is_drag_anchor = dragging_daywork && i == drag_i && j == drag_j && k == drag_k && l == drag_l;
						bool in_drag_set = is_drag_anchor;
						if (!in_drag_set && dragging_daywork && i == drag_i && j == drag_j && k == drag_k)
							for (int m : drag_group_ls)
								if (m == l) { in_drag_set = true; break; }
						const int drag_days = in_drag_set ? drag_offset_days : 0;
						// 拖拽预览绘制在目标行(吸附后的任务行),非拖拽时就是自身所在行
						const int cell_row = (in_drag_set && drag_preview_row >= 0) ? drag_preview_row : row;

						float cell_x = canvas_p0.x + scrolling.x + GRID_SIZE_H * (diff_days + drag_days + PLAN_NAME_CELLS + TASK_GROUP_CELLS + TASK_SUBGROUP_CELLS) + half_canvas_width;
						float cell_y = GRID_SIZE_V * 1 + GRID_SIZE_V * cell_row + canvas_p0.y + scrolling.y;
						float x0 = cell_x + 3;
						float y0 = cell_y + 3;
						float x1 = cell_x + GRID_SIZE_H - 5;
						float y1 = cell_y + GRID_SIZE_V - 5;

						// 命中测试:鼠标(绝对坐标)是否位于该 DayWork 单元格内
						const bool cell_hovered = io.MousePos.x >= cell_x && io.MousePos.x < cell_x + GRID_SIZE_H
							&& io.MousePos.y >= cell_y && io.MousePos.y < cell_y + GRID_SIZE_V;
						// 右键按下:开始拖拽该 DayWork
						if (!dragging_daywork && cell_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
						{
							dragging_daywork = true;
							drag_i = i; drag_j = j; drag_k = k; drag_l = l;
							drag_anchor_x = io.MousePos.x;
							drag_anchor_y = io.MousePos.y;
							drag_offset_days = 0;
							drag_offset_rows = 0;
							drag_preview_row = row;
							buildDragGroup(io.KeyShift, i, j, k, l); // Shift 按住时连同相邻日期一起拖
						}

						if (in_drag_set)
						{
							// 原位置画虚影(记录原始行与原始日期)
							if (drag_days != 0 || drag_offset_rows != 0)
							{
								float ghost_x = cell_x - drag_days * GRID_SIZE_H + 3;
								float ghost_y = GRID_SIZE_V * 1 + GRID_SIZE_V * row + canvas_p0.y + scrolling.y + 3;
								draw_list->AddRect(ImVec2(ghost_x, ghost_y), ImVec2(ghost_x + GRID_SIZE_H - 8, ghost_y + GRID_SIZE_V - 5), IM_COL32(255, 255, 0, 90));
							}
							// 组成员高亮(被抓住的那个边框更亮)
							draw_list->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(130, 100, 20, 255), 0);
							draw_list->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(255, 255, 0, is_drag_anchor ? 255 : 160), 0);
							if (is_drag_anchor)
							{
								// 目标行提示带
								if (drag_preview_row >= 0)
									draw_list->AddRectFilled(ImVec2(canvas_p0.x + GRID_SIZE_H * (PLAN_NAME_CELLS + TASK_GROUP_CELLS + TASK_SUBGROUP_CELLS), cell_y), ImVec2(canvas_p1.x, cell_y + GRID_SIZE_V), IM_COL32(255, 255, 0, 25));
								ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
								// 垂直拖动时,在预览单元格右侧显示目标任务名
								if (drag_offset_rows != 0)
								{
									int ti, tj, tk;
									if (drag_preview_row >= 0 && taskRowAt(drag_preview_row, ti, tj, tk))
										draw_list->AddText(ImVec2(x1 + 6, cell_y + (GRID_SIZE_V - ImGui::GetTextLineHeight()) * 0.5f), IM_COL32(255, 255, 0, 220), plans[ti].WorkGroups[tj].WorkSubGroups[tk].Name.c_str());
								}
								// 水平拖动时,在组最左预览单元格上方显示目标日期(组拖拽时为区间)
								if (drag_days != 0)
								{
									char date_buf[64];
									time_t nd0 = drag_group_min_date + (time_t)drag_days * 86400;
									struct tm tm0 = *localtime(&nd0);
									if (drag_group_ls.size() > 1)
									{
										time_t nd1 = drag_group_max_date + (time_t)drag_days * 86400;
										struct tm tm1 = *localtime(&nd1);
										sprintf_s(date_buf, sizeof(date_buf), "%d-%02d-%02d ~ %d-%02d-%02d",
											tm0.tm_year + 1900, tm0.tm_mon + 1, tm0.tm_mday,
											tm1.tm_year + 1900, tm1.tm_mon + 1, tm1.tm_mday);
									}
									else
									{
										sprintf_s(date_buf, sizeof(date_buf), "%d-%02d-%02d", tm0.tm_year + 1900, tm0.tm_mon + 1, tm0.tm_mday);
									}
									float label_x = canvas_p0.x + scrolling.x + GRID_SIZE_H * ((int)(difftime(drag_group_min_date, basetime) / (60 * 60 * 24)) + drag_days + PLAN_NAME_CELLS + TASK_GROUP_CELLS + TASK_SUBGROUP_CELLS) + half_canvas_width + 3;
									float text_y = y0 - 16.0f;
									if (text_y < canvas_p0.y + GRID_SIZE_V + 2.0f) text_y = canvas_p0.y + GRID_SIZE_V + 2.0f;
									draw_list->AddText(ImVec2(label_x, text_y), IM_COL32(255, 255, 0, 255), date_buf);
								}
							}
						}
						else
						{
							draw_list->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(100, 100, 100, 255), 0);
							// 悬停提示可拖拽
							if (cell_hovered && !dragging_daywork)
							{
								draw_list->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(230, 230, 230, 220), 0);
								ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
							}
						}

						textPos.x = cell_x + (GRID_SIZE_H - textsize.x) * 0.5f;
						textPos.y = cell_y + (GRID_SIZE_V - textsize.y) * 0.5f;
						ImGui::SetCursorPos(textPos);
						ImGui::Text(daywork.Person.c_str());
					}
					++row;
				}
			}
		}
		ImGui::PopClipRect();
	}
}
static void showDebugWindow()
{
	ImGui::SetNextWindowPos(ImGui::GetIO().DisplaySize, ImGuiCond_Always, ImVec2(1.0f, 1.0f));
	ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
	static long renderFrames = 0;
	ImGui::PushFont(nullptr, 10.0f);
	ImGui::Text("Frames = %6.ld", renderFrames++);
	ImGui::PopFont();
	ImGui::End();
}
static void mainloop()
{
	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_Always);
	if (ImGui::Begin("Hello, world!", 0, ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
	{
		drawCanvas();
		static ImVec2 cursorPos = {};
		static ImVec2 textPos = {};
		textPos.x = 0;
		textPos.y = cursorPos.y;
		cursorPos = ImGui::GetCursorPos();
		ImGui::PushFont(nullptr, 12.0f);
		ImGui::PushClipRect(ImVec2(canvas_p0.x + GRID_SIZE_H * (PLAN_NAME_CELLS + TASK_GROUP_CELLS + TASK_SUBGROUP_CELLS), canvas_p0.y), canvas_p1, false);

		auto fration = fmod(scrolling.x, GRID_SIZE_H);
		//if (fration > 0)
		//	fration -= GRID_SIZE_H;
		auto startN = (int)floor(- scrolling.x / GRID_SIZE_H);

		int days = (int)ceil((canvas_p1.x - canvas_p0.x) / GRID_SIZE_H) + (fration !=0? 1: 0) - (PLAN_NAME_CELLS + TASK_GROUP_CELLS + TASK_SUBGROUP_CELLS);

		time_t now = time(NULL);
		struct tm* tm_info = localtime(&now);
		tm_info->tm_hour = 0;
		tm_info->tm_min= 0;
		tm_info->tm_sec = 0;
		now = mktime(tm_info);
		tm_info->tm_mday += startN - (int)ceil((float)days / 2);
		static tm* new_tm_info = {};
		static time_t new_time;
		for(int i = 0; i< days; ++i)
		{
			static char buffer[5];
			tm_info->tm_mday++;
			new_time = mktime(tm_info);
			new_tm_info = localtime(&new_time);
			sprintf_s(buffer, 5, "%d\0", new_tm_info->tm_mday);
			auto textsize = ImGui::CalcTextSize(buffer);
			textPos.x = canvas_p0.x + fration + GRID_SIZE_H * (i + PLAN_NAME_CELLS + TASK_GROUP_CELLS + TASK_SUBGROUP_CELLS) + (GRID_SIZE_H - textsize.x) * 0.5f;
			textPos.y = cursorPos.y + (GRID_SIZE_V - textsize.y) * 0.5f;
			ImGui::SetCursorPos(textPos);
			if (new_time == now)
			{
				ImGui::TextColored(ImVec4(1, 0, 0, 1), buffer);
			}
			else
			{
				ImGui::Text(buffer);
			}
			cursorPos.x += GRID_SIZE_H;
		}
		ImGui::PopClipRect();
		drawGanntView(now);
		ImGui::PopFont();
	}
	ImGui::End();
	showDebugWindow();
}

int APIENTRY WinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE hInstPrev, _In_ PSTR cmdline, _In_ int cmdshow)
{
	main_imgui("Hello World!", mainloop);
	return 0;
}

