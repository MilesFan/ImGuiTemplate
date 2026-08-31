#define IMGUI_DEFINE_MATH_OPERATORS
#include "main.h"
#include "custommath.h"
const float grid_size = 48;
static ImVec2 scrolling(0.0f, 0.0f);
static ImVec2 grid_offsetcells = {};
static ImVec2 canvas_p0 = {};
static ImVec2 canvas_p1 = {};
static Task::GanntView view(make_time(2026, 8, 1), make_time(2026, 8, 30));
static Task::Plan plan1(
	"Plan 1",
	{
		make_time(2026, 8, 1),
		make_time(2026, 8, 2),
		make_time(2026, 8, 3)
	});
static Task::Plan plan2(
	"Plan 2",
	{
		make_time(2026, 8, 10),
		make_time(2026, 8, 11),
		make_time(2026, 8, 12)
	});
static std::vector<Task::Plan> plans = {
	Task::Plan(
	"Plan 1",
	{
		make_time(2026, 8, 1),
		make_time(2026, 8, 2),
		make_time(2026, 8, 3)
	}),
	Task::Plan(
	"Plan 2",
	{
		make_time(2026, 8, 10),
		make_time(2026, 8, 11),
		make_time(2026, 8, 12)
	})
};
static void drawCanvas()
{
	static ImVector<ImVec2> points;
	static bool opt_enable_grid = true;
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
	ImGui::InvisibleButton("canvas", canvas_sz, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
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
		scrolling.x += io.MouseDelta.x;
		scrolling.y += io.MouseDelta.y;
		if (scrolling.y > 0) scrolling.y = 0;
		grid_offsetcells.x = scrolling.x / grid_size;
		grid_offsetcells.y = scrolling.y / grid_size;
	}

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
		}
		ImGui::EndPopup();
	}

	// Draw grid + all lines in the canvas
	draw_list->PushClipRect(canvas_p0, canvas_p1, true);
	if (opt_enable_grid)
	{
		const float GRID_STEP = grid_size;
		for (float x = fmodf(scrolling.x, GRID_STEP); x < canvas_sz.x; x += GRID_STEP)
			draw_list->AddLineV(canvas_p0.x + x, canvas_p0.y, canvas_p1.y, IM_COL32(200, 200, 200, 40));
		for (float y = fmodf(scrolling.y, GRID_STEP); y < canvas_sz.y; y += GRID_STEP)
			draw_list->AddLineH(canvas_p0.x, canvas_p1.x, canvas_p0.y + y, IM_COL32(200, 200, 200, 40));
	}
	//for (int n = 0; n < points.Size; n += 2)
	//	draw_list->AddRect(ImVec2(origin.x + points[n].x, origin.y + points[n].y), ImVec2(origin.x + points[n + 1].x, origin.y + points[n + 1].y), IM_COL32(255, 255, 0, 255), 2.0f);
	draw_list->PopClipRect();
	ImGui::SetCursorPos(cursorPos);
}
static void drawGanntView()
{
	static struct tm* tm_info;
	static ImVec2 textPos = {};
	static char buffer[50];
	ImGui::PushClipRect(ImVec2(canvas_p0.x + grid_size, canvas_p0.y + grid_size), canvas_p1, false);
	for (int i = 0; i < plans.size(); ++i)
	{
		tm_info = localtime(&plans[i].Dates[0]);
		//sprintf_s(buffer, 50, "%s\0", ctime(&plans[i].Dates[0]));
		strftime(buffer, sizeof(buffer), "%Y-%m-%d", tm_info);
		auto textsize = ImGui::CalcTextSize(buffer);
		textPos.x = grid_size * 1 + canvas_p0.x + scrolling.x;
		textPos.y = grid_size * 1 + grid_size * i + canvas_p0.y + scrolling.y + (grid_size - textsize.y) * 0.5f;
		ImGui::SetCursorPos(textPos);
		ImGui::Text(buffer);
	}
	ImGui::PopClipRect();
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
		ImGui::PushFont(nullptr, 10.0f);
		ImGui::PushClipRect(canvas_p0, canvas_p1, false);

		auto fration = fmod(scrolling.x, grid_size);
		if (fration > 0)
			fration -= grid_size;
		auto startN = (int)floor(- scrolling.x / grid_size);

		int days = (int)ceil((canvas_p1.x - canvas_p0.x) / grid_size) + (fration !=0? 1: 0);

		time_t now = time(NULL);
		struct tm* tm_info = localtime(&now);
		tm_info->tm_hour = 0;
		tm_info->tm_min= 0;
		tm_info->tm_sec = 0;
		now = mktime(tm_info);
		tm_info->tm_mday += startN - (days / 2);
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
			textPos.x = canvas_p0.x + fration + grid_size * i + (grid_size - textsize.x) * 0.5f;
			textPos.y = cursorPos.y + (grid_size - textsize.y) * 0.5f;
			ImGui::SetCursorPos(textPos);
			if (new_time == now)
			{
				ImGui::TextColored(ImVec4(1, 0, 0, 1), buffer);
			}
			else
			{
				ImGui::Text(buffer);
			}
			cursorPos.x += grid_size;
		}
		drawGanntView();
		ImGui::PopClipRect();
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

