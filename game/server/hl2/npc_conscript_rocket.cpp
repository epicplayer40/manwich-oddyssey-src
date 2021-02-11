/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// monster template
//=========================================================
// UNDONE: Holster weapon?
//This is the actual leak source code conscript. If you have a look at them ingame you will find that they act stupid as hell. However, they can use the RPG without running away like complete boneheads.

#include	"cbase.h"

#include	"npc_talker.h"
#include	"ai_schedule.h"
#include	"ai_memory.h"
#include	"ai_playerally.h"
#include	"ai_senses.h"
#include	"ai_behavior.h"
#include	"ai_behavior_assault.h"
#include	"ai_behavior_lead.h"
#include	"scripted.h"
#include	"basecombatweapon.h"
#include	"soundent.h"
#include	"NPCEvent.h"
#include	"AI_Hull.h"
#include	"AI_Node.h"
#include	"AI_Network.h"
#include	"ai_squadslot.h"
#include	"ai_squad.h"
#include	"npc_conscript_rocket.h"
#include	"activitylist.h"
#include "AI_Interactions.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"

ConVar	sk_ronscript_health( "sk_ronscript_health","45");

#define RONSCRIPT_MAD 		"RONSCRIPT_MAD"
#define RONSCRIPT_SHOT 		"RONSCRIPT_SHOT"
#define RONSCRIPT_KILL 		"RONSCRIPT_KILL"
#define RONSCRIPT_OUT_AMMO 	"RONSCRIPT_OUT_AMMO"
#define RONSCRIPT_ATTACK 	"RONSCRIPT_ATTACK"
#define RONSCRIPT_LINE_FIRE "RONSCRIPT_LINE_FIRE"
#define RONSCRIPT_POK 		"RONSCRIPT_POK"

#define RONSCRIPT_PAIN1		"RONSCRIPT_PAIN1"
#define RONSCRIPT_PAIN2		"RONSCRIPT_PAIN2"
#define RONSCRIPT_PAIN3		"RONSCRIPT_PAIN3"

#define RONSCRIPT_DIE1		"RONSCRIPT_DIE1"
#define RONSCRIPT_DIE2		"RONSCRIPT_DIE2"
#define RONSCRIPT_DIE3		"RONSCRIPT_DIE3"

//#define CONCEPT_PLAYER_USE "FollowWhenUsed"

//=========================================================
// Combine activities
//=========================================================
int	ACT_RONSCRIPT_AIM;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
// first flag is barney dying for scripted sequences?
#define		RONSCRIPT_AE_RELOAD		( 1 )
#define		RONSCRIPT_AE_DRAW		( 2 )
#define		RONSCRIPT_AE_SHOOT		( 3 )
#define		RONSCRIPT_AE_HOLSTER	( 4 )

#define		RONSCRIPT_BODY_GUNHOLSTERED	0
#define		RONSCRIPT_BODY_GUNDRAWN		1
#define		RONSCRIPT_BODY_GUNGONE		2

