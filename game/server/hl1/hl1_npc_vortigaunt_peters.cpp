//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:	
//
// $NoKeywords: $
//=============================================================================//
// Peters, the well-meaning local leader of the friendly Zappas.


//TODO: Implement BLOOD_COLOR_BLUE (complete!)
//TODO: Make them follow the player when use key is pressed (complete!)
#include "cbase.h"
#include "beam_shared.h"
#include "game.h"
#include "ai_default.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "ai_route.h"
#include "ai_squad.h"
#include "ai_task.h"
#include "ai_node.h"
#include "ai_hint.h"
#include "ai_squadslot.h"
#include "ai_motor.h"
#include "npcevent.h"
#include "gib.h"
#include "ai_interactions.h"
#include "ndebugoverlay.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "hl1_npc_vortigaunt_ally.h"
#include "soundent.h"
#include "player.h"
#include "IEffects.h"
#include "basecombatweapon.h"
#include	"ai_behavior_follow.h"
#include	"AI_Criteria.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		ISLAVE_AE_CLAW		( 1 )
#define		ISLAVE_AE_CLAWRAKE	( 2 )
#define		ISLAVE_AE_ZAP_POWERUP	( 3 )
#define		ISLAVE_AE_ZAP_SHOOT		( 4 )
#define		ISLAVE_AE_ZAP_DONE		( 5 )


ConVar	sk_islave_ally_peters_health( "sk_islave_ally_peters_health","100");
ConVar	sk_islave_ally_peters_dmg_claw( "sk_islave_ally_peters_dmg_claw","10");
ConVar	sk_islave_ally_peters_dmg_clawrake( "sk_islave_ally_peters_dmg_clawrake","25");
ConVar	sk_islave_ally_peters_dmg_zap( "sk_islave_ally_peters_dmg_zap","14");

LINK_ENTITY_TO_CLASS( monster_alien_slave_ally_peters, CNPC_HL1AllyPeters );

BEGIN_DATADESC( CNPC_HL1AllyPeters )
	DEFINE_FIELD( m_iBravery, FIELD_INTEGER ),
	DEFINE_ARRAY( m_pBeam, FIELD_CLASSPTR, PETERS_MAX_BEAMS ),
	DEFINE_FIELD( m_iBeams, FIELD_INTEGER ),
	DEFINE_FIELD( m_flNextAttack, FIELD_TIME ),
	DEFINE_FIELD( m_iVoicePitch, FIELD_INTEGER ),
	DEFINE_FIELD( m_hDead, FIELD_EHANDLE ),
END_DATADESC()

enum
{
	SCHED_PETERS_ATTACK = LAST_SHARED_SCHEDULE,
};

#define PETERS_IGNORE_PLAYER 64

