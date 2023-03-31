#pragma once
#include <stdint.h>
#include <stdio.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

enum {
	HidNpadStyleSet_NpadStandard = 0,
	HidNpadButton_Plus = 1,
};

inline void consoleInit(void*){}
inline void consoleUpdate(void*){ fflush(stdout); }
inline void consoleExit(void*){}
inline bool appletMainLoop() { return false; }
struct PadState {};
inline void padUpdate(PadState*){}
inline u64 padGetButtonsDown(PadState*) { return 0; }
struct ThreadExceptionDump {
	const char* error_desc;
};
inline int appletGetAppletType() { return 0; }
inline void padConfigureInput(u64, u64) {}
inline void padInitializeDefault(PadState*) {}