//-----------------------------------------------------------------------------
// Purpose: Initialize the custom schedules
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Ronscript::InitCustomSchedules(void) 
{
	INIT_CUSTOM_AI(CNPC_Ronscript);

	ADD_CUSTOM_TASK(CNPC_Ronscript,	TASK_RONSCRIPT_CROUCH);

	ADD_CUSTOM_SCHEDULE(CNPC_Ronscript,	SCHED_RONSCRIPT_FOLLOW);
	ADD_CUSTOM_SCHEDULE(CNPC_Ronscript,	SCHED_RONSCRIPT_DRAW);
	ADD_CUSTOM_SCHEDULE(CNPC_Ronscript,	SCHED_RONSCRIPT_FACE_TARGET);
	ADD_CUSTOM_SCHEDULE(CNPC_Ronscript,	SCHED_RONSCRIPT_WANDER);
	ADD_CUSTOM_SCHEDULE(CNPC_Ronscript,	SCHED_RONSCRIPT_STAND);
	ADD_CUSTOM_SCHEDULE(CNPC_Ronscript,	SCHED_RONSCRIPT_AIM);
	ADD_CUSTOM_SCHEDULE(CNPC_Ronscript,	SCHED_RONSCRIPT_BARNACLE_HIT);
	ADD_CUSTOM_SCHEDULE(CNPC_Ronscript,	SCHED_RONSCRIPT_BARNACLE_PULL);
	ADD_CUSTOM_SCHEDULE(CNPC_Ronscript,	SCHED_RONSCRIPT_BARNACLE_CHOMP);
	ADD_CUSTOM_SCHEDULE(CNPC_Ronscript,	SCHED_RONSCRIPT_BARNACLE_CHEW);

	ADD_CUSTOM_ACTIVITY(CNPC_Ronscript,	ACT_RONSCRIPT_AIM);

	AI_LOAD_SCHEDULE(CNPC_Ronscript,	SCHED_RONSCRIPT_FOLLOW);
	AI_LOAD_SCHEDULE(CNPC_Ronscript,	SCHED_RONSCRIPT_DRAW);
	AI_LOAD_SCHEDULE(CNPC_Ronscript,	SCHED_RONSCRIPT_FACE_TARGET);
	AI_LOAD_SCHEDULE(CNPC_Ronscript,	SCHED_RONSCRIPT_WANDER);
	AI_LOAD_SCHEDULE(CNPC_Ronscript,	SCHED_RONSCRIPT_STAND);
	AI_LOAD_SCHEDULE(CNPC_Ronscript,	SCHED_RONSCRIPT_AIM);
	AI_LOAD_SCHEDULE(CNPC_Ronscript,	SCHED_RONSCRIPT_BARNACLE_HIT);
	AI_LOAD_SCHEDULE(CNPC_Ronscript,	SCHED_RONSCRIPT_BARNACLE_PULL);
	AI_LOAD_SCHEDULE(CNPC_Ronscript,	SCHED_RONSCRIPT_BARNACLE_CHOMP);
	AI_LOAD_SCHEDULE(CNPC_Ronscript,	SCHED_RONSCRIPT_BARNACLE_CHEW);
}

LINK_ENTITY_TO_CLASS( npc_conscript_rocket, CNPC_Ronscript );
IMPLEMENT_CUSTOM_AI( npc_conscript_rocket, CNPC_Ronscript );

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_Ronscript )

	DEFINE_FIELD( m_fGunDrawn,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flPainTime,			FIELD_TIME ),
	DEFINE_FIELD( m_checkAttackTime,	FIELD_TIME ),
	DEFINE_FIELD( m_nextLineFireTime,	FIELD_TIME ),
	DEFINE_FIELD( m_lastAttackCheck,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bInBarnacleMouth,	FIELD_BOOLEAN ),
//	DEFINE_OUTPUT( m_OnPlayerUse, "OnPlayerUse" ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Ronscript::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		if (GetEnemy() != NULL && (GetEnemy()->IsPlayer()))
		{
			m_flPlaybackRate = 1.0;
		}
		BaseClass::RunTask( pTask );
		break;
	default:
		BaseClass::RunTask( pTask );
		break;
	}
}



/*void CNPC_Ronscript::UseFunc( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	m_bDontUseSemaphore = true;
	SpeakIfAllowed( TLK_USE );
	m_bDontUseSemaphore = false;

	m_OnPlayerUse.FireOutput( pActivator, pCaller );
}

void CNPC_Ronscript::FollowUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	Speak( CONCEPT_PLAYER_USE );

	CBaseEntity *pLeader = ( !m_FollowBehavior.GetFollowTarget() ) ? pCaller : NULL;
	m_FollowBehavior.SetFollowTarget( pLeader );

	if ( m_pSquad && m_pSquad->NumMembers() > 1 )
	{
		AISquadIter_t iter;
		for (CAI_BaseNPC *pSquadMember = m_pSquad->GetFirstMember( &iter ); pSquadMember; pSquadMember = m_pSquad->GetNextMember( &iter ) )
		{
			CNPC_Roncript *pBarney = dynamic_cast<CNPC_Ronscript *>(pSquadMember);
			if ( pBarney && pBarney != this)
			{
				pBarney->m_FollowBehavior.SetFollowTarget( pLeader );
			}
		}

	}
}
*/


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
Activity CNPC_Ronscript::NPC_TranslateActivity( Activity eNewActivity )
{
	return BaseClass::NPC_TranslateActivity( eNewActivity );
}


