// Lychy!!! :D

#include "cbase.h"
#include "effect_dispatch_data.h"
#include "te_effect_dispatch.h"
#include "vehicle_apc.h"

ConVar	sk_apc_conscript_damagetype("sk_apc_conscript_damagetype", "Largeround");
// The conscript APC, inherits from normal APC, but with few stylistic changes


class CPropAPCConscript : public CPropAPC
{
	DECLARE_CLASS(CPropAPCConscript, CPropAPC);

	// Muzzle flashes
	const char* GetTracerType(void);
	void			DoImpactEffect(trace_t& tr, int nDamageType);
	void			DoMuzzleFlash(void);

	const char* GetBulletType() const;
	const char* GetFireMachineGunSound() const;
};

LINK_ENTITY_TO_CLASS(prop_vehicle_apc_conscript, CPropAPCConscript);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char* CPropAPCConscript::GetTracerType(void)
{
	return "Tracer";
}


//-----------------------------------------------------------------------------
// Allows the shooter to change the impact effect of his bullets
//-----------------------------------------------------------------------------
void CPropAPCConscript::DoImpactEffect(trace_t& tr, int nDamageType)
{
	CPropVehicleDriveable::DoImpactEffect(tr, nDamageType);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAPCConscript::DoMuzzleFlash(void)
{
	CEffectData data;
	data.m_nEntIndex = entindex();
	data.m_nAttachmentIndex = m_nMachineGunMuzzleAttachment;
	data.m_flScale = 1.0f;
	DispatchEffect("MuzzleFlash", data);
}

const char* CPropAPCConscript::GetBulletType() const
{
	return sk_apc_conscript_damagetype.GetString();
}

const char* CPropAPCConscript::GetFireMachineGunSound() const
{
	return "Weapon_AR2.NPC_Single";
}