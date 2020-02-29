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


template<typename R, typename ...Args>
void HookFunction(R(*target)(Args...), R(*hook)(Args...), R(**trampoline)(Args...)) {
	MH_CreateHook(target, hook, (LPVOID*)trampoline);
}

#define HookLine(TARGET, LAMBDA) do {\
	static decltype(TARGET) original;\
	HookFunction(TARGET, (decltype(TARGET)) LAMBDA, &original);\
	MH_QueueEnableHook(TARGET);\
} while(false)

