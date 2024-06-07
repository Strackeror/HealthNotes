// dllmain.cpp : Définit le point d'entrée de l'application DLL.
#include "MinHook.h"
#include "json.hpp"
#include "loader.h"
#include "plugin-utils.h"
#include "signatures.h"
#include <fstream>
#include <mutex>
#include <queue>
#include <windows.h>

static std::queue<std::pair<float, std::string>> messages;
static std::map<void *, std::queue<std::pair<float, std::string>>> monsterMessages;
static std::mutex lock;

using namespace loader;
using namespace plugin;

std::string_view plugin::module_name = "HealthNotes";

const int UENEMY_LEA_VFT_OFFSET = 0x43;
const int CHAT_INSTANCE_OFFSET = 0x4d;

static void *chat_instance = nullptr;
static void (*show_message)(void *, const char *, float, unsigned int, char) = nullptr;

void showMessage(std::string message) { show_message(*(void **)chat_instance, message.c_str(), -1, -1, 0); }

void handleMonsterCreated(void *monster) {
  char *monsterPath = offsetPtr<char>(monster, 0x7741);
  if (monsterPath[2] == '0' || monsterPath[2] == '1') {
    log(INFO, "monster created {} {:p}", monsterPath, monster);
    std::unique_lock l(lock);
    monsterMessages[monster] = messages;
  }
}

void checkHealth(void *monster) {
  void *healthMgr = *offsetPtr<void *>(monster, 0x7670);
  float health = *offsetPtr<float>(healthMgr, 0x64);
  float maxHealth = *offsetPtr<float>(healthMgr, 0x60);

  char *monsterPath = offsetPtr<char>(monster, 0x7741);
  auto &monsterQueue = monsterMessages[monster];
  std::string lastMessage;
  while (!monsterQueue.empty() && health / maxHealth < monsterQueue.front().first) {
    lastMessage = monsterQueue.front().second;
    monsterQueue.pop();
  }
  if (!lastMessage.empty()) {
    log(INFO, "Message: {}", lastMessage);
    showMessage(lastMessage);
  }
}

byte *get_lea_addr(byte *addr) {
  auto base_addr = addr + 7; // size of call instruction
  auto offset = *reinterpret_cast<int *>(addr + 3);
  return base_addr + offset;
}

__declspec(dllexport) extern bool Load() {
  std::ifstream file("nativePC/plugins/HealthNotes.json");
  if (file.fail()) {
    log(ERR, "Error: config file not found");
    return false;
  }

  LOG(INFO) << "Health notes loading";
  nlohmann::json ConfigFile = nlohmann::json::object();
  file >> ConfigFile;
  for (auto obj : ConfigFile["messages"]) {
    messages.push({obj["ratio"], obj["msg"]});
  }

  auto uenemy_ctor_addr = find_func(sig::monster_ctor);
  auto uenemy_reset_addr = find_func(sig::monster_reset);
  auto show_message_addr = find_func(sig::show_message);
  auto message_instance_addr = find_func(sig::chat_instance_source);
  if (!uenemy_ctor_addr || !show_message_addr || !message_instance_addr || !uenemy_reset_addr)
    return false;

  show_message = reinterpret_cast<decltype(show_message)>(*show_message_addr);
  chat_instance = get_lea_addr(*message_instance_addr + CHAT_INSTANCE_OFFSET);
  log(INFO, "chat_instance:{:p}", chat_instance);

  MH_Initialize();

  Hook<void *(void *, int, int)>::hook(*uenemy_ctor_addr, [](auto orig, auto this_, auto id, auto subid) {
    auto ret = orig(this_, id, subid);
    handleMonsterCreated(this_);
    return ret;
  });
  Hook<void *(void *)>::hook(*uenemy_reset_addr, [](auto orig, auto this_) {
    {
      std::unique_lock l(lock);
      if (monsterMessages.erase(this_)) {
        log(INFO, "Monster {:p} removed", this_);
      }
    }
    return orig(this_);
  });

  std::thread([]() {
    while (true) {
      std::this_thread::sleep_for(std::chrono::seconds(2));
      {
        std::unique_lock l(lock);
        for (auto [monster, queue] : monsterMessages) {
          checkHealth(monster);
        }
      }
    }
  }).detach();

  MH_ApplyQueued();

  return true;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
  if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    return Load();
  return TRUE;
}
