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

void DumpMonsterActions(undefined* monster)
{
	undefined* actionManager = monster + 0x61c8;
	undefined* actionListPtr = actionManager + 0x68;
	int group = 0;
	int sz = *offsetPtr<int>(actionListPtr, 0x8);
	char* monsterPath = offsetPtr<char>(monster, 0x7741);

	if (monsterPath[2] == '0' || monsterPath[2] == '1') {
		LOG(INFO) << "=====";
		LOG(INFO) << "Actions For" << monsterPath;
		LOG(INFO) << "=====";

		while (sz > 0)
		{
			LOG(INFO) << "Group : " << group << " Size : " << sz;
			void** actionList = *(void***)actionListPtr;
			if (actionList) {
				for (long long i = 1; i < sz; ++i)
				{
					void* action = actionList[i];
					if (action == nullptr) continue;
					char* actionName = *offsetPtr<char*>(action, 0x20);
					LOG(INFO) << "Group:" << group << " ID:" << i << " Name:" << actionName;
				}
			}
			group++;
			actionListPtr += 0x10;
			sz = *offsetPtr<int>(actionListPtr, 0x8);
		}

		LOG(INFO) << "=====";
		LOG(INFO) << "=====";
		LOG(INFO) << "=====";
		LOG(INFO) << "=====";
		LOG(INFO) << "=====";
		LOG(INFO) << "=====";
	}
}

void showMessage(std::string message) {
	MH::Chat::ShowGameMessage(*(undefined**)MH::Chat::MainPtr, &message[0], -1, -1, 0);
}

void handleMonsterCreated(undefined* monster)
{
	char* monsterPath = offsetPtr<char>(monster, 0x7741);
	if (monsterPath[2] == '0' || monsterPath[2] == '1') {
		LOG(INFO) << "Setting up health messages for " << monsterPath;
		monsterMessages[monster] = messages;
	}
}

void checkHealth(void* monster) {
	void* healthMgr = *offsetPtr<void*>(monster, 0x7670);
	float health = *offsetPtr<float>(healthMgr, 0x64);
	float maxHealth = *offsetPtr<float>(healthMgr, 0x60);

	char* monsterPath = offsetPtr<char>(monster, 0x7741);
	auto& monsterQueue = monsterMessages[monster];
	std::string lastMessage;
	while (!monsterQueue.empty() && health / maxHealth < monsterQueue.front().first) {
		lastMessage = monsterQueue.front().second;
		monsterQueue.pop();
	}
	if (!lastMessage.empty()) {
		LOG(INFO) << "Message: " << lastMessage;
		showMessage(lastMessage);
	}
}

CreateHook(MH::Monster::ctor, ConstructMonster, void*, void* this_ptr, unsigned int monster_id, unsigned int variant)
{
	LOG(INFO) << "Creating Monster : " << monster_id << "-" << variant << " @0x" << this_ptr;
	return original(this_ptr, monster_id, variant);
}

__declspec(dllexport) extern bool Load()
{
	if (std::string(GameVersion) != "410013") {
		LOG(ERR) << "Health Notes : Wrong version";
		return false;
	}

	std::ifstream file("nativePC/plugins/HealthNotes.json");
	if (file.fail()) {
		LOG(ERR) << "Health notes : Config file not found";
		return false;
	}

	LOG(INFO) << "Health notes loading";

	nlohmann::json ConfigFile = nlohmann::json::object();
	file >> ConfigFile;

	for (auto obj : ConfigFile["messages"])
	{
		messages.push({ obj["ratio"], obj["msg"] });
	}


	MH_Initialize();

	HookLambda(MH::Monster::ctor,
		[](auto monster, auto id, auto subId) {
			auto ret = original(monster, id, subId);
			handleMonsterCreated(monster);
			return ret;
		});
	HookLambda(MH::Monster::dtor,
		[](auto monster) {
			LOG(INFO) << "Monster destroyed " << (void*)monster;
			monsterMessages.erase(monster);
			DumpMonsterActions(monster);
			return original(monster);
		});
	HookLambda(MH::Monster::LaunchAction, 
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

