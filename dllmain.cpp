// dllmain.cpp : Définit le point d'entrée de l'application DLL.
#include <fstream>
#include <queue>

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
	LOG(INFO) << SHOW(monsterPath);
	if (monsterPath[2] == '0' || monsterPath[2] == '1') {
		if (monsterMessages.find(monster) == monsterMessages.end()) {
			monsterMessages[monster] = messages;
		}

		auto& monsterQueue = monsterMessages[monster];
		if (!monsterQueue.empty()) {
			if (health / maxHealth < monsterQueue.front().first) {
				showMessage(monsterQueue.front().second);
				monsterQueue.pop();
			}
		}
	}
}

CreateHook(LaunchAction, MH::Monster_LaunchAction, 
	bool, void* monster, int actionId) {
	checkHealth(monster);
	return originalLaunchAction(monster, actionId);
}

static bool Load()
{
	if (std::string(GameVersion) != "404549") {
		LOG(ERR) << "Health Notes : Wrong version";
		return false;
	}

	std::ifstream file("nativePC/plugins/HealthNotes.json");
	if (file.fail()) {
		LOG(ERR) << "Config file not found";
		return false;
	}

	nlohmann::json ConfigFile = nlohmann::json::object();
	file >> ConfigFile;

	for (auto obj : ConfigFile["messages"])
	{
		messages.push({ obj["ratio"], obj["msg"] });
	}


	MH_Initialize();
	
	QueueHook(LaunchAction);

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

