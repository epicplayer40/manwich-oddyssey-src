#include "cbase.h"
#include "hl2_player.h"
#include "basecombatweapon.h"
#include "gamerules.h"
#include "items.h"
#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CItemVest : public CItem
{
public:
	DECLARE_CLASS(CItemVest, CItem);

	void Spawn(void)
	{
		Precache();
		// FIXME: Change
		SetModel("models/items/vest.mdl");
		BaseClass::Spawn();
	}
	void Precache(void)
	{
		// FIXME: Change
		PrecacheModel("models/items/vest.mdl");

		PrecacheScriptSound("ItemVest.Touch");

	}
	bool MyTouch(CBasePlayer* pPlayer)
	{
		CHL2_Player* pHL2Player = dynamic_cast<CHL2_Player*>(pPlayer);
		return (pHL2Player && pHL2Player->ApplyVest());
	}
};

LINK_ENTITY_TO_CLASS(item_vest, CItemVest);
PRECACHE_REGISTER(item_vest);
