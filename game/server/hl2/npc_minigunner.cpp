#include	"cbase.h"

#include "npc_minigunner.h"

#include "npcevent.h"
#include "ai_hull.h"
#include "ai_basenpc.h"
#include "ai_task.h"
#include "ai_default.h"
#include "ai_schedule.h"
#include "ai_node.h"
#include "ai_network.h"
#include "ai_hint.h"
#include "ai_link.h"
#include "ai_waypoint.h"
#include "ai_navigator.h"
#include "ai_motor.h"
#include "ai_senses.h"
#include "ai_squadslot.h"
#include "ai_squad.h"
#include "ai_memory.h"
#include "ai_tacticalservices.h"
#include "ai_moveprobe.h"
#include "ai_utils.h"
#include "datamap.h"
#include "basecombatcharacter.h"
#include "basehlcombatweapon.h"


#define	HASSAULT_MAX_RANGE		800




enum 
{
	TASK_HASSAULT_ALERT = LAST_SHARED_TASK,
	TASK_HASSAULT_IDLE,
	TASK_HASSAULT_HEAR_TARGET,
	TASK_HASSAULT_SEEPLAYER,	
	TASK_HASSAULT_RANGE_ATTACK1,
	TASK_HASSAULT_SET_BALANCE,
};
enum
{
	SCHED_HASSAULT_CHASE = LAST_SHARED_SCHEDULE,
	SCHED_HASSAULT_RANGE_ATTACK1,
	SCHED_HASSAULT_IDLE,
};




DEFINE_CUSTOM_AI;
};

	LINK_ENTITY_TO_CLASS( monster_human_assault, CNPC_Hassault );



	void CNPC_Hassault::Precache()
	{

		m_iAmmoType = GetAmmoDef()->Index("Pistol");

		BaseClass::Precache();
		PrecacheModel( "models/hgang_minigun.mdl" );
		PrecacheScriptSound( "Hassault.Alert" );
		PrecacheScriptSound( "Hassault.Pain" );
		PrecacheScriptSound( "Hassault.Death" );
		PrecacheScriptSound( "Hassault.Gun"	);
	}

	void CNPC_Hassault::Spawn()
	{
		Precache();
		SetModel( "models/hgang_minigun.mdl" );

		BaseClass::Spawn();

		SetHullType(HULL_HUMAN);
		SetHullSizeNormal();
		SetSolid( SOLID_BBOX );
		AddSolidFlags( FSOLID_NOT_STANDABLE );
		SetMoveType( MOVETYPE_STEP );

		m_bloodColor	= BLOOD_COLOR_RED;
		m_iHealth		= sk_hassault_health.GetFloat();
		m_flFieldOfView	=	VIEW_FIELD_WIDE;
		m_NPCState		=	NPC_STATE_NONE;

		CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_INNATE_RANGE_ATTACK1 | bits_CAP_SQUAD | bits_CAP_NO_HIT_SQUADMATES | bits_CAP_OPEN_DOORS );

		//NPC_Init(); Is this needed??? hmm
	}

	void CNPC_Hassault::AlertSound( void )
	{
		EmitSound( "Hassault.Alert" );
	}

	void CNPC_Hassault::PainSound( const CTakeDamageInfo &info )
	{
		if ( random->RandomInt( 0, 5 ) == 0)
		{
			CPASAttenuationFilter filter( this );

			CSoundParameters params;
			if ( GetParametersForSound( "Hassault.Pain", params, NULL ) )
			{
				EmitSound_t ep( params );

				EmitSound( filter, entindex(), ep );
			}
		}
	}

	CLASS_T CNPC_Hassault::Classify ( void )
	{
		return CLASS_STALKER;
	}

	void CNPC_Hassault::DeathSound( const CTakeDamageInfo &info )
	{
		EmitSound( "Hassault.Death" );
	}

	float CNPC_Hassault::MaxYawSpeed ( void )
	{
		float flYS = 0;

		switch ( GetActivity () )
		{
		case	ACT_RUN:	flYS = 100; break;
		case	ACT_WALK:	flYS = 90; break;
		case	ACT_IDLE:	flYS = 90; break;
		case	ACT_RANGE_ATTACK1:	flYS = 70; break;
		
		default:
			flYS = 90;
			break;
		}

		return flYS;
	}

	void CNPC_Hassault::Event_Killed( const CTakeDamageInfo &info )
	{
		CTakeDamageInfo Info = info;
		if (Info.GetDamageType() & DMG_BLAST)
		{
			if ( random->RandomInt( 0, 4 ) == 0)
			{
				Gib();
			}
		}

		BaseClass::Event_Killed ( info );
	}

	int CNPC_Hassault::SelectSchedule( void )
	{
		return BaseClass::SelectSchedule();
	}

	void CNPC_Hassault::OnTakeDamage_Alive( const CTakeDamageInfo &info )
	{

		CTakeDamageInfo newInfo = info;
		if( newInfo.GetDamageType() & DMG_CRUSH)
		{
			newInfo.ScaleDamage( 0.5 );
		}	

		int nDamageTaken = BaseClass::OnTakeDamage_Alive( newInfo );

		return nDamageTaken;
	}

	void CNPC_Hassault::StartTask ( const Task_t *pTask )
	{
		switch ( pTask->iTask )
		{
		case TASK_HASSAULT_IDLE:
			SetActivity( ACT_IDLE );
			TaskComplete();
			break;
		case TASK_HASSAULT_RANGE_ATTACK1:
			{
				Vector flEnemyLKP = GetEnemyLKP();
				GetMotor()->SetIdealYawToTarget( flEnemyLKP );
			}
			return;
			default:
				BaseClass::StartTask( pTask );
				break;
		}
	}

	void CNPC_Hassault::RunTask( const Task_t *pTask )
	{
		switch ( pTask->iTask )
		{
		case TASK_HASSAULT_RANGE_ATTACK1:
			{
				Vector flEnemyLKP = GetEnemyLKP();
				GetMotor()->SetIdealYawToTargetAndUpdate( flEnemyLKP );

				if ( IsActivityFinished() )
				{
					TaskComplete();
					return;
				}
			}
			return;
		}

		BaseClass::RunTask( pTask );
	}

