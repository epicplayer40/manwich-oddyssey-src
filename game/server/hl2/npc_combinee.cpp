//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: This is the camo version of the combine
//			
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hl2_gamerules.h"
#include "ai_hull.h"
#include "npc_talker.h"
#include "npc_combinee.h"
#include "ammodef.h"

ConVar	sk_combine_elite_health( "sk_combine_elite_health","120");
ConVar	sk_combine_elite_kick( "sk_combine_elite_kick","4");
ConVar	sk_combine_elite_model( "sk_combine_elite_model", "models/combine_elite.mdl" ); //combine_soldier_prisonguard.mdl by default. Changing to Marijyn for testing purposes

extern ConVar sk_plr_dmg_buckshot;	
extern ConVar sk_plr_num_shotgun_pellets;


LINK_ENTITY_TO_CLASS( npc_combineelite, CNPC_CombineE );

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CNPC_CombineE::Spawn( void )
{
	Precache();
	SetModel( sk_combine_elite_model.GetString() );
	m_iHealth = sk_combine_elite_health.GetFloat();
	m_nKickDamage = sk_combine_elite_kick.GetFloat();

	CapabilitiesAdd( bits_CAP_MOVE_JUMP );
	AddSpawnFlags( SF_NPC_LONG_RANGE  );

	BaseClass::Spawn();
}

Class_T	CNPC_CombineE::Classify ( void )
{
	return	CLASS_COMBINE;
}

bool CNPC_CombineE::IsJumpLegal(const Vector &startPos, const Vector &apex, const Vector &endPos) const
{
	const float MAX_JUMP_RISE		= 440.0f;
	const float MAX_JUMP_DISTANCE	= 1024.0f;
	const float MAX_JUMP_DROP		= 768.0f;

	if ( BaseClass::IsJumpLegal( startPos, apex, endPos, MAX_JUMP_RISE, MAX_JUMP_DROP, MAX_JUMP_DISTANCE ) )
	{
		// Hang onto the jump distance. The AI is going to want it.
		m_flJumpDist = (startPos - endPos).Length();

		return true;
	}
	return false;
}

WeaponProficiency_t CNPC_CombineE::CalcWeaponProficiency( CBaseCombatWeapon *pWeapon )
{
	if( FClassnameIs( pWeapon, "weapon_pistol" ) )
	{
		return WEAPON_PROFICIENCY_PERFECT;
	}

	if( FClassnameIs( pWeapon, "weapon_smg1" ) )
	{
		return WEAPON_PROFICIENCY_VERY_GOOD;
	}

	if( FClassnameIs( pWeapon, "weapon_tommygun" ) )
	{
		return WEAPON_PROFICIENCY_GOOD;
	}

	if( FClassnameIs( pWeapon, "weapon_ar1" ) )
	{
		return WEAPON_PROFICIENCY_VERY_GOOD; 
	}

	if( FClassnameIs( pWeapon, "weapon_dbarrel" ) )
	{
		return WEAPON_PROFICIENCY_PERFECT;
	}

	if( FClassnameIs( pWeapon, "weapon_shotgun" ) )
	{
		return WEAPON_PROFICIENCY_PERFECT;
	}

	else
	{
		return WEAPON_PROFICIENCY_VERY_GOOD;
	}

	return BaseClass::CalcWeaponProficiency( pWeapon );
}


bool CNPC_CombineE::IsHeavyDamage( const CTakeDamageInfo &info )
{
	// Combine considers AR2 fire to be heavy damage
	if ( info.GetAmmoType() == GetAmmoDef()->Index("AR2") )
		return true;

	// 357 rounds are heavy damage
	if ( info.GetAmmoType() == GetAmmoDef()->Index("357") )
		return true;

	// Shotgun blasts where at least half the pellets hit me are heavy damage
	if ( info.GetDamageType() & DMG_BUCKSHOT )
	{
		int iHalfMax = sk_plr_dmg_buckshot.GetFloat() * sk_plr_num_shotgun_pellets.GetInt() * 0.5;
		if ( info.GetDamage() >= iHalfMax )
			return true;
	}

	// Rollermine shocks
	if( (info.GetDamageType() & DMG_SHOCK) && hl2_episodic.GetBool() )
	{
		return true;
	}

	return BaseClass::IsHeavyDamage( info );
}

bool CNPC_CombineE::IsLightDamage( const CTakeDamageInfo &info )
{
	return BaseClass::IsLightDamage( info );
}

void CNPC_CombineE::DeathSound( const CTakeDamageInfo &info )
{
	// NOTE: The response system deals with this at the moment
	if ( GetFlags() & FL_DISSOLVING )
		return;

	GetSentences()->Speak( "COMBINE_DIE", SENTENCE_PRIORITY_INVALID, SENTENCE_CRITERIA_ALWAYS ); 
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_CombineE::Precache()
{
	PrecacheModel( sk_combine_elite_model.GetString() );

	BaseClass::Precache();
}
