#pragma once

#include <algorithm>
#include <string>
#include <vector>
#include <windows.h>

#include <Psapi.h>

typedef unsigned char byte;
inline std::vector<byte *> scanmem(const std::vector<byte> &bytes) {
  std::vector<byte *> results;
  auto module = GetModuleHandle("MonsterHunterWorld.exe");
  if (module == nullptr)
    return results;

  MODULEINFO moduleInfo;
  if (!GetModuleInformation(GetCurrentProcess(), module, &moduleInfo, sizeof(moduleInfo)))
    return results;

  byte *startAddr = (byte *)module;
  byte *endAddr = startAddr + moduleInfo.SizeOfImage;
  byte *addr = startAddr;

  while (addr < endAddr) {
    MEMORY_BASIC_INFORMATION memInfo;
    if (!VirtualQuery(addr, &memInfo, sizeof(memInfo)) || memInfo.State != MEM_COMMIT || (memInfo.Protect & PAGE_GUARD))
      continue;
    byte *begin = (byte *)memInfo.BaseAddress;
    byte *end = begin + memInfo.RegionSize;

    byte *found = std::search(begin, end, bytes.begin(), bytes.end());
    while (found != end) {
      results.push_back(found);
      found = std::search(found + 1, end, bytes.begin(), bytes.end());
    }

    addr = end;
    memInfo = {};
  }

  return results;
}

inline std::vector<byte *> scanmem(const std::vector<byte> &bytes, const std::vector<byte> &mask) {
  std::vector<byte *> results;
  auto module = GetModuleHandle("MonsterHunterWorld.exe");
  if (module == nullptr)
    return results;

  MODULEINFO moduleInfo;
  if (!GetModuleInformation(GetCurrentProcess(), module, &moduleInfo, sizeof(moduleInfo)))
    return results;

  byte *startAddr = (byte *)module;
  byte *endAddr = startAddr + moduleInfo.SizeOfImage;
  byte *addr = startAddr;

  std::vector<std::tuple<byte, byte>> maskedBytes(bytes.size());
  for (int i = 0; i < bytes.size(); ++i) {
    maskedBytes[i] = {bytes[i], mask[i]};
  }

  while (addr < endAddr) {
    MEMORY_BASIC_INFORMATION memInfo;
    if (!VirtualQuery(addr, &memInfo, sizeof(memInfo)) || memInfo.State != MEM_COMMIT || (memInfo.Protect & PAGE_GUARD))
      continue;
    byte *begin = (byte *)memInfo.BaseAddress;
    byte *end = begin + memInfo.RegionSize;

    auto predicate = [](byte a, std::tuple<byte, byte> b) {
      auto [check, mask] = b;
      return (a & mask) == (check & mask);
    };

    byte *found = std::search(begin, end, maskedBytes.begin(), maskedBytes.end(), predicate);

    while (found != end) {
      results.push_back(found);
      found = std::search(found + 1, end, maskedBytes.begin(), maskedBytes.end(), predicate);
    }

    addr = end;
    memInfo = {};
  }

  return results;
}

inline std::tuple<std::vector<byte>, std::vector<byte>> parseBinary(std::string_view aob) {
  std::vector<byte> data;
  std::vector<byte> mask;

  byte currentByte = 0;
  byte currentMask = 0;
  int bitsRead = 0;
  for (char c : aob) {
    if (std::string("01.").find(c) == std::string::npos)
      continue;
    bitsRead += 1;
    currentByte = (currentByte << 1) + (c == '1');
    currentMask = (currentMask << 1) + (c != '.');
    if (bitsRead == 8) {
      bitsRead = 0;
      data.push_back(currentByte);
      mask.push_back(currentMask);
      currentByte = 0;
      currentMask = 0;
    }
  }
  return {data, mask};
}