#define TRANSLATE_SCHEDULE( type, in, out ) { if ( type == in ) return out; }

	Activity CNPC_Hassault::NPC_TranslateActivity( Activity baseAct )
	{
		if ( baseAct == ACT_IDLE && m_NPCState != NPC_STATE_IDLE )
		{
			return ( Activity ) ACT_IDLE_ANGRY;
		}
		
		return baseAct;
	}

	int CNPC_Hassault::TranslateSchedule( int type )
	{
		switch( type )
		{
		case SCHED_RANGE_ATTACK1:
				return SCHED_HASSAULT_RANGE_ATTACK1;
				break;
		}

		return BaseClass::TranslateSchedule( type );
	}

	void CNPC_Hassault::HandleAnimEvent( animevent_t *pEvent )
	{
		switch( pEvent->event )
		{
			case HASSAULT_AE_SHOOT:
			{
				ShootMinigun();
				Vector flEnemyLKP = GetEnemyLKP();
				GetMotor()->SetIdealYawToTargetAndUpdate( flEnemyLKP );
			}
			break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;

		}
	}

void CNPC_Hassault::ShootMinigun( void )
{
	Vector vecShootOrigin;

//	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = GetAbsOrigin() + Vector( 0, 0, 55 );
	Vector vecShootDir = GetShootEnemyDir( vecShootOrigin );
	QAngle angDir;
	
	QAngle	angShootDir;
	GetAttachment( LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir );
	//VectorAngles( vecShootDir, angDir );
	//SetBlending( 0, angDir.x );
	//pev->effects = EF_MUZZLEFLASH;
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 800, m_iAmmoType  );
//	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "hass/minigun_shoot.wav", 1, ATTN_NORM, 0, 100 );
}

int CNPC_Hassault::RangeAttack1Conditions( float flDot, float flDist )
{
	if ( flDist > HASSAULT_MAX_RANGE )
		return COND_TOO_FAR_TO_ATTACK;	
	return COND_CAN_RANGE_ATTACK1;
}

int CNPC_Hassault::SelectSchedule( void )
{
	if( HasCondition ( COND_CAN_RANGE_ATTACK1 ) )
	{
		return SCHED_HASSAULT_RANGE_ATTACK1;
	}
	if( HasCondition( COND_SEE_ENEMY ) )
	{
		if( HasCondition( COND_TOO_FAR_TO_ATTACK ) )
		{
			return SCHED_HASSAULT_CHASE;
			//TODO: Implement one in 8 chance to spout "GET BACK HERE!!!" Lines
		}
		return SCHED_HASSAULT_RANGE_ATTACK1;
	}
	return BaseClass::SelectSchedule();
}

int CNPC_Hassault::TranslateSchedule( int type )
{
	switch( type )
	{
	case SCHED_RANGE_ATTACK1:
		return SCHED_HASSAULT_RANGE_ATTACK1;
		break;
	case SCHED_CHASE_ENEMY:
		return SCHED_HASSAULT_CHASE;
		break;
	}
	return BaseClass:TranslateSchedule( type );
}

///////////////////////////////////////////////////////

int CNPC_Hassault::GetSoundInterests( void )
{
	return SOUND_COMBAT |
		   SOUND_BULLET_IMPACT;
}

AI_BEGIN_CUSTOM_NPC( monster_human_assault, CNPC_Hassault )

	DECLARE_TASK( TASK_HASSAULT_RANGE_ATTACK1 )
	DECLARE_TASK( TASK_HASSAULT_IDLE )
//	DECLARE_ACTIVITY ( ACT_SHOOTMINIGUN )

	DEFINE_SCHEDULE
	(
		SCHED_HASSAULT_RANGE_ATTACK1,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_FACE_ENEMY			0"
		"		TASK_ANNOUNCE_ATTACK	1"
		"		TASK_PLAY_SEQUENCE		ACTIVITY:ACT_RANGE_ATTACK1"
		"	"
			"	Interrupts"
			"	COND_NEW_ENEMY"
			"	COND_ENEMY_DEAD"
			"	COND_TOO_FAR_TO_ATTACK"
	)
	DEFINE_SCHEDULE
	(
		SCHED_HASSAULT_CHASE,

		"	Tasks"
		"		TASK_SET_TOLERANCE_DISTANCE		320"
		"		 TASK_GET_CHASE_PATH_TO_ENEMY	1000"
		"		 TASK_RUN_PATH					0"
		"		 TASK_WAIT_FOR_MOVEMENT			0"
		"		 TASK_FACE_ENEMY				0"
		"	"
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
		"		COND_CAN_RANGE_ATTACK1"
	)
	DEFINE_SCHEDULE
	(
		SCHED_HASSAULT_IDLE,

		"	Tasks"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_IDLE"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
	)


AI_END_CUSTOM_NPC()

