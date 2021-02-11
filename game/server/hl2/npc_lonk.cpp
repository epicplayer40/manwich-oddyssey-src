//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: Father Grigori, a benevolent lonk who is the last remaining human
//			in Ravenholm. He keeps to the rooftops and uses a big ole elephant
//			gun to send his zombified former friends to a peaceful death.
//
//=============================================================================

#include "cbase.h"
#include "ai_baseactor.h"
#include "ai_hull.h"
#include "ammodef.h"
#include "gamerules.h"
#include "IEffects.h"
#include "engine/IEngineSound.h"
#include "ai_behavior.h"
#include "ai_behavior_assault.h"
#include "ai_behavior_lead.h"
#include "npcevent.h"
#include "ai_playerally.h"
#include "ai_senses.h"
#include "soundent.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



//-----------------------------------------------------------------------------
// Activities.
//-----------------------------------------------------------------------------
int ACT_LONK_GUN_IDLE;


//-----------------------------------------------------------------------------
// Bodygroups.
//-----------------------------------------------------------------------------
enum
{
	BODYGROUP_MONK_GUN = 1,
};


//-----------------------------------------------------------------------------
// Animation events.
//-----------------------------------------------------------------------------
enum
{
	AE_MONK_FIRE_GUN = 50,
};


class CNPC_Lonk : public CAI_BaseActor
{
	typedef CAI_BaseActor BaseClass;

public:

	CNPC_Lonk() {}
	void Spawn();
	void Precache();

	Class_T	Classify( void );

	float MaxYawSpeed();
	int	TranslateSchedule( int scheduleType );
	int	SelectSchedule ();

	void HandleAnimEvent( animevent_t *pEvent );
	int RangeAttack1Conditions ( float flDot, float flDist );
	Activity NPC_TranslateActivity( Activity eNewActivity );

	void PrescheduleThink();
	void BuildScheduleTestBits( void );

	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );

	bool m_bHasGun;
	int m_nAmmoType;

	DECLARE_DATADESC();

private:

	DEFINE_CUSTOM_AI;
};

