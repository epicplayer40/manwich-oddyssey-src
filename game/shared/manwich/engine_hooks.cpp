#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Psapi.h>
#endif // WIN32

#include "dbg.h"
#define ENGINE_SIZE 0x0069b000

bool g_bIsEngineChecksumValid = false;

void CheckEngineChecksum()
{
#ifdef WIN32
	//TODO: Do a proper checksum
	HMODULE module = GetModuleHandle("engine.dll");
	MODULEINFO mInfo = { 0 };
	GetModuleInformation(GetCurrentProcess(), module, &mInfo, sizeof(mInfo));
	g_bIsEngineChecksumValid = mInfo.SizeOfImage == ENGINE_SIZE;
#endif // WIN32
}
bool IsEngineValidChecksum()
{
	return g_bIsEngineChecksumValid;
}
void* GetEngineBaseAddress()
{
#ifdef WIN32
	if (IsEngineValidChecksum())
	{
		return GetModuleHandle("engine.dll");
	}
	else
#endif // WIN32
	{
		Assert(0);
		return 0;
	}
}

void* GetAbsoluteAddress(uint offset)
{
	void* baseAddress = GetEngineBaseAddress();
	if (baseAddress == NULL)
	{
		Assert(0);
		return NULL;
	}
	return (void*)((uint)baseAddress + offset);
}