//=========================================================
// GetSoundInterests - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CNPC_Ronscript::GetSoundInterests ( void) 
{
	return	SOUND_WORLD		|
			SOUND_COMBAT	|
			SOUND_CARCASS	|
			SOUND_MEAT		|
			SOUND_GARBAGE	|
			SOUND_DANGER	|
			SOUND_PLAYER;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
Class_T	CNPC_Ronscript::Classify ( void )
{
	return	CLASS_CONSCRIPT;
}

//=========================================================
// ALertSound - barney says "Freeze!"
//=========================================================
void CNPC_Ronscript::AlertSound( void )
{
	if ( GetEnemy() != NULL )
	{
		if ( IsOkToCombatSpeak() )
		{
			Speak( RONSCRIPT_ATTACK );
		}
	}

}
//=========================================================
// MaxYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
float CNPC_Ronscript::MaxYawSpeed ( void )
{
	switch ( GetActivity() )
	{
	case ACT_IDLE_ANGRY_SMG1:		
		return 70;
		break;
	case ACT_WALK_AIM_RIFLE:
		return 70;
		break;
	case ACT_RUN_AIM_RIFLE:
		return 90;
		break;
	default:
		return 70;
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Set proper blend for shooting
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Ronscript::ConscriptFirePistol ( void )
{
	Vector vecShootOrigin;

	vecShootOrigin = GetLocalOrigin() + Vector( 0, 0, 55 );
	Vector vecShootDir = GetShootEnemyDir( vecShootOrigin );

	QAngle angDir;
	VectorAngles( vecShootDir, angDir );
	SetPoseParameter( 0, angDir.x );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CNPC_Ronscript::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
	case RONSCRIPT_AE_RELOAD:
		
		// We never actually run out of ammo, just need to refill the clip
		if (GetActiveWeapon())
		{
			GetActiveWeapon()->WeaponSound( RELOAD_NPC );
			GetActiveWeapon()->m_iClip1 = GetActiveWeapon()->GetMaxClip1(); 
			GetActiveWeapon()->m_iClip2 = GetActiveWeapon()->GetMaxClip2(); 
		}
		ClearCondition(COND_NO_PRIMARY_AMMO);
		ClearCondition(COND_NO_SECONDARY_AMMO);
		break;

/*	case RONSCRIPT_AE_SHOOT:
		ConscriptFirePistol();
		break;
*/
	case RONSCRIPT_AE_DRAW:
		// barney's bodygroup switches here so he can pull gun from holster
		m_nBody = RONSCRIPT_BODY_GUNDRAWN;
		m_fGunDrawn = true;
		break;

	case RONSCRIPT_AE_HOLSTER:
		// change bodygroup to replace gun in holster
		m_nBody = RONSCRIPT_BODY_GUNHOLSTERED;
		m_fGunDrawn = false;
		break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// Spawn
//=========================================================
void CNPC_Ronscript::Spawn()
{
	Precache( );

	SetModel( "models/animal.mdl" ); //Placeholder model until unique rocketeer model is finished
	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	SetBloodColor( BLOOD_COLOR_RED );
	m_iHealth			= sk_ronscript_health.GetFloat();
	SetViewOffset( Vector ( 0, 0, 50 ) );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_NPCState			= NPC_STATE_NONE;

	m_nBody			= 0; // gun in holster
	m_fGunDrawn			= false;
	m_bInBarnacleMouth	= false;

	m_nextLineFireTime	= 0;

	m_HackedGunPos		= Vector ( 0, 0, 55 );

	CapabilitiesAdd( bits_CAP_TURN_HEAD | bits_CAP_MOVE_GROUND | bits_CAP_MOVE_JUMP | bits_CAP_MOVE_CLIMB | bits_CAP_NO_HIT_PLAYER);
	CapabilitiesAdd	( bits_CAP_USE_WEAPONS );
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_OPEN_DOORS | bits_CAP_SQUAD | bits_CAP_AUTO_DOORS | bits_CAP_USE | bits_CAP_DOORS_GROUP);
	CapabilitiesAdd	( bits_CAP_DUCK );			// In reloading and cover

	NPCInit();
	//SetUse( &CNPC_Ronscript::FollowUse );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CNPC_Ronscript::Precache()
{
	engine->PrecacheModel("models/animal.mdl");

	enginesound->PrecacheSound("barney/ba_pain1.wav");
	enginesound->PrecacheSound("barney/ba_pain2.wav");
	enginesound->PrecacheSound("barney/ba_pain3.wav");

	enginesound->PrecacheSound("barney/ba_die1.wav");
	enginesound->PrecacheSound("barney/ba_die2.wav");
	enginesound->PrecacheSound("barney/ba_die3.wav");
	
	enginesound->PrecacheSound("barney/ba_close.wav");

	// every new barney must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	BaseClass::Precache();
}	

// Init talk data
void CNPC_Ronscript::TalkInit()
{
	
	BaseClass::TalkInit();

		// vortigaunt will try to talk to friends in this order:
	m_szFriends = "npc_conscript_rocket";
//	m_szFriends[1] = "npc_vortigaunt"; disable for now

	// get voice for head - just one barney voice for now
	GetExpresser()->SetVoicePitch( 100 );
}


static bool IsFacing( CBaseCombatCharacter *pBCC, const Vector &reference )
{
	Vector vecDir = (reference - pBCC->GetLocalOrigin());
	vecDir.z = 0;
	VectorNormalize( vecDir );
	Vector vecForward = pBCC->BodyDirection2D( );

	// He's facing me, he meant it
	if ( DotProduct( vecForward, vecDir ) > 0.96 )	// +/- 15 degrees or so
	{
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int	CNPC_Ronscript::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	// make sure friends talk about it if player hurts talkmonsters...
	int ret = BaseClass::OnTakeDamage_Alive( info );
	if (!IsAlive())
	{
		return ret;
	}

	if ( m_NPCState != NPC_STATE_PRONE && (info.GetAttacker()->GetFlags() & FL_CLIENT) )
	{

		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if ( GetEnemy() == NULL )
		{
			// If I'm already suspicious, get mad
			if (m_afMemory & bits_MEMORY_SUSPICIOUS)
			{
				// Alright, now I'm pissed!
				Speak( RONSCRIPT_MAD );

				Remember( bits_MEMORY_PROVOKED );

				// Allowed to hit the player now!
				CapabilitiesRemove(bits_CAP_NO_HIT_PLAYER);

//				StopFollowing();
			}
			else
			{
				// Hey, be careful with that
				Speak( RONSCRIPT_SHOT );
				Remember( bits_MEMORY_SUSPICIOUS );
			}
		}
		else if ( !(GetEnemy()->IsPlayer()) && (m_lifeState != LIFE_DEAD ))
		{
			Speak( RONSCRIPT_SHOT );
		}
	}
	return ret;
}

//------------------------------------------------------------------------------
// Purpose : Override to always shoot at eyes (for ducking behind things)
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CNPC_Ronscript::BodyTarget( const Vector &posSrc, bool bNoisy ) 
{
	return EyePosition();
}

//=========================================================
// PainSound
//=========================================================
/*void CNPC_Ronscript::PainSound( const CTakeDamageInfo &info )
{
	if (gpGlobals->curtime < m_flPainTime)
		return;
	
	m_flPainTime = gpGlobals->curtime + random->RandomFloat( 0.5, 0.75 );

	CPASAttenuationFilter filter( this );

	CSoundParameters params;
	if ( GetParametersForSound( "Barhl1.Pain", params, NULL ) )
	{
		params.pitch = GetExpresser()->GetVoicePitch();

		EmitSound_t ep( params );

		EmitSound( filter, entindex(), ep );
	}
}
*/

//=========================================================
// DeathSound 
//=========================================================
void CNPC_Ronscript::DeathSound( const CTakeDamageInfo &info )
{
	CPASAttenuationFilter filter( this );

	CSoundParameters params;
	if ( GetParametersForSound( "Barhl1.Die", params, NULL ) )
	{
		params.pitch = GetExpresser()->GetVoicePitch();

		EmitSound_t ep( params );

		EmitSound( filter, entindex(), ep );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Ronscript::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr )
{
	CTakeDamageInfo info = inputInfo;

	switch( ptr->hitgroup)
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if (info.GetDamageType() & (DMG_BULLET | DMG_SLASH | DMG_BLAST))
		{
			info.ScaleDamage( 0.5f ); //Thanks to these handy dandy vests provided by our benefactors, they take only take half damage from explosions, cutting, and bullets.
		}
		break;
	case 10:
		if (info.GetDamageType() & (DMG_BULLET | DMG_SLASH | DMG_CLUB))
		{
			info.SetDamage( info.GetDamage() - 20 );
			if (info.GetDamage() <= 0)
			{
				g_pEffects->Ricochet( ptr->endpos, (vecDir*-1.0f) );
				info.SetDamage( 0 );
			}
		}
		// always a head shot
		ptr->hitgroup = HITGROUP_HEAD;
		break;
	}

	BaseClass::TraceAttack( info, vecDir, ptr, 0 );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_Ronscript::TranslateSchedule( int scheduleType )
{
	int baseType;

	switch( scheduleType )
	{
	case SCHED_RONSCRIPT_DRAW:
		if ( GetEnemy() != NULL )
		{
			// face enemy, then draw.
			return SCHED_RONSCRIPT_DRAW;
		}
		else
		{
			// BUGBUG: What is this code supposed to do when there isn't an enemy? You dummy, just keep the gun out for a random time interval above 3 seconds.
			Warning( "BUG IN CONSCRIPT AI!\n");
			return SCHED_RONSCRIPT_WANDER;
		}
		break;

	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		{
			// call base class default so that barney will talk
			// when 'used' 
			baseType = BaseClass::TranslateSchedule(scheduleType);

			if (baseType == SCHED_IDLE_STAND)
				return SCHED_RONSCRIPT_FACE_TARGET;	// override this for different target face behavior
			else
			return SCHED_RONSCRIPT_WANDER;
		}
		break;

	case SCHED_CHASE_ENEMY:
		{
			// ---------------------------------------------
			// If I'm in ducking, cover pause for while
			// before running towards my enemy.  See if they
			// come out first as this is a good place to be!
			// ---------------------------------------------
			if (HasMemory(bits_MEMORY_INCOVER))
			{
				Forget( bits_MEMORY_INCOVER );
				return SCHED_COMBAT_SWEEP;
			}
		}
		break;
	case SCHED_TARGET_CHASE:
		return SCHED_RONSCRIPT_FOLLOW;
		break;

	case SCHED_IDLE_STAND:
		{
			// call base class default so that scientist will talk
			// when standing during idle
			baseType = BaseClass::TranslateSchedule(scheduleType);

			if (baseType == SCHED_RONSCRIPT_WANDER)
			{
				// just look straight ahead
				return SCHED_RONSCRIPT_WANDER;
			}
			return baseType;
			break;

		}
	case SCHED_FAIL_ESTABLISH_LINE_OF_FIRE:
		{
			return SCHED_RONSCRIPT_AIM;
			break;
		}

	}

		enum
	{
		SCHED_RONSCRIPT_RANGE_ATTACK1 = BaseClass::NEXT_SCHEDULE,
	};
	return BaseClass::TranslateSchedule( scheduleType );
}

/*bool CNPC_Ronscript::CreateBehaviors( void )
{
	BaseClass::CreateBehaviors();
	AddBehavior( &m_FollowBehavior );

	return true;
}

bool CNPC_Ronscript::OnBehaviorChangeStatus( CAI_BehaviorBase *pBehavior, bool fCanFinishSchedule )
{
	bool interrupt = BaseClass::OnBehaviorChangeStatus( pBehavior, fCanFinishSchedule );
	if ( !interrupt )
	{
		interrupt = ( pBehavior == &m_FollowBehavior && ( m_NPCState == NPC_STATE_IDLE || m_NPCState == NPC_STATE_ALERT ) );
	}
	return interrupt;
}

void CNPC_Ronscript::FollowUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	Speak( CONCEPT_PLAYER_USE );

	CBaseEntity *pLeader = ( !m_FollowBehavior.GetFollowTarget() ) ? pCaller : NULL;
	m_FollowBehavior.SetFollowTarget( pLeader );

	if ( m_pSquad && m_pSquad->NumMembers() > 1 )
	{
		AISquadIter_t iter;
		for (CAI_BaseNPC *pSquadMember = m_pSquad->GetFirstMember( &iter ); pSquadMember; pSquadMember = m_pSquad->GetNextMember( &iter ) )
		{
			CNPC_Ronscript *pBarney = dynamic_cast<CNPC_Ronscript *>(pSquadMember);
			if ( pBarney && pBarney != this)
			{
				pBarney->m_FollowBehavior.SetFollowTarget( pLeader );
			}
		}

	}
} THIS SHIT DON'T WORK DAWG!
*/

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_Ronscript::SelectSchedule ( void )
{
	// These things are done in any state but dead and prone
	if (m_NPCState != NPC_STATE_DEAD && m_NPCState != NPC_STATE_PRONE)
	{
		if ( HasCondition( COND_HEAR_DANGER ) )
		{
			return SCHED_TAKE_COVER_FROM_BEST_SOUND;
		}
		if ( HasCondition( COND_ENEMY_DEAD ) && IsOkToCombatSpeak() )
		{
			Speak( RONSCRIPT_KILL );
		}
	}
	switch( m_NPCState )
	{
	case NPC_STATE_PRONE:
		{
			if (m_bInBarnacleMouth)
			{
				return SCHED_RONSCRIPT_BARNACLE_CHOMP;
			}
			else
			{
				return SCHED_RONSCRIPT_BARNACLE_HIT;
			}
		}
	case NPC_STATE_COMBAT:
		{
// dead enemy
			if ( HasCondition( COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return BaseClass::SelectSchedule();
			}

			// always act surprized with a new enemy
			if ( HasCondition( COND_NEW_ENEMY ) && HasCondition( COND_LIGHT_DAMAGE) )
				return SCHED_SMALL_FLINCH;
				
			// wait for one schedule to draw gun
			if (!m_fGunDrawn )
				return SCHED_RONSCRIPT_DRAW;

			if ( HasCondition( COND_HEAVY_DAMAGE ) )
				return SCHED_TAKE_COVER_FROM_ENEMY;

			// ---------------------
			// no ammo
			// ---------------------
			if ( HasCondition ( COND_NO_PRIMARY_AMMO ) )
			{
				Speak( RONSCRIPT_OUT_AMMO );
				return SCHED_HIDE_AND_RELOAD;
			}
			else if (!HasCondition(COND_CAN_RANGE_ATTACK1) && HasCondition( COND_NO_SECONDARY_AMMO ))
			{
				Speak( RONSCRIPT_OUT_AMMO );
				return SCHED_HIDE_AND_RELOAD;
			}

			/* UNDONE: check distance for genade attacks...
			// If player is next to what I'm trying to attack...
			if ( HasCondition( COND_WEAPON_PLAYER_NEAR_TARGET ))
			{
				return SCHED_RONSCRIPT_AIM;
			}
			*/			

			// -------------------------------------------
			// If I might hit the player shooting...
			// -------------------------------------------
			if ( HasCondition( COND_WEAPON_PLAYER_IN_SPREAD ))
			{
				if ( IsOkToCombatSpeak() && m_nextLineFireTime	< gpGlobals->curtime)
				{
					Speak( RONSCRIPT_LINE_FIRE );
					m_nextLineFireTime = gpGlobals->curtime + 3.0f;
				}

				// Run to a new location or stand and aim
				if (random->RandomInt(0,2) == 0)
				{
					return SCHED_ESTABLISH_LINE_OF_FIRE;
				}
				else
				{
					return SCHED_RONSCRIPT_AIM;
				}
			}

			// -------------------------------------------
			// If I'm in cover and I don't have a line of
			// sight to my enemy, wait randomly before attacking
			// -------------------------------------------

		}
		break;

	case NPC_STATE_ALERT:	
	case NPC_STATE_IDLE:
		if ( HasCondition(COND_LIGHT_DAMAGE) || HasCondition(COND_HEAVY_DAMAGE))
		{
			// flinch if hurt
			return SCHED_SMALL_FLINCH;
		}

	
		// try to say something about smells
//		TrySmellTalk();
		break;
	}
	
	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
// Purpose:  This is a generic function (to be implemented by sub-classes) to
//			 handle specific interactions between different types of characters
//			 (For example the barnacle grabbing an NPC)
// Input  :  Constant for the type of interaction
// Output :	 true  - if sub-class has a response for the interaction
//			 false - if sub-class has no response
//-----------------------------------------------------------------------------
bool CNPC_Ronscript::HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt)
{
	if (interactionType == g_interactionBarnacleVictimDangle)
	{
		// Force choosing of a new schedule
//		ClearSchedule();
		m_bInBarnacleMouth	= true;
		return true;
	}
	else if ( interactionType == g_interactionBarnacleVictimReleased )
	{
		m_IdealNPCState = NPC_STATE_IDLE;

		CPASAttenuationFilter filter( this );
//		EmitSound( filter, entindex(), CHAN_VOICE, "barney/ba_close.wav", 1, SNDLVL_TALKING, 0, GetExpresser()->GetVoicePitch());

		m_bInBarnacleMouth	= false;
		SetAbsVelocity( vec3_origin );
		SetMoveType( MOVETYPE_STEP );
		return true;
	}
	else if ( interactionType == g_interactionBarnacleVictimGrab )
	{
		if ( GetFlags() & FL_ONGROUND )
		{
			RemoveFlag( FL_ONGROUND );
		}
		m_IdealNPCState = NPC_STATE_PRONE;
//		PainSound();
		return true;
	}
	return false;
}

void CNPC_Ronscript::DeclineFollowing( void )
{
	Speak( RONSCRIPT_POK );
}


//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

//=========================================================
// > SCHED_RONSCRIPT_FOLLOW
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_RONSCRIPT_FOLLOW,

	"	Tasks"
	"		TASK_GET_PATH_TO_TARGET			0"
	"		TASK_MOVE_TO_TARGET_RANGE		128"	// Move within 128 of target ent (client)
	"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_TARGET_FACE "
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_HEAR_DANGER"
	"		COND_PROVOKED"
);

//=========================================================
// > Wander around until enemy found
//=========================================================
AI_DEFINE_SCHEDULE
(
		SCHED_RONSCRIPT_WANDER,

		"	Tasks"
		"		TASK_WANDER						720432" // 480240 by default..
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			3"
		""
		"	Interrupts"
		"		COND_ZOMBIE_RELEASECRAB"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
		"		COND_DOOR_OPENED"
);

//=========================================================
//  > SCHED_RONSCRIPT_DRAW
//		much better looking draw schedule for when
//		conscript knows who he's gonna attack.
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_RONSCRIPT_DRAW,

	"	Tasks"
	"		 TASK_STOP_MOVING					0"
	"		 TASK_FACE_ENEMY					0"
	"		 TASK_PLAY_SEQUENCE_FACE_ENEMY		ACTIVITY:ACT_RANGE_ATTACK1 "
	""
	"	Interrupts"
);

//===============================================
//	> SCHED_RONSCRIPT_AIM
//
//	Stand in place and aim at enemy (used when
//  line of sight blocked by player)
//===============================================
AI_DEFINE_SCHEDULE
(
	SCHED_RONSCRIPT_AIM,

	"	Tasks"
	"		TASK_STOP_MOVING		0"
	"		TASK_FACE_ENEMY			0"
	"		TASK_PLAY_SEQUENCE		ACTIVITY:ACT_IDLE_AIM_RPG_STIMULATED"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_NO_PRIMARY_AMMO"
	"		COND_WEAPON_HAS_LOS"
	"		COND_CAN_MELEE_ATTACK1 "
	"		COND_CAN_MELEE_ATTACK2 "
	"		COND_HEAR_DANGER"
);

//=========================================================
// > SCHED_RONSCRIPT_FACE_TARGET
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_RONSCRIPT_FACE_TARGET,

	"	Tasks"
	"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE_SMG1_STIMULATED"
	"		TASK_FACE_TARGET			0"
	"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE_SMG1_STIMULATED"
	"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_TARGET_CHASE"
	""
	"	Interrupts"
	//"		CLIENT_PUSH			<<TODO>>
	"		COND_NEW_ENEMY"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_HEAR_DANGER"
	"		COND_PROVOKED"
);

//=========================================================
// > SCHED_RONSCRIPT_STAND
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_RONSCRIPT_STAND,

	"	Tasks"
	"		TASK_STOP_MOVING			0"
	"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE_SMG1_STIMULATED "
	"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_RONSCRIPT_WANDER"
	"		TASK_WAIT					2"	// repick IDLESTAND every two seconds.
	"		TASK_TALKER_HEADRESET		0"	// reset head position
	""
	"	Interrupts	 "
	"		COND_NEW_ENEMY"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_SMELL"
	"		COND_PROVOKED"
	"		COND_HEAR_COMBAT"
	"		COND_HEAR_DANGER"
);

AI_DEFINE_SCHEDULE
(
		SCHED_RONSCRIPT_RANGE_ATTACK1,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_FACE_ENEMY			0"
		"		TASK_ANNOUNCE_ATTACK	1"	// 1 = primary attack
		"		TASK_RANGE_ATTACK1		0"
		""
		"	Interrupts"
		"		COND_HEAVY_DAMAGE"
		"		COND_ENEMY_OCCLUDED"
		"		COND_HEAR_DANGER"
		"		COND_WEAPON_BLOCKED_BY_FRIEND"
		"		COND_WEAPON_SIGHT_OCCLUDED"
);

//=========================================================
// > SCHED_RONSCRIPT_BARNACLE_HIT
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_RONSCRIPT_BARNACLE_HIT,

	"	Tasks"
	"		TASK_STOP_MOVING			0"
	"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_BARNACLE_HIT"
	"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_RONSCRIPT_BARNACLE_PULL"
	""
	"	Interrupts"
);

//=========================================================
// > SCHED_RONSCRIPT_BARNACLE_PULL
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_RONSCRIPT_BARNACLE_PULL,

	"	Tasks"
	"		 TASK_PLAY_SEQUENCE			ACTIVITY:ACT_BARNACLE_PULL"
	""
	"	Interrupts"
);

//=========================================================
// > SCHED_RONSCRIPT_BARNACLE_CHOMP
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_RONSCRIPT_BARNACLE_CHOMP,

	"	Tasks"
	"		 TASK_PLAY_SEQUENCE			ACTIVITY:ACT_BARNACLE_CHOMP"
	"		 TASK_SET_SCHEDULE			SCHEDULE:SCHED_RONSCRIPT_BARNACLE_CHEW"
	""
	"	Interrupts"
);

void CNPC_Ronscript::BuildScheduleTestBits( void )
{
	// FIXME: we need a way to make scenes non-interruptible
#if 0
	if ( IsCurSchedule( SCHED_RANGE_ATTACK1 ) || IsCurSchedule( SCHED_SCENE_GENERIC ) )
	{
		ClearCustomInterruptCondition( COND_LIGHT_DAMAGE );
		ClearCustomInterruptCondition( COND_HEAVY_DAMAGE );
		ClearCustomInterruptCondition( COND_NEW_ENEMY );
		ClearCustomInterruptCondition( COND_HEAR_DANGER );
	}
#endif

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


//=========================================================
// > SCHED_RONSCRIPT_BARNACLE_CHEW
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_RONSCRIPT_BARNACLE_CHEW,

	"	Tasks"
	"		 TASK_PLAY_SEQUENCE			ACTIVITY:ACT_BARNACLE_CHEW"
);
