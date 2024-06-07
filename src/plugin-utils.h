#pragma once

#include "MinHook.h"
#include "loader.h"
#include "scanmem.h"
#include <format>
#include <functional>
#include <optional>
#include <source_location>
#include <string_view>

struct Signature {
  std::string_view name;
  std::string_view binary;
  int offset = 0;
};

namespace plugin {
extern std::string_view module_name;

template <typename... Args> void log(loader::LogLevel level, std::format_string<Args...> &&fmt, Args... args) {
  if (level >= loader::MinLogLevel) {
    loader::LOG(level) << std::format("[{}]{}", module_name, std::vformat(fmt.get(), std::make_format_args(args...)));
  }
}

inline std::optional<unsigned char *> find_func(Signature sig) {
  using namespace loader;
  auto [bytes, mask] = parseBinary(sig.binary);
  auto found = scanmem(bytes, mask);
  if (found.size() != 1) {
    log(ERR, "failed to find unique function {} found {}", sig.name, found.size());
    return {};
  }
  auto final_addr = found[0] + sig.offset;
  log(INFO, "found function {} at address {:p}", sig.name, (void *)final_addr);
  return final_addr;
}

template <typename T>
inline T* offsetPtr(void* ptr, int offset) {
  return (T*)(((char*)ptr) + offset);
}

consteval int line(std::source_location loc = std::source_location::current()) {
  return loc.line();
}

template <typename F, int Id = 0>
struct Hook {};
template <typename R, typename... A, int Id>
struct Hook<R(A...), Id> {
  using func = R(A...);
  using hook_function = std::function<R(R (*)(A...), A...)>;

  inline static hook_function static_hook = {};
  inline static func* orig = nullptr;
  inline static constexpr int id = Id;

  static R c_hook(A... args) { return static_hook(orig, args...); }

  static int hook(void* addr, hook_function f) {
    static_hook = f;
    auto ret = 0;
    ret = MH_CreateHook(addr, (void*)c_hook, (void**)&orig);
    if (ret != MH_OK) {
      return ret;
    }
    ret = MH_QueueEnableHook(addr);
    if (ret != MH_OK) {
      return ret;
    }
    return 0;
  };
};

} // namespace plugin
