//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "basehlcombatweapon.h"
#include "basecombatcharacter.h"
#include "movie_explosion.h"
#include "soundent.h"
#include "player.h"
#include "rope.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "explode.h"
#include "util.h"
#include "in_buttons.h"
#include "weapon_ml.h"
#include "shake.h"
#include "ai_basenpc.h"
#include "ai_squad.h"
#include "te_effect_dispatch.h"
#include "triggers.h"
#include "smoke_trail.h"
#include "collisionutils.h"
#include "hl2_shareddefs.h"
#include "rumble_shared.h"
#include "gamestats.h"
//#include "player_missile.h"

#ifdef HL2_DLL
	extern int g_interactionPlayerLaunchedML;
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	ML_SPEED	1500

//=============================================================================
// RPG
//=============================================================================

BEGIN_DATADESC( CWeaponMissileLauncher )

	DEFINE_FIELD( m_bInitialStateUpdate,FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bGuiding,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_vecNPCLaserDot,		FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_hLaserMuzzleSprite, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hLaserBeam,			FIELD_EHANDLE ),
	DEFINE_FIELD( m_bHideGuiding,		FIELD_BOOLEAN ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST(CWeaponMissileLauncher, DT_WeaponMissileLauncher)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_ml, CWeaponMissileLauncher );
PRECACHE_WEAPON_REGISTER(weapon_ml);

acttable_t	CWeaponMissileLauncher::m_acttable[] = 
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_RPG, true },

	{ ACT_IDLE_RELAXED,				ACT_IDLE_RPG_RELAXED,			true },
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_ANGRY_RPG,				true },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_RPG,				true },

	{ ACT_IDLE,						ACT_IDLE_RPG,					true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_RPG,				true },
	{ ACT_WALK,						ACT_WALK_RPG,					true },
	{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RPG,			true },
	{ ACT_RUN,						ACT_RUN_RPG,					true },
	{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RPG,				true },
	{ ACT_COVER_LOW,				ACT_COVER_LOW_RPG,				true },
};