//=========================================================
// Spawn
//=========================================================
void CNPC_HL1AllyPeters::Spawn()
{
	Precache( );

	SetModel( "models/islave_ally_peters.mdl" );
	
	SetRenderColor( 255, 255, 255, 255 );

	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();
	
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	m_bloodColor		= BLOOD_COLOR_BLUE;
	ClearEffects();
    m_iHealth			= sk_islave_ally_peters_health.GetFloat();
	//pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE;
	m_NPCState			= NPC_STATE_NONE;

	m_iVoicePitch		= random->RandomInt( 85, 110 );

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS | bits_CAP_USE | bits_CAP_DOORS_GROUP);
	CapabilitiesAdd( bits_CAP_INNATE_RANGE_ATTACK1 | bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_TURN_HEAD | bits_CAP_ANIMATEDFACE );
	CapabilitiesAdd( bits_CAP_SQUAD | bits_CAP_NO_HIT_SQUADMATES | bits_CAP_FRIENDLY_DMG_IMMUNE  );

	m_iBravery = 0;

	NPCInit();
	SetUse( &CNPC_HL1AllyPeters::FollowerUse );

	BaseClass::Spawn();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CNPC_HL1AllyPeters::Precache()
{
	BaseClass::Precache();

	PrecacheModel("models/islave_ally_peters.mdl");
	PrecacheModel("sprites/lgtning.vmt");


	PrecacheScriptSound( "Vortigaunt.Cower" );
	PrecacheScriptSound( "Vortigaunt.Pain" );
	PrecacheScriptSound( "Vortigaunt.Die" );
	PrecacheScriptSound( "Vortigaunt.AttackHit" );
	PrecacheScriptSound( "Vortigaunt.AttackMiss" );
	PrecacheScriptSound( "Vortigaunt.ZapPowerup" );
	PrecacheScriptSound( "Vortigaunt.ZapShoot" );
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
Class_T	CNPC_HL1AllyPeters::Classify ( void )
{
	return	CLASS_CONSCRIPT;
}

void CNPC_HL1AllyPeters::CallForHelp( char *szClassname, float flDist, CBaseEntity * pEnemy, Vector &vecLocation )
{
	// ALERT( at_aiconsole, "help " );

	// skip ones not on my netname
	if ( !m_pSquad )
		 return;

	AISquadIter_t iter;
	for (CAI_BaseNPC *pSquadMember = m_pSquad->GetFirstMember( &iter ); pSquadMember; pSquadMember = m_pSquad->GetNextMember( &iter ) )
	{
		float d = ( GetAbsOrigin() - pSquadMember->GetAbsOrigin() ).Length();

		if ( d < flDist )
		{
			pSquadMember->Remember( bits_MEMORY_PROVOKED );
			pSquadMember->UpdateEnemyMemory( pEnemy, vecLocation );
		}
	}
}


//=========================================================
// ALertSound - scream
//=========================================================
void CNPC_HL1AllyPeters::AlertSound( void )
{
	if ( GetEnemy() != NULL )
	{
		SENTENCEG_PlayRndSz( edict(), "SLV_ALERT", 1.0, SNDLVL_NORM, 0, m_iVoicePitch );

		Vector vecTmp = GetEnemy()->GetAbsOrigin();
		CallForHelp( "monster_alien_slave_ally_peters", 3000, GetEnemy(), vecTmp );
	}
}

//=========================================================
// IdleSound
//=========================================================
void CNPC_HL1AllyPeters::IdleSound( void )
{
	if ( random->RandomInt( 0, 2 ) == 0)
	  	 SENTENCEG_PlayRndSz( edict(), "SLV_IDLE", 0.85, SNDLVL_NORM, 0, m_iVoicePitch);
}

//=========================================================
// PainSound
//=========================================================
void CNPC_HL1AllyPeters::PainSound( const CTakeDamageInfo &info )
{
	if ( random->RandomInt( 0, 2 ) == 0)
	{
		CPASAttenuationFilter filter( this );

		CSoundParameters params;
		if ( GetParametersForSound( "Vortigaunt.Pain", params, NULL ) )
		{
			EmitSound_t ep( params );
			params.pitch = m_iVoicePitch;

			EmitSound( filter, entindex(), ep );
		}
	}
}

//=========================================================
// DieSound
//=========================================================

void CNPC_HL1AllyPeters::DeathSound( const CTakeDamageInfo &info )
{
	CPASAttenuationFilter filter( this );
	CSoundParameters params;
	if ( GetParametersForSound( "Vortigaunt.Die", params, NULL ) )
	{
		EmitSound_t ep( params );
		params.pitch = m_iVoicePitch;

		EmitSound( filter, entindex(), ep );
	}
}

int CNPC_HL1AllyPeters::GetSoundInterests ( void )
{
	return	SOUND_WORLD	|
			SOUND_COMBAT	|
			SOUND_DANGER	|
			SOUND_PLAYER;
}

void CNPC_HL1AllyPeters::Event_Killed( const CTakeDamageInfo &info )
{
	ClearBeams( );
	BaseClass::Event_Killed( info );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
float CNPC_HL1AllyPeters::MaxYawSpeed ( void )
{
	float flYS;

	switch ( GetActivity() )
	{
	case ACT_WALK:		
		flYS = 50;	
		break;
	case ACT_RUN:		
		flYS = 70;
		break;
	case ACT_IDLE:		
		flYS = 50;
		break;
	default:
		flYS = 90;
		break;
	}

	return flYS;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CNPC_HL1AllyPeters::HandleAnimEvent( animevent_t *pEvent )
{
	// ALERT( at_console, "event %d : %f\n", pEvent->event, pev->frame );
	switch( pEvent->event )
	{
		case ISLAVE_AE_CLAW:
		{
			// SOUND HERE!
			CBaseEntity *pHurt = CheckTraceHullAttack( 40, Vector(-10,-10,-10), Vector(10,10,10), sk_islave_ally_peters_dmg_claw.GetFloat(), DMG_SLASH );
			CPASAttenuationFilter filter( this );
			if ( pHurt )
			{
				if ( pHurt->GetFlags() & ( FL_NPC | FL_CLIENT ) )
					 pHurt->ViewPunch( QAngle( 5, 0, -18 ) );
			
				// Play a random attack hit sound
				CSoundParameters params;
				if ( GetParametersForSound( "Vortigaunt.AttackHit", params, NULL ) )
				{
					EmitSound_t ep( params );
					params.pitch = m_iVoicePitch;

					EmitSound( filter, entindex(), ep );
				}
			}
			else
			{
				// Play a random attack miss sound
				CSoundParameters params;
				if ( GetParametersForSound( "Vortigaunt.AttackMiss", params, NULL ) )
				{
					EmitSound_t ep( params );
					params.pitch = m_iVoicePitch;

					EmitSound( filter, entindex(), ep );
				}
			}
		}
		break;

		case ISLAVE_AE_CLAWRAKE:
		{
			CBaseEntity *pHurt = CheckTraceHullAttack( 40, Vector(-10,-10,-10), Vector(10,10,10), sk_islave_ally_peters_dmg_clawrake.GetFloat(), DMG_SLASH );
			CPASAttenuationFilter filter2( this );
			if ( pHurt )
			{
				if ( pHurt->GetFlags() & ( FL_NPC | FL_CLIENT ) )
					 pHurt->ViewPunch( QAngle( 5, 0, 18 ) );

				CSoundParameters params;
				if ( GetParametersForSound( "Vortigaunt.AttackHit", params, NULL ) )
				{
					EmitSound_t ep( params );
					params.pitch = m_iVoicePitch;

					EmitSound( filter2, entindex(), ep );
				}
			}
			else
			{
				CSoundParameters params;
				if ( GetParametersForSound( "Vortigaunt.AttackMiss", params, NULL ) )
				{
					EmitSound_t ep( params );
					params.pitch = m_iVoicePitch;

					EmitSound( filter2, entindex(), ep );
				}
			}
		}
		break;

		case ISLAVE_AE_ZAP_POWERUP:
		{
			// speed up attack when on hard
			if ( g_iSkillLevel == SKILL_HARD )
				 m_flPlaybackRate = 1.5;

			Vector v_forward;
			GetVectors( &v_forward, NULL, NULL );

			CBroadcastRecipientFilter filter;
			te->DynamicLight( filter, 0.0, &GetAbsOrigin(), 0, 255, 255, 2, 120, 0.2 / m_flPlaybackRate, 0 );
			//te->DynamicLight( filter, 0.0, &GetAbsOrigin(), 160, 150, 255, 2, 120, 0.2 / m_flPlaybackRate, 0 );

			if ( m_hDead != NULL )
			{
				WackBeam( -1, m_hDead );
				WackBeam( 1, m_hDead );
			}
			else
			{
				ArmBeam( -1 );
				ArmBeam( 1 );
				BeamGlow( );
			}

			CPASAttenuationFilter filter3( this );
			CSoundParameters params;
			if ( GetParametersForSound( "Vortigaunt.ZapPowerup", params, NULL ) )
			{
				EmitSound_t ep( params );
				ep.m_nPitch = 100 + m_iBeams * 10;
				EmitSound( filter3, entindex(), ep );
			}

// Huh?  Model doesn't have multiple texturegroups, commented this out.  -LH
//			m_nSkin = m_iBeams / 2;
		}
		break;

		case ISLAVE_AE_ZAP_SHOOT:
		{
			ClearBeams( );

			if ( m_hDead != NULL )
			{
				Vector vecDest = m_hDead->GetAbsOrigin() + Vector( 0, 0, 38 );
				trace_t trace;
				UTIL_TraceHull( vecDest, vecDest, GetHullMins(), GetHullMaxs(),MASK_SOLID, m_hDead, COLLISION_GROUP_NONE, &trace );

				if ( !trace.startsolid )
				{
					CBaseEntity *pNew = Create( "monster_alien_slave_ally_peters", m_hDead->GetAbsOrigin(), m_hDead->GetAbsAngles() );
					
					pNew->AddSpawnFlags( 1 );
					WackBeam( -1, pNew );
					WackBeam( 1, pNew );
					UTIL_Remove( m_hDead );
					break;
				}
			}
			
			ClearMultiDamage();

			ZapBeam( -1 );
			ZapBeam( 1 );

			CPASAttenuationFilter filter4( this );
			EmitSound( filter4, entindex(), "Vortigaunt.ZapShoot" );
			ApplyMultiDamage();

			m_flNextAttack = gpGlobals->curtime + random->RandomFloat( 0.5, 4.0 );
		}
		break;

		case ISLAVE_AE_ZAP_DONE:
		{
			ClearBeams();
		}
		break;

		default:
			BaseClass::HandleAnimEvent( pEvent );
			break;
	}
}

//------------------------------------------------------------------------------
// Purpose : For innate range attack
// Input   :
// Output  :
//------------------------------------------------------------------------------
int CNPC_HL1AllyPeters::RangeAttack1Conditions( float flDot, float flDist )
{
	if ( GetEnemy() == NULL )
		 return( COND_LOST_ENEMY );
	if ( flDist > 1024 )
	{
		return( COND_TOO_FAR_TO_ATTACK );
	}
	if ( gpGlobals->curtime < m_flNextAttack )
		 return COND_NONE;

	if ( HasCondition( COND_CAN_MELEE_ATTACK1 ) )
		 return COND_NONE;
	
	return COND_CAN_RANGE_ATTACK1;
}


void CNPC_HL1AllyPeters::StartTask( const Task_t *pTask )
{
	ClearBeams();
	BaseClass::StartTask( pTask );
}

//=========================================================
// TakeDamage - get provoked when injured
//=========================================================

int CNPC_HL1AllyPeters::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	CTakeDamageInfo newInfo = info;
	if( newInfo.GetDamageType() & DMG_SONIC)
	{
		newInfo.ScaleDamage( 0 );
		DevMsg( "shockwave damage; no actual damage is taken\n" );
	}	

	int nDamageTaken = BaseClass::OnTakeDamage_Alive( newInfo );


	return nDamageTaken;
}

void CNPC_HL1AllyPeters::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	if ( info.GetDamageType() & DMG_SHOCK )
		 return;

	BaseClass::TraceAttack( info, vecDir, ptr );
}

//=========================================================
//=========================================================

int CNPC_HL1AllyPeters::SelectSchedule( void )
{

	ClearBeams();


	if ( GetFollowTarget() == NULL )
	{
		if ( HasCondition( COND_PLAYER_PUSHING ) && !(GetSpawnFlags() & SF_NPC_PREDISASTER ) )	// Player wants me to move
			return SCHED_HL1TALKER_FOLLOW_MOVE_AWAY;
	}

	if ( BehaviorSelectSchedule() )
		return BaseClass::SelectSchedule();

	if ( HasCondition( COND_ENEMY_DEAD ))// && IsOkToSpeak() )
	{
		Speak( "SLV_KILL" );
	}

	switch( m_NPCState )
	{


	case NPC_STATE_ALERT:
		{
			if ( HasCondition ( COND_HEAR_DANGER ) || HasCondition ( COND_HEAR_COMBAT ) )
			{
				if ( HasCondition ( COND_HEAR_DANGER ) )
					 return SCHED_TAKE_COVER_FROM_BEST_SOUND;
				
				else
				 	 return SCHED_INVESTIGATE_SOUND;
			}
		}
		break;


	case NPC_STATE_COMBAT:
		{
			// dead enemy
			if ( HasCondition( COND_ENEMY_DEAD ) )
				 return BaseClass::SelectSchedule(); // call base class, all code to handle dead enemies is centralized there.
		
			// always act surprized with a new enemy
			if ( HasCondition( COND_NEW_ENEMY ) && HasCondition( COND_LIGHT_DAMAGE) )
				return SCHED_SMALL_FLINCH;
				
			if ( HasCondition( COND_HEAVY_DAMAGE ) )
				 return SCHED_TAKE_COVER_FROM_ENEMY;

			if ( !HasCondition(COND_SEE_ENEMY) )
			{
				// we can't see the enemy
				if ( !HasCondition(COND_ENEMY_OCCLUDED) )
				{
					// enemy is unseen, but not occluded!
					// turn to face enemy
					return SCHED_CHASE_ENEMY;
				}
				else //These two have been swapped
				{
					return SCHED_COMBAT_FACE;
				}

		if ( HasCondition( COND_CAN_RANGE_ATTACK1 ) )
			 return SCHED_RANGE_ATTACK1;

		if ( m_iHealth < 20 || m_iBravery < 0)
			{
			if ( !HasCondition( COND_CAN_MELEE_ATTACK1 ) )
					{
					SetDefaultFailSchedule( SCHED_CHASE_ENEMY );
					if ( HasCondition( COND_LIGHT_DAMAGE ) || HasCondition( COND_HEAVY_DAMAGE ) ) 
						 return SCHED_TAKE_COVER_FROM_ENEMY;
						if ( HasCondition ( COND_SEE_ENEMY ) && HasCondition ( COND_ENEMY_FACING_ME ) )
						 return SCHED_TAKE_COVER_FROM_ENEMY;
					}
				}

			}
		}
		break;

	if( HasCondition( COND_SEE_ENEMY ) ) /////////added this - Should pursue enemies now..
	{
		if( HasCondition( COND_TOO_FAR_TO_ATTACK ) )
		{
			return SCHED_CHASE_ENEMY;
		}
	}

	case NPC_STATE_IDLE:
	if ( HasCondition( COND_LIGHT_DAMAGE ) || HasCondition( COND_HEAVY_DAMAGE ) ) 
		{
			// flinch if hurt
			return SCHED_SMALL_FLINCH;
		}

		if ( GetEnemy() == NULL && GetFollowTarget() )
		{
			if ( !GetFollowTarget()->IsAlive() )
			{
				StopFollowing();
				break;
			}
			else
			{

				return SCHED_TARGET_FACE;
			}
		}

		// try to say something about smells
		break;
	}
	
	return BaseClass::SelectSchedule();
}

NPC_STATE CNPC_HL1AllyPeters::SelectIdealState ( void )
{
	return BaseClass::SelectIdealState();
}

void CNPC_HL1AllyPeters::DeclineFollowing( void )
{
	if ( CanSpeakAfterMyself() )
	{
		Speak( "SLV_DECLINEFOLLOW" );
	}
}

int CNPC_HL1AllyPeters::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{

	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		{
			int	baseType;

			// call base class default so that scientist will talk
			// when 'used' 
			baseType = BaseClass::TranslateSchedule( scheduleType );
			
			if ( baseType == SCHED_IDLE_STAND )
				return SCHED_PETERS_FACE_TARGET;
			else
				return baseType;
		}
		break;

	case SCHED_TARGET_CHASE:
	{
		return SCHED_PETERS_FOLLOW;
		break;
	}

	case SCHED_IDLE_STAND:
		{
			int	baseType;

			// call base class default so that scientist will talk
			// when 'used' 
			baseType = BaseClass::TranslateSchedule( scheduleType );
			
			if ( baseType == SCHED_IDLE_STAND )
				return SCHED_PETERS_IDLE_STAND;
			else
				return baseType;
		}
		break;


	case SCHED_CHASE_ENEMY:
		{
			if ( HasCondition( COND_HEAVY_DAMAGE ) )
				 return SCHED_TAKE_COVER_FROM_ENEMY;

			// No need to take cover since I can see him
			// SHOOT!
			if ( HasCondition( COND_CAN_RANGE_ATTACK1 ) )
				 return SCHED_RANGE_ATTACK1;
		}
		break;

	}



	//Oops can't get to my enemy.
	if ( scheduleType == SCHED_CHASE_ENEMY_FAILED )
	{
		return SCHED_ESTABLISH_LINE_OF_FIRE;
	}

	switch	( scheduleType )
	{
	case SCHED_FAIL:

		if ( HasCondition( COND_CAN_MELEE_ATTACK1 ) )
		{
	  		 return ( SCHED_MELEE_ATTACK1 );
		}

		break;

	case SCHED_RANGE_ATTACK1:
		{
			//Adrian - HACK HACK! This should've been done up there ^^^^
			if ( HasCondition( COND_CAN_MELEE_ATTACK1 ) )
			{
	  			return ( SCHED_MELEE_ATTACK1 );
			}
		
			return SCHED_PETERS_ATTACK;
		}

		break;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}

//=========================================================
// ArmBeam - small beam from arm to nearby geometry
//=========================================================
void CNPC_HL1AllyPeters::ArmBeam( int side )
{
	trace_t tr;
	float flDist = 1.0;
	
	if ( m_iBeams >= PETERS_MAX_BEAMS )
		 return;

	Vector forward, right, up;
	Vector vecAim;
	AngleVectors( GetAbsAngles(), &forward, &right, &up );
	Vector vecSrc = GetAbsOrigin() + up * 36 + right * side * 16 + forward * 32;

	for (int i = 0; i < 3; i++)
	{
		vecAim = right * side * random->RandomFloat( 0, 1 ) + up * random->RandomFloat( -1, 1 );
		trace_t tr1;
		UTIL_TraceLine ( vecSrc, vecSrc + vecAim * 512, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr1);
		if (flDist > tr1.fraction)
		{
			tr = tr1;
			flDist = tr.fraction;
		}
	}

	// Couldn't find anything close enough
	if ( flDist == 1.0 )
		 return;

	if( tr.m_pEnt && tr.m_pEnt->m_takedamage && !tr.m_pEnt->IsNPC() )
	{
		CTakeDamageInfo info( this, this, 10, DMG_SHOCK );
		CalculateMeleeDamageForce( &info, vecAim, tr.endpos );

		tr.m_pEnt->TakeDamage( info );
	}

	UTIL_DecalTrace( &tr, "FadingScorch" );

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.vmt", 3.0f );

	if ( m_pBeam[m_iBeams] == NULL )
		 return;

	m_pBeam[m_iBeams]->PointEntInit( tr.endpos, this );
	m_pBeam[m_iBeams]->SetEndAttachment( side < 0 ? 2 : 1 );

	m_pBeam[m_iBeams]->SetColor( 0, 255, 255 );

	m_pBeam[m_iBeams]->SetBrightness( 64 );
	m_pBeam[m_iBeams]->SetNoise( 12.8 );
	m_pBeam[m_iBeams]->AddSpawnFlags( SF_BEAM_TEMPORARY );	

	m_iBeams++;
}

//=========================================================
// BeamGlow - brighten all beams
//=========================================================
void CNPC_HL1AllyPeters::BeamGlow( )
{
	int b = m_iBeams * 32;
	
	if ( b > 255 )
		 b = 255;

	for ( int i = 0; i < m_iBeams; i++ )
	{
		if ( m_pBeam[i] != NULL )
		{
			if ( m_pBeam[i]->GetBrightness() != 255 ) 
				m_pBeam[i]->SetBrightness( b );
		}
	}
}

//=========================================================
// WackBeam - regenerate dead colleagues
//=========================================================
void CNPC_HL1AllyPeters::WackBeam( int side, CBaseEntity *pEntity )
{
	Vector vecDest;
	
	if ( m_iBeams >= PETERS_MAX_BEAMS )
		 return;

	if ( pEntity == NULL )
		 return;

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.vmt", 3.0f );
	if ( m_pBeam[m_iBeams] == NULL )
		 return;

	m_pBeam[m_iBeams]->PointEntInit( pEntity->WorldSpaceCenter(), this );
	m_pBeam[m_iBeams]->SetEndAttachment( side < 0 ? 2 : 1 );
	m_pBeam[m_iBeams]->SetColor( 0, 255, 255 );
	m_pBeam[m_iBeams]->SetBrightness( 255 );
	m_pBeam[m_iBeams]->SetNoise( 12.8 );
	m_pBeam[m_iBeams]->AddSpawnFlags( SF_BEAM_TEMPORARY );
	m_iBeams++;
}

//=========================================================
// ZapBeam - heavy damage directly forward
//=========================================================
void CNPC_HL1AllyPeters::ZapBeam( int side )
{
	Vector vecSrc, vecAim;
	trace_t tr;
	CBaseEntity *pEntity;

	if ( m_iBeams >= PETERS_MAX_BEAMS )
		 return;

	Vector forward, right, up;
	AngleVectors( GetAbsAngles(), &forward, &right, &up );

	vecSrc = GetAbsOrigin() + up * 36;
	vecAim = GetShootEnemyDir( vecSrc );
	float deflection = 0.01;
	vecAim = vecAim + side * right * random->RandomFloat( 0, deflection ) + up * random->RandomFloat( -deflection, deflection );
	UTIL_TraceLine ( vecSrc, vecSrc + vecAim * 1024, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr);
	
	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.vmt", 5.0f );
	if ( m_pBeam[m_iBeams] == NULL )
		 return;

	m_pBeam[m_iBeams]->PointEntInit( tr.endpos, this );
	m_pBeam[m_iBeams]->SetEndAttachment( side < 0 ? 2 : 1 );
	m_pBeam[m_iBeams]->SetColor( 0, 255, 255 );
	m_pBeam[m_iBeams]->SetBrightness( 255 );
	m_pBeam[m_iBeams]->SetNoise( 3.2f );
	m_pBeam[m_iBeams]->AddSpawnFlags( SF_BEAM_TEMPORARY );
	m_iBeams++;

	pEntity = tr.m_pEnt;

	if ( pEntity != NULL && m_takedamage )
	{
		CTakeDamageInfo info( this, this, sk_islave_ally_peters_dmg_zap.GetFloat(), DMG_SHOCK );
		CalculateMeleeDamageForce( &info, vecAim, tr.endpos );
		pEntity->DispatchTraceAttack( info, vecAim, &tr );
	}
}

//=========================================================
// ClearBeams - remove all beams
//=========================================================
void CNPC_HL1AllyPeters::ClearBeams( )
{
	for (int i = 0; i < PETERS_MAX_BEAMS; i++)
	{
		if (m_pBeam[i])
		{
			UTIL_Remove( m_pBeam[i] );
			m_pBeam[i] = NULL;
		}
	}

	m_iBeams = 0;
	m_nSkin = 0;
}


//------------------------------------------------------------------------------
//
// Schedules
//
//------------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( monster_alien_slave_ally_peters, CNPC_HL1AllyPeters )

	//=========================================================
	// > SCHED_PETERS_ATTACK
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_PETERS_ATTACK,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_FACE_IDEAL				0"
		"		TASK_RANGE_ATTACK1			0"
		"	"
		"	Interrupts"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
	)

	DEFINE_SCHEDULE
	(
		SCHED_PETERS_FOLLOW,
	
		"	Tasks"
//		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_BARNEY_STOP_FOLLOWING"
		"		TASK_GET_PATH_TO_TARGET			0"
		"		TASK_MOVE_TO_TARGET_RANGE		180"
		"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_TARGET_FACE"
		"	"
		"	Interrupts"
		"			COND_NEW_ENEMY"
		"			COND_LIGHT_DAMAGE"
		"			COND_HEAVY_DAMAGE"
		"			COND_HEAR_DANGER"
		"			COND_PROVOKED"
	)
	
	//=========================================================
	// > SCHED_BARNEY_ENEMY_DRAW
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_PETERS_ENEMY_DRAW,
	
		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_FACE_ENEMY				0"
		"		TASK_PLAY_SEQUENCE_FACE_ENEMY		ACTIVITY:ACT_ARM"
		"	"
		"	Interrupts"
	)
	
	//=========================================================
	// > SCHED_BARNEY_FACE_TARGET
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_PETERS_FACE_TARGET,
	
		"	Tasks"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
		"		TASK_FACE_TARGET			0"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
		"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_PETERS_FOLLOW"
		"	"
		"	Interrupts"
		"		COND_GIVE_WAY"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_PROVOKED"
		"		COND_HEAR_DANGER"
	)
	
	//=========================================================
	// > SCHED_BARNEY_IDLE_STAND
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_PETERS_IDLE_STAND,
	
		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
		"		TASK_WAIT					2"
		"		TASK_TALKER_HEADRESET		0"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_PROVOKED"
		"		COND_HEAR_COMBAT"
		"		COND_SMELL"
	)

AI_END_CUSTOM_NPC()
