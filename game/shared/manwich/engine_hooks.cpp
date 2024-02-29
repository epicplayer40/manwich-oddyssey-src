#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Psapi.h>
#endif // WIN32

#include "dbg.h"
#define ENGINE_SIZE 0x0069b000
#define GAMEUI_SIZE 0x00210000

bool g_bIsEngineChecksumValid = false;
bool g_bIsGameUIChecksumValid = false;

void DoCheckSum(const char* moduleName, const DWORD expectedSize, bool& boolResult)
{
#ifdef WIN32
	//TODO: Do a proper checksum
	HMODULE module = GetModuleHandle(moduleName);
	MODULEINFO mInfo = { 0 };
	GetModuleInformation(GetCurrentProcess(), module, &mInfo, sizeof(mInfo));
	boolResult = mInfo.SizeOfImage == expectedSize;
#endif // WIN32
}
void CheckEngineChecksum()
{
	DoCheckSum("engine.dll", ENGINE_SIZE, g_bIsEngineChecksumValid);
	DoCheckSum("GameUI.dll", GAMEUI_SIZE, g_bIsGameUIChecksumValid);
}


bool IsEngineValidChecksum()
{
	return g_bIsEngineChecksumValid;
}

bool IsGameUIValidChecksum()
{
	return g_bIsGameUIChecksumValid;
}

void* GetBaseAddress(const char* moduleName)
{
	#ifdef WIN32
	if (IsEngineValidChecksum())
	{
		return GetModuleHandle(moduleName);
	}
	else
#endif // WIN32
	{
		Assert(0);
		return 0;
	}
}
void* GetEngineBaseAddress()
{
	return GetBaseAddress("engine.dll");
}

void* GetGameUIBaseAddress()
{
	return GetBaseAddress("GameUI.dll");
}

void* GetAbsoluteAddress(void* baseAddress, uint offset)
{
	if (baseAddress == NULL)
	{
		Assert(0);
		return NULL;
	}
	return (void*)((uint)baseAddress + offset);
}