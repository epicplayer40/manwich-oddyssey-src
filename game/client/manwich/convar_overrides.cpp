#include "cbase.h"
#include "engine_hooks.h"

void PatchGameUI();

void SetupConVarOverrides()
{
	ConVarRef r_radiosityRef("r_radiosity");
	ConVar* r_radiosity = (ConVar*)r_radiosityRef.GetLinkedConVar();
	if (r_radiosity)
	{
		r_radiosity->SetFlags(FCVAR_ARCHIVE);
	}
	PatchGameUI();
}

void PatchGameUI()
{
	// Patch out in GameUI.dll where dsp_enhance_stereo is set to 0 in the audio settings
	if (!IsGameUIValidChecksum())
		return;
	//PatchMemory(GetAbsoluteAddress(GetGameUIBaseAddress(), 0x72aef), 0x72b10 - 0x72aef, 0x90);  // NOP it out
	PatchMemory(GetAbsoluteAddress(GetGameUIBaseAddress(), 0x72aef), 1, 0xEB); // Short jump 33 
	PatchMemory(GetAbsoluteAddress(GetGameUIBaseAddress(), 0x72af0), 1, 33);
	
}