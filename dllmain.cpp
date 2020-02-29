// dllmain.cpp : Définit le point d'entrée de l'application DLL.
#include <fstream>
#include <queue>
#include <functional>

#include <windows.h>

#include "minhook/MinHook.h"
#include "json/json.hpp"
#include "loader.h"
#include "ghidra_export.h"
#include "util.h"


static std::queue<std::pair<float, std::string>> messages;
static std::map<void*, std::queue<std::pair<float, std::string>>> monsterMessages;

using namespace loader;

void showMessage(std::string message) {
	MH::Chat::ShowGameMessage(*(undefined**)MH::Chat::MainPtr, &message[0], -1, -1, 0);
}

void checkHealth(void* monster) {
	void* healthMgr = *offsetPtr<void*>(monster, 0x7670);
	float health = *offsetPtr<float>(healthMgr, 0x64);
	float maxHealth = *offsetPtr<float>(healthMgr, 0x60);

	char* monsterPath = offsetPtr<char>(monster, 0x7741);
	if (monsterPath[2] == '0' || monsterPath[2] == '1') {
		if (monsterMessages.find(monster) == monsterMessages.end()) {
			monsterMessages[monster] = messages;
		}
		auto& monsterQueue = monsterMessages[monster];
		std::string lastMessage;
		while (!monsterQueue.empty() && health / maxHealth < monsterQueue.front().first) {
			lastMessage = monsterQueue.front().second;
			monsterQueue.pop();
		}
		if (!lastMessage.empty()) {
			showMessage(lastMessage);
		}
	}
}
static bool Load()
{
	if (std::string(GameVersion) != "404549") {
		LOG(ERR) << "Health Notes : Wrong version";
		return false;
	}

	std::ifstream file("nativePC/plugins/HealthNotes.json");
	if (file.fail()) {
		LOG(ERR) << "Health notes : Config file not found";
		return false;
	}

	nlohmann::json ConfigFile = nlohmann::json::object();
	file >> ConfigFile;

	for (auto obj : ConfigFile["messages"])
	{
		messages.push({ obj["ratio"], obj["msg"] });
	}


	MH_Initialize();

	HookLine(MH::SOBJ::LoadSOBJs,
		[](auto ptr) {
			LOG(INFO) << "New quest load detected, clearing";
			monsterMessages.clear();
			return original(ptr);
		});
	
	HookLine(MH::Monster_LaunchAction, 
		[](auto monster, auto id) {
			checkHealth(monster);
			return original(monster, id);
		});

	MH_ApplyQueued();

	return true;
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
		return Load();
    return TRUE;
}