IMPLEMENT_ACTTABLE(CWeaponMissileLauncher);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWeaponMissileLauncher::CWeaponMissileLauncher()
{
	m_bReloadsSingly = true;
	m_bInitialStateUpdate= false;
	m_bHideGuiding = false;
	m_bGuiding = false;

	m_fMinRange1 = m_fMinRange2 = 40*12;
	m_fMaxRange1 = m_fMaxRange2 = 500*12;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWeaponMissileLauncher::~CWeaponMissileLauncher()
{

	if ( m_hLaserMuzzleSprite )
	{
		UTIL_Remove( m_hLaserMuzzleSprite );
	}

	if ( m_hLaserBeam )
	{
		UTIL_Remove( m_hLaserBeam );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMissileLauncher::Precache( void )
{
	BaseClass::Precache();

	PrecacheScriptSound( "Missile.Ignite" );
	PrecacheScriptSound( "Missile.Accelerate" );

	// Laser dot...
	PrecacheModel( "sprites/redglow1.vmt" );

	UTIL_PrecacheOther( "rpg_missile" );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMissileLauncher::Activate( void )
{
	BaseClass::Activate();

	// Restore the laser pointer after transition
	if ( m_bGuiding )
	{
		CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
		
		if ( pOwner == NULL )
			return;

		if ( pOwner->GetActiveWeapon() == this )
		{
			StartGuiding();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//			*pOperator - 
//-----------------------------------------------------------------------------
void CWeaponMissileLauncher::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
		case EVENT_WEAPON_SMG1:
		{
			if ( m_hMissile != NULL )
				return;

			Vector	muzzlePoint;
			QAngle	vecAngles;

			muzzlePoint = GetOwner()->Weapon_ShootPosition();

			CAI_BaseNPC *npc = pOperator->MyNPCPointer();
			ASSERT( npc != NULL );

			Vector vecShootDir = npc->GetActualShootTrajectory( muzzlePoint );

			// look for a better launch location
			Vector altLaunchPoint;
			if (GetAttachment( "missile", altLaunchPoint ))
			{
				// check to see if it's relativly free
				trace_t tr;
				AI_TraceHull( altLaunchPoint, altLaunchPoint + vecShootDir * (10.0f*12.0f), Vector( -24, -24, -24 ), Vector( 24, 24, 24 ), MASK_NPCSOLID, NULL, &tr );

				if( tr.fraction == 1.0)
				{
					muzzlePoint = altLaunchPoint;
				}
			}

			VectorAngles( vecShootDir, vecAngles );

//			m_hMissile = new CPlayer_Missile::Spawn( muzzlePoint, vecAngles, GetOwner()->edict() );		
	//		m_hMissile->m_hOwner = this;
		//	m_hMissile->SetDamage(APC_MISSILE_DAMAGE);

			// NPCs always get a grace period
			//m_hMissile->SetGracePeriod( 0.5 );

			pOperator->DoMuzzleFlash();

			WeaponSound( SINGLE_NPC );

			// Make sure our laserdot is off
			m_bGuiding = false;
		}
		break;

		default:
			BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponMissileLauncher::HasAnyAmmo( void )
{
	if ( m_hMissile != NULL )
		return true;

	return BaseClass::HasAnyAmmo();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponMissileLauncher::WeaponShouldBeLowered( void )
{
	// Lower us if we're out of ammo
	if ( !HasAnyAmmo() )
		return true;
	
	return BaseClass::WeaponShouldBeLowered();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMissileLauncher::PrimaryAttack( void )
{
	// Can't have an active missile out
	if ( m_hMissile != NULL )
		return;

	// Can't be reloading
	if ( GetActivity() == ACT_VM_RELOAD )
		return;

	Vector vecOrigin;
	Vector vecForward;

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.5f;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	Vector	vForward, vRight, vUp;

	pOwner->EyeVectors( &vForward, &vRight, &vUp );

	Vector	muzzlePoint = pOwner->Weapon_ShootPosition() + vForward * 12.0f + vRight * 6.0f + vUp * -3.0f;

	QAngle vecAngles;
	VectorAngles( vForward, vecAngles );
//	m_hMissile = CPlayer_Missile::Create("missile", muzzlePoint, vecAngles, GetOwner());
	m_hMissile = (CBaseAnimating*)CreateEntityByName("player_missile");

//	m_hMissile->m_hOwner = this;
	m_hMissile->SetOwnerEntity(this);
	m_hMissile->Spawn();
//	pBattery->m_vSpawnPos = muzzlePoint;
//	pBattery->m_vSpawnAng = vecAngles;

	// If the shot is clear to the player, give the missile a grace period
	trace_t	tr;
	Vector vecEye = pOwner->EyePosition();
	UTIL_TraceLine( vecEye, vecEye + vForward * 128, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction == 1.0 )
	{
//		m_hMissile->SetGracePeriod( 0.3 );
	}

	DecrementAmmo( GetOwner() );

	// Register a muzzleflash for the AI
	pOwner->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	WeaponSound( SINGLE );

	pOwner->RumbleEffect( RUMBLE_SHOTGUN_SINGLE, 0, RUMBLE_FLAG_RESTART );

	m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired( pOwner, true, GetClassname() );

	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 1000, 0.2, GetOwner(), SOUNDENT_CHANNEL_WEAPON );

	// Check to see if we should trigger any RPG firing triggers
	int iCount = g_hWeaponFireTriggers.Count();
	for ( int i = 0; i < iCount; i++ )
	{
		if ( g_hWeaponFireTriggers[i]->IsTouching( pOwner ) )
		{
			if ( FClassnameIs( g_hWeaponFireTriggers[i], "trigger_rpgfire" ) )
			{
				g_hWeaponFireTriggers[i]->ActivateMultiTrigger( pOwner );
			}
		}
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOwner - 
//-----------------------------------------------------------------------------
void CWeaponMissileLauncher::DecrementAmmo( CBaseCombatCharacter *pOwner )
{
	// Take away our primary ammo type
	pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : state - 
//-----------------------------------------------------------------------------
void CWeaponMissileLauncher::SuppressGuiding( bool state )
{
	m_bHideGuiding = state;
}

//-----------------------------------------------------------------------------
// Purpose: Override this if we're guiding a missile currently
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponMissileLauncher::Lower( void )
{
	if ( m_hMissile != NULL )
		return false;

	return BaseClass::Lower();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMissileLauncher::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	//If we're pulling the weapon out for the first time, wait to draw the laser
	if ( ( m_bInitialStateUpdate ) && ( GetActivity() != ACT_VM_DRAW ) )
	{
		m_bInitialStateUpdate = false;
	}

	// Supress our guiding effects if we're lowered
	if ( GetIdealActivity() == ACT_VM_IDLE_LOWERED || GetIdealActivity() == ACT_VM_RELOAD )
	{
		SuppressGuiding();
	}
	else
	{
		SuppressGuiding( false );
	}

	//Player has toggled guidance state

		if ( pPlayer->m_afButtonPressed & IN_ATTACK2 )
		{
			ToggleGuiding();
		}


}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true if the rocket is being guided, false if it's dumb
//-----------------------------------------------------------------------------
bool CWeaponMissileLauncher::IsGuiding( void )
{
	return m_bGuiding;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponMissileLauncher::Deploy( void )
{
	m_bInitialStateUpdate = true;

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponMissileLauncher::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	//Can't have an active missile out
	if ( m_hMissile != NULL )
		return false;

	StopGuiding();
	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: Turn on the guiding laser
//-----------------------------------------------------------------------------
void CWeaponMissileLauncher::StartGuiding( void )
{
	// Don't start back up if we're overriding this
	if ( m_bHideGuiding )
		return;

	m_bGuiding = true;

	WeaponSound(SPECIAL1);

}

//-----------------------------------------------------------------------------
// Purpose: Turn off the guiding laser
//-----------------------------------------------------------------------------
void CWeaponMissileLauncher::StopGuiding( void )
{
	m_bGuiding = false;

	WeaponSound( SPECIAL2 );

}

//-----------------------------------------------------------------------------
// Purpose: Toggle the guiding laser
//-----------------------------------------------------------------------------
void CWeaponMissileLauncher::ToggleGuiding( void )
{
	if ( IsGuiding() )
	{
		StopGuiding();
	}
	else
	{
		StartGuiding();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMissileLauncher::Drop( const Vector &vecVelocity )
{
	StopGuiding();

	BaseClass::Drop( vecVelocity );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMissileLauncher::NotifyRocketDied( void )
{
	m_hMissile = NULL;

	Reload();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponMissileLauncher::Reload( void )
{
	CBaseCombatCharacter *pOwner = GetOwner();
	
	if ( pOwner == NULL )
		return false;

	if ( pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0 )
		return false;

	WeaponSound( RELOAD );
	
	SendWeaponAnim( ACT_VM_RELOAD );

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CWeaponMissileLauncher::WeaponLOSCondition( const Vector &ownerPos, const Vector &targetPos, bool bSetConditions )
{
	bool bResult = BaseClass::WeaponLOSCondition( ownerPos, targetPos, bSetConditions );

	if( bResult )
	{
		CAI_BaseNPC* npcOwner = GetOwner()->MyNPCPointer();

		if( npcOwner )
		{
			trace_t tr;

			Vector vecRelativeShootPosition;
			VectorSubtract( npcOwner->Weapon_ShootPosition(), npcOwner->GetAbsOrigin(), vecRelativeShootPosition );
			Vector vecMuzzle = ownerPos + vecRelativeShootPosition;
			Vector vecShootDir = npcOwner->GetActualShootTrajectory( vecMuzzle );

			// Make sure I have a good 10 feet of wide clearance in front, or I'll blow my teeth out.
			AI_TraceHull( vecMuzzle, vecMuzzle + vecShootDir * (10.0f*12.0f), Vector( -24, -24, -24 ), Vector( 24, 24, 24 ), MASK_NPCSOLID, NULL, &tr );

			if( tr.fraction != 1.0f )
				bResult = false;
		}
	}

	return bResult;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flDot - 
//			flDist - 
// Output : int
//-----------------------------------------------------------------------------
int CWeaponMissileLauncher::WeaponRangeAttack1Condition( float flDot, float flDist )
{
	if (m_hMissile != NULL)
		return 0;

	// Ignore vertical distance when doing our RPG distance calculations
	CAI_BaseNPC *pNPC = GetOwner()->MyNPCPointer();
	if ( pNPC )
	{
		CBaseEntity *pEnemy = pNPC->GetEnemy();
		Vector vecToTarget = (pEnemy->GetAbsOrigin() - pNPC->GetAbsOrigin());
		vecToTarget.z = 0;
		flDist = vecToTarget.Length();
	}

	if ( flDist < MIN( m_fMinRange1, m_fMinRange2 ) )
		return COND_TOO_CLOSE_TO_ATTACK;

	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return 0;

	// See if there's anyone in the way!
	CAI_BaseNPC *pOwner = GetOwner()->MyNPCPointer();
	ASSERT( pOwner != NULL );

	if( pOwner )
	{
		// Make sure I don't shoot the world!
		trace_t tr;

		Vector vecMuzzle = pOwner->Weapon_ShootPosition();
		Vector vecShootDir = pOwner->GetActualShootTrajectory( vecMuzzle );

		// Make sure I have a good 10 feet of wide clearance in front, or I'll blow my teeth out.
		AI_TraceHull( vecMuzzle, vecMuzzle + vecShootDir * (10.0f*12.0f), Vector( -24, -24, -24 ), Vector( 24, 24, 24 ), MASK_NPCSOLID, NULL, &tr );

		if( tr.fraction != 1.0 )
		{
			return COND_WEAPON_SIGHT_OCCLUDED;
		}
	}

	return COND_CAN_RANGE_ATTACK1;
}