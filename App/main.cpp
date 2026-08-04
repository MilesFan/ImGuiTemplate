#include "../ImGuiScaffoldSDL3GL/imgui/imgui.h"
#include "../ImGuiScaffoldSDL3GL/main_imgui.h"
#include <windows.h>
#pragma comment( lib, "opengl32" )
#pragma comment( lib, "SDL3-static" )
#pragma comment( lib, "Winmm" )
#pragma comment( lib, "Setupapi" )
#pragma comment( lib, "Version" )
void mainloop()
{
	if (ImGui::Begin("Hello, world!"))
	{
		ImGui::Text("This is some useful text.");
		static bool show_about = false;
		if (ImGui::Button("Click me"))
		{
			show_about = true;
		}
		if (show_about)
			ImGui::OpenPopup("About");

		if (ImGui::BeginPopupModal("About", &show_about, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("This is an example of a modal popup.");
			if (ImGui::Button("Close"))
			{
				show_about = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
		ImGui::End();
	}
}

int APIENTRY WinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE hInstPrev, _In_ PSTR cmdline, _In_ int cmdshow)
{
	main_imgui("Hello World!", mainloop);
	return 0;
}
