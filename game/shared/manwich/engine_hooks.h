#ifndef ENGINE_HOOKS_H
#define ENGINE_HOOKS_H



void CheckEngineChecksum();
bool IsEngineValidChecksum();
bool IsGameUIValidChecksum();
void* GetEngineBaseAddress();
void* GetGameUIBaseAddress();
void* GetAbsoluteAddress(void* baseAddress, uint offset);

#endif // !ENGINE_HOOKS_H
