//========= Totally Not Copyright © 2024, Valve Cut Content, All rights reserved. ============//
//Cigbine, Manwich's quirky sidekick for the latter half of the story!
//Based off of the Combine soldier but designed to be an ally, with a few other quirks.
//
//=============================================================================//

#include "cbase.h"
#include "npc_combines.h"
#include "weapon_physcannon.h"
#include "hl2_gamerules.h"
#include "basehlcombatweapon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define CIGBINE_MODEL "models/Cigbine.mdl"

ConVar	sk_cigbine_health("sk_cigbine_health", "2000");
ConVar sk_cigbine_kick("sk_cigbine_kick", "50");

class CNPC_Cigbine : public CNPC_Combine
{
	DECLARE_CLASS(CNPC_Cigbine, CNPC_Combine);
	DECLARE_DATADESC();

public:
	void	Spawn(void);
	void	Precache(void);
	void	SelectModel();
	Class_T Classify(void);
	virtual int	SelectSchedule(void);
	void	PickupWeapon(CBaseCombatWeapon *pWeapon);
	COutputEvent	m_OnWeaponPickup;
};

LINK_ENTITY_TO_CLASS(npc_cigbine, CNPC_Cigbine);

BEGIN_DATADESC(CNPC_Cigbine)

	DEFINE_OUTPUT(m_OnWeaponPickup, "OnWeaponPickup"),

END_DATADESC()


Class_T	CNPC_Cigbine::Classify(void)
{
	return	CLASS_PLAYER_ALLY_VITAL;
}

void CNPC_Cigbine::Spawn(void)
{
	Precache();
	SetModel(STRING(GetModelName()));

	SetHealth(sk_cigbine_health.GetFloat());
	SetMaxHealth(sk_cigbine_health.GetFloat());
	SetKickDamage(sk_cigbine_kick.GetFloat());

	BaseClass::Spawn();

	AddEFlags(EFL_NO_MEGAPHYSCANNON_RAGDOLL | EFL_NO_PHYSCANNON_INTERACTION);

	CapabilitiesAdd(bits_CAP_ANIMATEDFACE);
	CapabilitiesAdd(bits_CAP_MOVE_SHOOT);
	CapabilitiesAdd(bits_CAP_DOORS_GROUP);
	CapabilitiesAdd(bits_CAP_NO_HIT_PLAYER | bits_CAP_NO_HIT_SQUADMATES | bits_CAP_FRIENDLY_DMG_IMMUNE);

	NPCInit();

}

void CNPC_Cigbine::SelectModel()
{
	SetModelName(AllocPooledString(CIGBINE_MODEL));
}

void CNPC_Cigbine::Precache()
{
	SelectModel();
	PrecacheModel(STRING(GetModelName()));
	BaseClass::Precache();
}

int CNPC_Cigbine::SelectSchedule(void)
{
	if (m_NPCState == NPC_STATE_IDLE || m_NPCState == NPC_STATE_ALERT)
	{
		CBaseEntity *pEnemy;

		pEnemy = GetEnemy();

		if (!GetActiveWeapon() && !HasCondition(COND_TASK_FAILED) && Weapon_FindUsable(Vector(500, 500, 500)))
		{
			CBaseHLCombatWeapon *pWeapon = dynamic_cast<CBaseHLCombatWeapon *>(Weapon_FindUsable(Vector(500, 500, 500)));
			if (pWeapon)
			{
				m_flNextWeaponSearchTime = gpGlobals->curtime + 10.0;
				// Now lock the weapon for several seconds while we go to pick it up.
				pWeapon->Lock(10.0, this);
				SetTarget(pWeapon);
				return SCHED_NEW_WEAPON;
			}
		}
	}

	return BaseClass::SelectSchedule();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_Cigbine::PickupWeapon(CBaseCombatWeapon *pWeapon)
{
	BaseClass::PickupWeapon(pWeapon);
//	SpeakIfAllowed(TLK_NEWWEAPON);
	m_OnWeaponPickup.FireOutput(this, this);
}
