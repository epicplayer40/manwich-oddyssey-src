#ifndef ENGINE_HOOKS_H
#define ENGINE_HOOKS_H



void CheckEngineChecksum();
bool IsEngineValidChecksum();
bool IsGameUIValidChecksum();
void* GetEngineBaseAddress();
void* GetGameUIBaseAddress();
void* GetAbsoluteAddress(void* baseAddress, uint offset);

bool PatchMemory(void* address, size_t length, byte newByte);

#endif // !ENGINE_HOOKS_H
