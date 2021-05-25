#include "cbase.h"
#include "npc_combines.h"
#include "weapon_physcannon.h"
#include "hl2_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define CIGBINE_MODEL "models/Cigbine.mdl"

ConVar	sk_cigbine_health("sk_cigbine_health", "2000");
ConVar sk_cigbine_kick("sk_cigbine_kick", "50");

class CNPC_Cigbine : public CNPC_Combine
{
	DECLARE_CLASS(CNPC_Cigbine, CNPC_Combine);
public:
	void	Spawn(void);
	void	Precache(void);
	void	SelectModel();
	Class_T Classify(void);
};

LINK_ENTITY_TO_CLASS(npc_cigbine, CNPC_Cigbine);

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