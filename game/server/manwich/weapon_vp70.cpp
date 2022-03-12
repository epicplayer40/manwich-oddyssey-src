//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "basehlcombatweapon.h"
#include "NPCevent.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "player.h"
#include "game.h"
#include "in_buttons.h"
#include "gamestats.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CWeaponSilencedPistol : public CHLSelectFireMachineGun
{
public:
	DECLARE_CLASS( CWeaponSilencedPistol, CHLSelectFireMachineGun );

	CWeaponSilencedPistol();

	DECLARE_SERVERCLASS();

	virtual const Vector& GetBulletSpread(void);

	void			Precache( void );
	void			AddViewKick( void );
	void			Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	bool			Reload( void );
	float			GetFireRate(void);
	float			GetBurstCycleRate(void) { return 0.2f; }

	virtual int	GetMinBurst()
	{
		return 1;
	}

	virtual int	GetMaxBurst()
	{
		return 1;
	}

//	void PrimaryAttack(void); 
	void SecondaryAttack(void);	
	
	int				CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }



	DECLARE_ACTTABLE();
};

IMPLEMENT_SERVERCLASS_ST(CWeaponSilencedPistol, DT_WeaponSilencedPistol)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_silenced_pistol, CWeaponSilencedPistol );
PRECACHE_WEAPON_REGISTER(weapon_silenced_pistol);

acttable_t	CWeaponSilencedPistol::m_acttable[] =
{
	{ ACT_IDLE,						ACT_IDLE_PISTOL,					true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_PISTOL,				true },
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_PISTOL,			true },
	{ ACT_RELOAD,					ACT_RELOAD_PISTOL,					true },
	{ ACT_WALK_AIM,					ACT_WALK_AIM_PISTOL,				true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_PISTOL,					true },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_PISTOL,	true },
	{ ACT_RELOAD_LOW,				ACT_RELOAD_PISTOL_LOW,				false },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_PISTOL_LOW,		false },
	{ ACT_COVER_LOW,				ACT_COVER_PISTOL_LOW,				false },
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_PISTOL_LOW,			false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_PISTOL,			false },
	{ ACT_WALK,						ACT_WALK_PISTOL,					false },
	{ ACT_RUN,						ACT_RUN_PISTOL,						false },
};

IMPLEMENT_ACTTABLE(CWeaponSilencedPistol);

//=========================================================
CWeaponSilencedPistol::CWeaponSilencedPistol( )
{
	m_fMaxRange1		= 2000;
	m_fMinRange1		= 32;

	m_iFireMode			= FIREMODE_3RNDBURST;
	m_bIsSilenced		= true;

}

void CWeaponSilencedPistol::Precache( void )
{
	BaseClass::Precache();
}

float CWeaponSilencedPistol::GetFireRate(void)
{
	switch (m_iFireMode)
	{
	case FIREMODE_SEMI:
		// the time between rounds fired on semi auto
		return 0.5f;	// 600 rounds per minute = 0.1 seconds per bullet
		break;

	case FIREMODE_3RNDBURST:
		// the time between rounds fired within a single burst
		return 0.033f;
		break;

	default:
		return 0.1f;
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//			*pOperator - 
//-----------------------------------------------------------------------------
void CWeaponSilencedPistol::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
		case EVENT_WEAPON_PISTOL_FIRE:
		{
			Vector vecShootOrigin, vecShootDir;
			vecShootOrigin = pOperator->Weapon_ShootPosition( );

			CAI_BaseNPC *npc = pOperator->MyNPCPointer();
			ASSERT( npc != NULL );
			vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );

			switch (m_iFireMode)
			{
			case FIREMODE_SEMI:
			case FIREMODE_FULLAUTO:
				WeaponSound(SINGLE_NPC);
				pOperator->FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2);
				pOperator->DoMuzzleFlash();
				m_iClip1 = m_iClip1 - 1;
				break;

			case FIREMODE_3RNDBURST:
				WeaponSound(BURST);
				pOperator->FireBullets(3, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2);
				pOperator->DoMuzzleFlash();
				m_iClip1 = m_iClip1 - 3;
				break;
			}
		}
		break;
		default:
			BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	}
}

const Vector &CWeaponSilencedPistol::GetBulletSpread(void)
{
	{
		static Vector cone;

//		if (m_iFireMode = FIREMODE_3RNDBURST)
//		{
			cone = VECTOR_CONE_2DEGREES;
//		}
//		else
//		{
//			cone = VECTOR_CONE_2DEGREES;
//		}

		return cone;
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSilencedPistol::AddViewKick( void )
{
	#define	EASY_DAMPEN			0.5f
	#define	MAX_VERTICAL_KICK	2.0f	//Degrees
	#define	SLIDE_LIMIT			1.0f	//Seconds
	
	//Get the view kick
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (!pPlayer)
		return;

	DoMachineGunKick( pPlayer, EASY_DAMPEN, MAX_VERTICAL_KICK, m_fFireDuration, SLIDE_LIMIT );
}
/*
void CWeaponSilencedPistol::PrimaryAttack(void)
{
	if (m_bFireOnEmpty)
	{
		return;
	}
	switch (m_iFireMode)
	{
	case FIREMODE_SEMI:
		BaseClass::PrimaryAttack();
		// Msg("%.3f\n", m_flNextPrimaryAttack.Get() );
		SetWeaponIdleTime(gpGlobals->curtime + 1.0f);
		break;

	case FIREMODE_3RNDBURST:
		m_iBurstSize = GetBurstSize();

		// Call the think function directly so that the first round gets fired immediately.
		BurstThink();
		SetThink(&CHLSelectFireMachineGun::BurstThink);
		m_flNextPrimaryAttack = gpGlobals->curtime + GetBurstCycleRate();
		m_flNextSecondaryAttack = gpGlobals->curtime + GetBurstCycleRate();

		// Pick up the rest of the burst through the think function.
		SetNextThink(gpGlobals->curtime + GetFireRate());
		break;
	}

	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (pOwner)
	{
		m_iPrimaryAttacks++;
		gamestats->Event_WeaponFired(pOwner, true, GetClassname());
	}
}
*/
//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CWeaponSilencedPistol::SecondaryAttack(void)
{
	// change fire modes.

	switch (m_iFireMode)
	{
	case FIREMODE_SEMI:
		//Msg( "Burst\n" );
		m_iFireMode = FIREMODE_3RNDBURST;
		WeaponSound(SPECIAL2);
		break;

	case FIREMODE_3RNDBURST:
		//Msg( "Auto\n" );
		m_iFireMode = FIREMODE_SEMI;
		WeaponSound(SPECIAL1);
		break;
	}

	SendWeaponAnim(GetSecondaryAttackActivity());

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.3;

	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (pOwner)
	{
		m_iSecondaryAttacks++;
		gamestats->Event_WeaponFired(pOwner, false, GetClassname());
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CWeaponSilencedPistol::Reload(void)
{
	bool fRet = DefaultReload(GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD);
	if (fRet)
	{
		WeaponSound(RELOAD);
	}
	return fRet;
}