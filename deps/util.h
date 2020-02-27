#pragma once

template<typename T>
inline T* offsetPtr(void* ptr, int offset) { return (T*)(((char*)ptr) + offset); }

#define SHOW(VAR) "\""#VAR"\"={" << VAR << "}";

#define CreateHook(NAME, ADDR, RET, ...) \
	static void* NAME ## address = (void*) ADDR; \
	typedef RET (__fastcall*  t ## NAME)( __VA_ARGS__); \
	t##NAME original##NAME; \
	RET __fastcall NAME(__VA_ARGS__)

#define QueueHook(NAME) \
 do {\
	MH_CreateHook((void*)NAME ## address, & NAME, (LPVOID *)& original##NAME);\
	MH_QueueEnableHook((void *)NAME ## address);\
} while(0) 