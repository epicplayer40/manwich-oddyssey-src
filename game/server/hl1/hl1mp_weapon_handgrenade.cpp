//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		Hand grenade
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "NPCEvent.h"
//#include "hl1mp_basecombatweapon_shared.h"
//#include "basecombatcharacter.h"
//#include "AI_BaseNPC.h"
#ifdef CLIENT_DLL
#include "c_basehlplayer.h"
#else
#include "hl2_player.h"
#endif
#include "gamerules.h"
#include "in_buttons.h"
#ifdef CLIENT_DLL
#else
#include "soundent.h"
#include "game.h"
#endif
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#ifdef CLIENT_DLL
#else
#include "hl1/hl1_basegrenade.h"
#endif


#define HANDGRENADE_MODEL "models/w_grenade.mdl"


#ifndef CLIENT_DLL

extern ConVar sk_plr_dmg_grenade;

//-----------------------------------------------------------------------------
// CHandGrenade
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( grenade_hand, CHandGrenade );

BEGIN_DATADESC( CHandGrenade )
	DEFINE_ENTITYFUNC( BounceTouch ),
END_DATADESC()


void CHandGrenade::Spawn( void )
{
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );

	SetModel( HANDGRENADE_MODEL ); 

	UTIL_SetSize( this, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	m_bHasWarnedAI = false;
}


void CHandGrenade::Precache( void )
{
	BaseClass::Precache( );

	PrecacheScriptSound( "Weapon_HandGrenade.GrenadeBounce" );

	PrecacheModel( HANDGRENADE_MODEL );
}


void CHandGrenade::ShootTimed( CBaseCombatCharacter *pOwner, Vector vecVelocity, float flTime )
{
	SetAbsVelocity( vecVelocity );

	SetThrower( pOwner );
	SetOwnerEntity( pOwner );

	SetTouch( &CHandGrenade::BounceTouch );	// Bounce if touched

	m_flDetonateTime = gpGlobals->curtime + flTime;
	SetThink( &CBaseGrenade::TumbleThink );
	SetNextThink( gpGlobals->curtime + 0.1 );
	if ( flTime < 0.1 )
	{
		SetNextThink( gpGlobals->curtime );
		SetAbsVelocity( vec3_origin );
	}

//	SetSequence( SelectWeightedSequence( ACT_GRENADE_TOSS ) );
	SetSequence( 0 );
	m_flPlaybackRate = 1.0;

	SetAbsAngles( QAngle( 0,0,60) );

	AngularImpulse angImpulse;
	angImpulse[0] = random->RandomInt( -200, 200 );
	angImpulse[1] = random->RandomInt( 400, 500 );
	angImpulse[2] = random->RandomInt( -100, 100 );
	ApplyLocalAngularVelocityImpulse( angImpulse );	

	SetGravity( UTIL_ScaleForGravity( 400 ) );	// use a lower gravity for grenades to make them easier to see
	SetFriction( 0.8 );

	SetDamage( sk_plr_dmg_grenade.GetFloat() );
	SetDamageRadius( GetDamage() * 2.5 );
}


void CHandGrenade ::BounceSound( void )
{
	EmitSound( "Weapon_HandGrenade.GrenadeBounce" );
}


void CHandGrenade::BounceTouch( CBaseEntity *pOther )
{
	if ( pOther->IsSolidFlagSet(FSOLID_TRIGGER | FSOLID_VOLUME_CONTENTS) )
		return;

	// don't hit the guy that launched this grenade
	if ( pOther == GetThrower() )
		return;

	// Do a special test for players
	if ( pOther->IsPlayer() )
	{
		// Never hit a player again (we'll explode and fixup anyway)
		SetCollisionGroup( COLLISION_GROUP_DEBRIS );
	}
	// only do damage if we're moving fairly fast
	if ( (pOther->m_takedamage != DAMAGE_NO) && (m_flNextAttack < gpGlobals->curtime && GetAbsVelocity().Length() > 100))
	{
		if ( GetThrower() )
		{
			trace_t tr;
			tr = CBaseEntity::GetTouchTrace( );
			ClearMultiDamage( );
			Vector forward;
			AngleVectors( GetAbsAngles(), &forward );

			CTakeDamageInfo info( this, GetThrower(), 1, DMG_CLUB );
			CalculateMeleeDamageForce( &info, forward, tr.endpos );
			pOther->DispatchTraceAttack( info, forward, &tr ); 
			ApplyMultiDamage();
		}
		m_flNextAttack = gpGlobals->curtime + 1.0; // debounce
	}

	Vector vecTestVelocity;
	// m_vecAngVelocity = Vector (300, 300, 300);

	// this is my heuristic for modulating the grenade velocity because grenades dropped purely vertical
	// or thrown very far tend to slow down too quickly for me to always catch just by testing velocity. 
	// trimming the Z velocity a bit seems to help quite a bit.
	vecTestVelocity = GetAbsVelocity(); 
	vecTestVelocity.z *= 0.45;

	if ( !m_bHasWarnedAI && vecTestVelocity.Length() <= 60 )
	{
		// grenade is moving really slow. It's probably very close to where it will ultimately stop moving. 
		// emit the danger sound.
		
		// register a radius louder than the explosion, so we make sure everyone gets out of the way
		CSoundEnt::InsertSound ( SOUND_DANGER, GetAbsOrigin(), m_flDamage / 0.4, 0.3 );
		m_bHasWarnedAI = TRUE;
	}

	// HACKHACK - On ground isn't always set, so look for ground underneath
	trace_t tr;
	UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() - Vector(0,0,10), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction < 1.0 )
	{
		// add a bit of static friction
//		SetAbsVelocity( GetAbsVelocity() * 0.8 );
		SetSequence( SelectWeightedSequence( ACT_IDLE ) );
		SetAbsAngles( vec3_angle );
	}

	// play bounce sound
	BounceSound();

	m_flPlaybackRate = GetAbsVelocity().Length() / 200.0;
	if (m_flPlaybackRate > 1.0)
		m_flPlaybackRate = 1;
	else if (m_flPlaybackRate < 0.5)
		m_flPlaybackRate = 0;
}

#endif