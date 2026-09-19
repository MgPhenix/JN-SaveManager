#include <iostream>

#include "SDL3/SDL.h"
#include "IMGuiManager.h"
#include "imgui_impl_sdlgpu3.h"
#include "imgui_impl_sdl3.h"

#include <filesystem>
#include <fstream>
#include "nlohmann/json.hpp"


using json = nlohmann::json;

static constexpr int windW = 600;
static constexpr int windH = 450;

std::filesystem::path GetJustNatsukiPath()
{
#ifdef _WIN32

	const char* appData = std::getenv("APPDATA");

	if (!appData)
		throw std::runtime_error("APPDATA not found");

	return std::filesystem::path(appData)
		/ "RenPy"
		/ "JustNatsuki";

#elif defined(__linux__)

	const char* home = std::getenv("HOME");

	if (!home)
		throw std::runtime_error("HOME not found");

	return std::filesystem::path(home)
		/ ".renpy"
		/ "JustNatsuki";

#endif
}

#ifdef _DEBUG
	std::filesystem::path save_directory = "../../saves";

	std::filesystem::path json_path = "../../data/data.json";
	std::filesystem::path current_save_path = "../../destination/persistant.txt";
#else
	std::filesystem::path save_directory = "saves";

	std::filesystem::path json_path = "data/data.json";
	std::filesystem::path current_save_path = "destination/persistant.txt";
#endif // DEBUG



void MoveFile(std::filesystem::path path, std::filesystem::path new_path)
{
	if (!std::filesystem::exists(path))
	{
		std::cout << "path does not exist : " << path << std::endl;
		return;
	}
	std::filesystem::copy(path, new_path);
	std::filesystem::remove(path);
}

void RenameFile(const char* path, const char* newName)
{
	std::filesystem::rename(path, newName);
}

void ScanData(json& data)
{
	std::filesystem::path directory(save_directory);

	if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory)) {
		std::cout << "Couldn't open directory : " + directory.string() << std::endl;
		return;
	}

	for (auto& entry : std::filesystem::directory_iterator(directory))
	{
		std::string name = entry.path().stem().string();
		std::string path = entry.path().string();

		if (!data["saves"].contains(name))
			data["saves"][name] = path;
	}

	std::ofstream out(json_path);
	out << data.dump(4);
}

void ChangeToNewSave(json& data, const std::string& new_name)
{
	std::filesystem::path current_save = data.value("current", "Default");
	std::filesystem::path current_path = data["saves"].value(current_save, save_directory / "Default.txt");

	int i = 0;
	while (std::filesystem::exists(current_path))
	{
		current_path = save_directory / ("Default" + std::to_string(i) + ".txt");
		i++;
	}

	MoveFile(current_save_path, current_path.c_str());

	data["current"] = new_name;
	std::filesystem::path new_path = data["saves"].value(new_name, save_directory / "Default.txt");
	MoveFile(new_path.c_str(), current_save_path);

	std::ofstream out(json_path);
	out << data.dump(4);
}

ImFont* AddDefaultFont(float pixel_size)
{
	ImGuiIO& io = ImGui::GetIO();
	ImFontConfig config;
	config.SizePixels = pixel_size;
	config.OversampleH = config.OversampleV = 1;
	config.PixelSnapH = true;
	ImFont* font = io.Fonts->AddFontDefault(&config);
	return font;
}

int main()
{
	std::ifstream file(json_path);
	if (!file.is_open())
		throw std::runtime_error("Couldn't find data.json");

	json data = json::parse(file);

	ScanData(data);

	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;

	ImGuiManager::InitWindow("JN-SaveManager", windW, windH, &window, &renderer, SDL_WINDOW_RESIZABLE);
	ImGuiIO& io = ImGuiManager::GetIO();
	ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = ImVec4(
		0.4f, 0.009f, 0.34f, 0.15f
	);

	ImFont* fontA = AddDefaultFont(30);
	ImFont* fontB = AddDefaultFont(20);

	bool is_selected = false;
	std::string selected = "";

	bool run = true;
	while (run)
	{
		SDL_Event event;

		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_EVENT_QUIT)
				run = false;

			ImGuiManager::ProcessEvent(&event);
		}

		std::string current_save = data.value("current", "Default");

		ImGuiManager::NewFrame();


		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImVec2(windW, windH));

		ImGui::Begin("Main", NULL,
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoDecoration
		);

		ImGui::PushStyleColor(ImGuiCol_HeaderActive, IM_COL32(189, 0, 151, 255));
		ImGui::PushFont(fontA);
		ImGui::NewLine();
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 28, 210, 255));
		ImGui::Text("Just Natsuki Save Manager");
		ImGui::PopStyleColor();
		ImGui::NewLine();
		ImGui::Separator();
		ImGui::NewLine();
		ImGui::PopFont();

		ImGui::PushFont(fontB);
		ImGui::Text(std::string("Current save : " + current_save).c_str()); //ew
		ImGui::SameLine();
		if (is_selected) 
			ImGui::Text(std::string("  |   Selected Save : " + selected).c_str());
		else
			ImGui::NewLine();

		ImGui::NewLine();

		ImGui::Text("Available save : ");

		ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(189, 0, 151, 255));
		ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(189, 0, 151, 255));
		ImGui::BeginChild("Saves", ImVec2(300,200), ImGuiChildFlags_Borders);

		for (auto& [key, value] : data["saves"].items())
		{
			if (ImGui::Button(key.c_str()))
			{
				selected = key;
				is_selected = true;
			}
		}

		ImGui::EndChild();
		ImGui::SameLine();
		if (is_selected)
		{
			if (ImGui::Button("Change Save to selected"))
				ChangeToNewSave(data, selected);
		}

		ImGui::PopStyleColor();
		ImGui::PopStyleColor();
		ImGui::PopStyleColor();
		ImGui::PopFont();
		ImGui::End();

		SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
		SDL_RenderClear(renderer);

		ImGuiManager::Render(renderer);
	}

	ImGuiManager::Quit(window, renderer);

	return 0;
}