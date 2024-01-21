#ifndef ENGINE_HOOKS_H
#define ENGINE_HOOKS_H



void CheckEngineChecksum();
bool IsEngineValidChecksum();
void* GetEngineBaseAddress();
void* GetAbsoluteAddress(uint offset);

#endif // !ENGINE_HOOKS_H
