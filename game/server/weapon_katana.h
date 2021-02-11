//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef WEAPON_KATANA_H
#define WEAPON_KATANA_H

#include "basebludgeonweapon.h"

#if defined( _WIN32 )
#pragma once
#endif

#ifdef HL2MP
#error weapon_katana.h must not be included in hl2mp. The windows compiler will use the wrong class elsewhere if it is.
#endif

#define	KATANA_RANGE	75.0f
#define	KATANA_REFIRE	0.4f

//-----------------------------------------------------------------------------
// CWeaponKatana
//-----------------------------------------------------------------------------

class CWeaponKatana : public CBaseHLBludgeonWeapon
{
public:
	DECLARE_CLASS( CWeaponKatana, CBaseHLBludgeonWeapon );

	DECLARE_SERVERCLASS();
	DECLARE_ACTTABLE();

	CWeaponKatana();

	float		GetRange( void )		{	return	KATANA_RANGE;	}
	float		GetFireRate( void )		{	return	KATANA_REFIRE;	}

	void		AddViewKick( void );
	float		GetDamageForActivity( Activity hitActivity );

	virtual int WeaponMeleeAttack1Condition( float flDot, float flDist );
	void		SecondaryAttack( void )	{	return;	}

	// Animation event
	virtual void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

private:
	// Animation event handlers
	void HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
};

#endif // WEAPON_KATANA_H