BEGIN_DATADESC( CNPC_Lonk )

	DEFINE_KEYFIELD( m_bHasGun, FIELD_BOOLEAN, "HasGun" ),
	DEFINE_FIELD(  m_nAmmoType, FIELD_INTEGER ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( npc_lonk, CNPC_Lonk );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Class_T	CNPC_Lonk::Classify( void )
{
	return CLASS_CITIZEN_REBEL;
}

void CNPC_Lonk::BuildScheduleTestBits( void )
{
		// Don't interrupt while shooting the gun
	const Task_t* pTask = GetTask();
	if ( pTask && (pTask->iTask == TASK_RANGE_ATTACK1) )
	{
		ClearCustomInterruptCondition( COND_HEAVY_DAMAGE );
		ClearCustomInterruptCondition( COND_ENEMY_OCCLUDED );
		ClearCustomInterruptCondition( COND_HEAR_DANGER );
		ClearCustomInterruptCondition( COND_WEAPON_BLOCKED_BY_FRIEND );
		ClearCustomInterruptCondition( COND_WEAPON_SIGHT_OCCLUDED );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Activity CNPC_Lonk::NPC_TranslateActivity( Activity eNewActivity )
{
	if (m_bHasGun)
	{
		//
		// He is carrying Annabelle.
		//
		if (eNewActivity == ACT_IDLE)
		{
			return (Activity)ACT_LONK_GUN_IDLE;
		}

		if ((eNewActivity == ACT_WALK) && (GetActiveWeapon() != NULL))
		{
			// HACK: Run when carrying two weapons. This is for the balcony scene. Remove once we
			//		 can specify movement activities between scene cue points.
			return (Activity)ACT_RUN_RIFLE;
		}

		if((eNewActivity == ACT_RUN))
		{
			return (Activity)ACT_RUN_RIFLE;
		}
	}

	return eNewActivity;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Lonk::Precache()
{
	engine->PrecacheModel( "models/Lonk.mdl" );
	
	BaseClass::Precache();
}
 

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Lonk::Spawn()
{
	Precache();

	BaseClass::Spawn();

	SetModel( "models/Lonk.mdl" );

	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	SetBloodColor( BLOOD_COLOR_RED );
	//m_fEffects			= 0;
	m_iHealth			= 100;
	m_flFieldOfView		= 0.5;// indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState			= NPC_STATE_NONE;

	m_nAmmoType = GetAmmoDef()->Index("357"); //SniperRound by default. A little OP so I changed it to 357 -Stacker

	m_HackedGunPos = Vector ( 0, 0, 55 );

	CapabilitiesAdd( bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP | bits_CAP_MOVE_GROUND | bits_CAP_MOVE_CLIMB | bits_CAP_SQUAD );
//	CapabilitiesAdd( bits_CAP_USE_WEAPONS ); removed in favor of built-in gun
	CapabilitiesAdd( bits_CAP_ANIMATEDFACE );

	m_bHasGun = true;

	if ( m_bHasGun )
	{
		CapabilitiesAdd( bits_CAP_INNATE_RANGE_ATTACK1 );
	}

	SetBodygroup( BODYGROUP_MONK_GUN, m_bHasGun );

		AddSpawnFlags( SF_NPC_LONG_RANGE | SF_NPC_ALWAYSTHINK );
	NPCInit();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flDot - 
//			flDist - 
//-----------------------------------------------------------------------------
int CNPC_Lonk::RangeAttack1Conditions( float flDot, float flDist )
{
	//if ( flDist < 64)
	//{
	//	return COND_TOO_CLOSE_TO_ATTACK;
	//}
	if (flDist > 3072) //range was originally 1024, I've multiplied this by 3
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	else if (flDot < 0.5)
	{
		return COND_NOT_FACING_ATTACK;
	}

	return COND_CAN_RANGE_ATTACK1;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pEvent - 
//-----------------------------------------------------------------------------
void CNPC_Lonk::HandleAnimEvent( animevent_t *pEvent )
{
	switch ( pEvent->event )
	{
		case AE_MONK_FIRE_GUN:
		{
			Vector vecShootOrigin;
			QAngle vecAngles;
			Vector	muzzlePos;
			QAngle	muzzleAngle;
			GetAttachment( "muzzle", vecShootOrigin, vecAngles );

			Vector vecShootDir = GetShootEnemyDir( vecShootOrigin );

			CPASAttenuationFilter filter2( this, "Weapon_DBarrel.Single" );
			EmitSound( filter2, entindex(), "Weapon_DBarrel.Single" );

			UTIL_Smoke( vecShootOrigin, random->RandomInt(40, 80), 10 ); //smokepuff is 20, 30 by default
			FireBullets( 1, vecShootOrigin, vecShootDir, vec3_origin, MAX_TRACE_LENGTH, m_nAmmoType, 0 );
			GetAttachment( "muzzle", muzzlePos, muzzleAngle );
			UTIL_MuzzleFlash( muzzlePos, muzzleAngle, 2.0f, 1 );
			break;
		}

		default:
		{
			BaseClass::HandleAnimEvent( pEvent );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CNPC_Lonk::MaxYawSpeed()
{
	if( IsMoving() )
	{
		return 20;
	}

	switch( GetActivity() )
	{
	case ACT_180_LEFT:
		return 30;
		break;

	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		return 30;
		break;
	default:
		return 15;
		break;
	}
}

//-------------------------------------

int CNPC_Lonk::TranslateSchedule( int scheduleType ) 
{
	return BaseClass::TranslateSchedule( scheduleType );
}


//-------------------------------------

void CNPC_Lonk::PrescheduleThink()
{
	BaseClass::PrescheduleThink();
}	

//-------------------------------------

int CNPC_Lonk::SelectSchedule ()
{
		if( HasCondition( COND_TOO_FAR_TO_ATTACK ) )
		{
			return SCHED_CHASE_ENEMY;
		}

		if( HasCondition( COND_ENEMY_OCCLUDED ) )
		{
			return SCHED_ESTABLISH_LINE_OF_FIRE;
		}

	return BaseClass::SelectSchedule();
}

//-------------------------------------

void CNPC_Lonk::StartTask( const Task_t *pTask )
{
	BaseClass::StartTask( pTask );
}


void CNPC_Lonk::RunTask( const Task_t *pTask )
{
	BaseClass::RunTask( pTask );
}


//-----------------------------------------------------------------------------
//
// CNPC_Lonk Schedules
//
//-----------------------------------------------------------------------------
AI_BEGIN_CUSTOM_NPC( npc_lonk, CNPC_Lonk )

	DECLARE_ACTIVITY( ACT_LONK_GUN_IDLE )

AI_END_CUSTOM_NPC()