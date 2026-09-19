#include "main.h"
#include "gantt_view.h"
#include "sample_data.h"

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
	static std::vector<Task::Plan> plans = makeSamplePlans();
	static GanttView gantt(plans);

	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_Always);
	if (ImGui::Begin("Hello, world!", 0, ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
	{
		ImGui::PushFont(nullptr, 12.0f);
		gantt.Draw();
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
