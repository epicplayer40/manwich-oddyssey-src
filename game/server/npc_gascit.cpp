
//=========================================================
// monster template
//=========================================================
// UNDONE: Holster weapon?

#include	"cbase.h"

//#if 0

#include	"npc_talker.h"
#include	"ai_schedule.h"
#include	"scripted.h"
#include	"basecombatweapon.h"
#include	"soundent.h"
#include	"NPCEvent.h"
#include	"AI_Hull.h"
#include	"AI_Node.h"
#include	"AI_Network.h"
#include	"ai_hint.h"
#include	"ai_squad.h"
#include	"npc_gascit.h"
#include	"activitylist.h"
#include "AI_Interactions.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include	"hl1/hl1_basegrenade.h"

ConVar	sk_gascit_health( "sk_gascit_health","100");
ConVar	sk_gascit_model( "sk_gascit_model", "models/gascit.mdl" );


#define GASCIT_MAD 		"GASCIT_MAD"
#define GASCIT_SHOT 		"GASCIT_SHOT"
#define GASCIT_KILL 		"GASCIT_KILL"
#define GASCIT_OUT_AMMO 	"GASCIT_OUT_AMMO"
#define GASCIT_ATTACK 	"GASCIT_ATTACK"
#define GASCIT_LINE_FIRE "GASCIT_LINE_FIRE"
#define GASCIT_POK 		"GASCIT_POK"

#define GASCIT_PAIN1		"GASCIT_PAIN1"
#define GASCIT_PAIN2		"GASCIT_PAIN2"
#define GASCIT_PAIN3		"GASCIT_PAIN3"

#define GASCIT_DIE1		"GASCIT_DIE1"
#define GASCIT_DIE2		"GASCIT_DIE2"
#define GASCIT_DIE3		"GASCIT_DIE3"

//=========================================================
// Combine activities
//=========================================================
int	ACT_GASCIT_AIM;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
// first flag is barney dying for scripted sequences?
#define		GASCIT_AE_RELOAD		( 1 )
#define		GASCIT_AE_DRAW		( 2 )
#define		GASCIT_AE_SHOOT		( 3 )
#define		GASCIT_AE_HOLSTER	( 4 )
#define		GASCIT_AE_GRENADETHROW	( 5 )

#define		GASCIT_BODY_GUNHOLSTERED	0
#define		GASCIT_BODY_GUNDRAWN		1
#define		GASCIT_BODY_GUNGONE		2

//-----------------------------------------------------------------------------
// Purpose: Initialize the custom schedules
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Gascit::InitCustomSchedules(void) 
{
	INIT_CUSTOM_AI(CNPC_Gascit);

	ADD_CUSTOM_TASK(CNPC_Gascit,	TASK_GASCIT_CROUCH);

	ADD_CUSTOM_SCHEDULE(CNPC_Gascit,	SCHED_GASCIT_FOLLOW);
	ADD_CUSTOM_SCHEDULE(CNPC_Gascit,	SCHED_GASCIT_DRAW);
	ADD_CUSTOM_SCHEDULE(CNPC_Gascit,	SCHED_GASCIT_FACE_TARGET);
	ADD_CUSTOM_SCHEDULE(CNPC_Gascit,	SCHED_GASCIT_STAND);
	ADD_CUSTOM_SCHEDULE(CNPC_Gascit,	SCHED_GASCIT_AIM);
//	ADD_CUSTOM_SCHEDULE(CNPC_Gascit,	SCHED_GASCIT_MELEE_CHARGE);
	ADD_CUSTOM_SCHEDULE(CNPC_Gascit,	SCHED_GASCIT_ROLL1);
	ADD_CUSTOM_SCHEDULE(CNPC_Gascit,	SCHED_GASCIT_ROLL2);
	ADD_CUSTOM_SCHEDULE(CNPC_Gascit,	SCHED_GASCIT_BARNACLE_HIT);
	ADD_CUSTOM_SCHEDULE(CNPC_Gascit,	SCHED_GASCIT_BARNACLE_PULL);
	ADD_CUSTOM_SCHEDULE(CNPC_Gascit,	SCHED_GASCIT_BARNACLE_CHOMP);
	ADD_CUSTOM_SCHEDULE(CNPC_Gascit,	SCHED_GASCIT_BARNACLE_CHEW);

	ADD_CUSTOM_ACTIVITY(CNPC_Combine,	ACT_GASCIT_AIM);

	AI_LOAD_SCHEDULE(CNPC_Gascit,	SCHED_GASCIT_FOLLOW);
	AI_LOAD_SCHEDULE(CNPC_Gascit,	SCHED_GASCIT_DRAW);
	AI_LOAD_SCHEDULE(CNPC_Gascit,	SCHED_GASCIT_FACE_TARGET);
	AI_LOAD_SCHEDULE(CNPC_Gascit,	SCHED_GASCIT_STAND);
	AI_LOAD_SCHEDULE(CNPC_Gascit,	SCHED_GASCIT_AIM);
	AI_LOAD_SCHEDULE(CNPC_Gascit,	SCHED_GASCIT_ROLL1);
	AI_LOAD_SCHEDULE(CNPC_Gascit,	SCHED_GASCIT_ROLL2);
//	AI_LOAD_SCHEDULE(CNPC_Gascit,	SCHED_GASCIT_MELEE_CHARGE);
	AI_LOAD_SCHEDULE(CNPC_Gascit,	SCHED_GASCIT_BARNACLE_HIT);
	AI_LOAD_SCHEDULE(CNPC_Gascit,	SCHED_GASCIT_BARNACLE_PULL);
	AI_LOAD_SCHEDULE(CNPC_Gascit,	SCHED_GASCIT_BARNACLE_CHOMP);
	AI_LOAD_SCHEDULE(CNPC_Gascit,	SCHED_GASCIT_BARNACLE_CHEW);

}

LINK_ENTITY_TO_CLASS( npc_gascit, CNPC_Gascit );
IMPLEMENT_CUSTOM_AI( npc_gascit, CNPC_Gascit );

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_Gascit )

	DEFINE_FIELD( m_fGunDrawn,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_painTime,			FIELD_TIME ),
	DEFINE_FIELD( m_checkAttackTime,	FIELD_TIME ),
	DEFINE_FIELD( m_nextLineFireTime,	FIELD_TIME ),
	DEFINE_FIELD( m_lastAttackCheck,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bInBarnacleMouth,	FIELD_BOOLEAN ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Gascit::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		if (GetEnemy() != NULL && (GetEnemy()->IsPlayer()))
		{
			m_flPlaybackRate = 1.5;
		}
		BaseClass::RunTask( pTask );
		break;
	default:
		BaseClass::RunTask( pTask );
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
Activity CNPC_Gascit::NPC_TranslateActivity( Activity eNewActivity )
{
		if (m_iHealth <= 15 && eNewActivity == ACT_RUN)
		{
			// limp!
			return ACT_WALK_HURT;
		}


		if (m_iHealth <= 15 && eNewActivity == ACT_WALK )
		{
			// limp!
			return ACT_WALK_HURT;
		}
		
	
	return eNewActivity;

	return BaseClass::NPC_TranslateActivity( eNewActivity );
}


//=========================================================
// GetSoundInterests - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CNPC_Gascit::GetSoundInterests ( void) 
{
	return	SOUND_WORLD		|
			SOUND_COMBAT	|
			SOUND_CARCASS	|
			SOUND_MEAT		|
			SOUND_GARBAGE	|
			SOUND_DANGER	|
			SOUND_PLAYER;
}

int CNPC_Gascit::RangeAttack2Conditions(float flDot, float flDist)
{
	m_iLastGrenadeCondition = GetGrenadeConditions(flDot, flDist);
	return m_iLastGrenadeCondition;
}

int CNPC_Gascit::GetGrenadeConditions(float flDot, float flDist)
{

	// assume things haven't changed too much since last time
	if (gpGlobals->curtime < m_flNextGrenadeCheck)
		return m_iLastGrenadeCondition;

	if (m_flGroundSpeed != 0)
		return COND_NONE;

	CBaseEntity *pEnemy = GetEnemy();

	if (!pEnemy)
		return COND_NONE;

	Vector flEnemyLKP = GetEnemyLKP();
	if (!(pEnemy->GetFlags() & FL_ONGROUND) && pEnemy->GetWaterLevel() == 0 && flEnemyLKP.z > (GetAbsOrigin().z + WorldAlignMaxs().z))
	{
		//!!!BUGBUG - we should make this check movetype and make sure it isn't FLY? Players who jump a lot are unlikely to 
		// be grenaded.
		// don't throw grenades at anything that isn't on the ground!
		return COND_NONE;
	}

	Vector vecTarget;


		// find feet
		if (random->RandomInt(0, 1))
		{
			// magically know where they are
			pEnemy->CollisionProp()->NormalizedToWorldSpace(Vector(0.5f, 0.5f, 0.0f), &vecTarget);
		}
		else
		{
			// toss it to where you last saw them
			vecTarget = flEnemyLKP;
		}


	// are any of my squad members near the intended grenade impact area?
/*	if (m_pSquad)
	{
		if (m_pSquad->SquadMemberInRange(vecTarget, 256))
		{
			// crap, I might blow my own guy up. Don't throw a grenade and don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->curtime + 1; // one full second.
			return COND_NONE;
		}
	}*/

	if ((vecTarget - GetAbsOrigin()).Length2D() <= 256)
	{
		// crap, I don't want to blow myself up
		m_flNextGrenadeCheck = gpGlobals->curtime + 1; // one full second.
		return COND_NONE;
	}



		Vector vGunPos;
		QAngle angGunAngles;
		GetAttachment("anim_attachment_Rhand", vGunPos, angGunAngles);


		Vector vecToss = VecCheckToss(this, vGunPos, vecTarget, -1, 0.5, false);

		if (vecToss != vec3_origin)
		{
			m_vecTossVelocity = vecToss;

			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->curtime + 0.3; // 1/3 second.

			return COND_CAN_RANGE_ATTACK2;
		}

		{
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->curtime + 1; // one full second.

			return COND_NONE;
		}
	}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
Class_T	CNPC_Gascit::Classify ( void )
{
	return	CLASS_CONSCRIPT;
}

//=========================================================
// ALertSound - barney says "Freeze!"
//=========================================================
void CNPC_Gascit::AlertSound( void )
{
	if ( GetEnemy() != NULL )
	{
		if ( IsOkToCombatSpeak() )
		{
			Speak( GASCIT_ATTACK );
		}
	}

}
//=========================================================
// MaxYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
float CNPC_Gascit::MaxYawSpeed ( void )
{
	switch ( GetActivity() )
	{
	case ACT_IDLE:		
		return 70;
		break;
	case ACT_WALK:
		return 70;
		break;
	case ACT_RUN:
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
void CNPC_Gascit::GascitFirePistol ( void )
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
void CNPC_Gascit::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
	case GASCIT_AE_RELOAD:
		
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

	case GASCIT_AE_SHOOT:
		GascitFirePistol();
		break;

		case GASCIT_AE_GRENADETHROW:
		{
			CHandGrenade *pGrenade = (CHandGrenade*)Create("grenade_molotov", GetAbsOrigin() + Vector(0, 0, 60), vec3_angle);
			if (pGrenade)
			{
				pGrenade->ShootTimed(this, m_vecTossVelocity, 3.5);
			}

			m_iLastGrenadeCondition = COND_NONE;
			m_flNextGrenadeCheck = gpGlobals->curtime + 2;// wait six seconds before even looking again to see if a grenade can be thrown.

			Msg("Gasmask Citizen threw molotov!\n");
		}
		break;

	case GASCIT_AE_DRAW:
		// barney's bodygroup switches here so he can pull gun from holster
		m_nBody = GASCIT_BODY_GUNDRAWN;
		m_fGunDrawn = true;
		break;

	case GASCIT_AE_HOLSTER:
		// change bodygroup to replace gun in holster
		m_nBody = GASCIT_BODY_GUNHOLSTERED;
		m_fGunDrawn = false;
		break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// Spawn
//=========================================================
void CNPC_Gascit::Spawn()
{
	Precache( );

	SetModel( sk_gascit_model.GetString());
	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	SetBloodColor( BLOOD_COLOR_RED );
	m_iHealth			= sk_gascit_health.GetFloat();
	SetViewOffset( Vector ( 0, 0, 50 ) );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_NPCState			= NPC_STATE_NONE;

	m_nBody			= 0; // gun in holster
	m_fGunDrawn			= false;
	m_bInBarnacleMouth	= false;

	m_nextLineFireTime	= 0;

	m_HackedGunPos		= Vector ( 0, 0, 55 );

	CapabilitiesAdd( bits_CAP_TURN_HEAD | bits_CAP_MOVE_GROUND | bits_CAP_MOVE_JUMP | bits_CAP_MOVE_CLIMB | bits_CAP_NO_HIT_PLAYER | bits_CAP_FRIENDLY_DMG_IMMUNE | bits_CAP_ANIMATEDFACE );
	CapabilitiesAdd	( bits_CAP_USE_WEAPONS | bits_CAP_SQUAD | bits_CAP_INNATE_RANGE_ATTACK2 | bits_CAP_MOVE_SHOOT | bits_CAP_AIM_GUN );
	CapabilitiesAdd	( bits_CAP_DUCK );			// In reloading and cover

			if( Weapon_OwnsThisType( "weapon_stunstick" ) )
			{
				CapabilitiesRemove (bits_CAP_INNATE_RANGE_ATTACK2);
			}

	NPCInit();
	SetUse( &CNPCSimpleTalker::FollowerUse );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CNPC_Gascit::Precache()
{
	engine->PrecacheModel("models/gascit.mdl");
	engine->PrecacheModel( sk_gascit_model.GetString() );

	enginesound->PrecacheSound("barney/ba_pain1.wav");
	enginesound->PrecacheSound("barney/ba_pain2.wav");
	enginesound->PrecacheSound("barney/ba_pain3.wav");

	enginesound->PrecacheSound("barney/ba_die1.wav");
	enginesound->PrecacheSound("barney/ba_die2.wav");
	enginesound->PrecacheSound("barney/ba_die3.wav");
	
	enginesound->PrecacheSound("barney/ba_close.wav");

	UTIL_PrecacheOther("grenade_molotov");

	// every new barney must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	BaseClass::Precache();
}	

// Init talk data
void CNPC_Gascit::TalkInit()
{
	
	BaseClass::TalkInit();

		// vortigaunt will try to talk to friends in this order:
	m_szFriends[0] = "npc_gascit";
	m_szFriends[1] = "npc_vortigaunt";

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
int	CNPC_Gascit::OnTakeDamage_Alive( const CTakeDamageInfo &info )
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
				Speak( GASCIT_MAD );

				Remember( bits_MEMORY_PROVOKED );

				// Allowed to hit the player now!
				CapabilitiesRemove(bits_CAP_NO_HIT_PLAYER);

				StopFollowing();
			}
			else
			{
				// Hey, be careful with that
				Speak( GASCIT_SHOT );
				Remember( bits_MEMORY_SUSPICIOUS );
			}
		}
		else if ( !(GetEnemy()->IsPlayer()) && (m_lifeState != LIFE_DEAD ))
		{
			Speak( GASCIT_SHOT );
		}
	}
	return ret;
}

//------------------------------------------------------------------------------
// Purpose : Override to always shoot at eyes (for ducking behind things)
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CNPC_Gascit::BodyTarget( const Vector &posSrc, bool bNoisy ) 
{
	return EyePosition();
}

//=========================================================
// PainSound
//=========================================================
void CNPC_Gascit::PainSound ( void )
{
	if (gpGlobals->curtime < m_painTime)
		return;

	AIConcept_t concepts[] =
	{
		GASCIT_PAIN1,
		GASCIT_PAIN2,
		GASCIT_PAIN3,
	};
	
	m_painTime = gpGlobals->curtime + random->RandomFloat(0.5, 0.75);

	Speak( concepts[random->RandomInt(0,2)] );
}

//=========================================================
// DeathSound 
//=========================================================
void CNPC_Gascit::DeathSound ( void )
{
	AIConcept_t concepts[] =
	{
		GASCIT_DIE1,
		GASCIT_DIE2,
		GASCIT_DIE3,
	};
	
	Speak( concepts[random->RandomInt(0,2)] );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Gascit::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr )
{
	CTakeDamageInfo info = inputInfo;

	switch( ptr->hitgroup)
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if (info.GetDamageType() & (DMG_BULLET | DMG_SLASH | DMG_BLAST))
		{
			info.ScaleDamage( 0.5f );
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

	CTakeDamageInfo newInfo = info;
	if( newInfo.GetDamageType() & DMG_BURN)
	{
		newInfo.ScaleDamage( 0.10 );
	}

	if( newInfo.GetDamageType() & DMG_DISSOLVE)
	{
		newInfo.ScaleDamage( 0.05 );
	}

	if( newInfo.GetDamageType() & DMG_NERVEGAS)
	{
		newInfo.ScaleDamage( 0.0 );
	}	

	//BaseClass::TraceAttack( info, vecDir, ptr );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_Gascit::TranslateSchedule( int scheduleType )
{
	int baseType;

	switch( scheduleType )
	{
	case SCHED_GASCIT_DRAW:
		if ( GetEnemy() != NULL )
		{
			// face enemy, then draw.
			return SCHED_GASCIT_DRAW;
		}
		else
		{
			// BUGBUG: What is this code supposed to do when there isn't an enemy?
			Warning( "BUG IN GASCIT AI!\n");
		}
		break;

	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		{
			// call base class default so that barney will talk
			// when 'used' 
			baseType = BaseClass::TranslateSchedule(scheduleType);

			if (baseType == SCHED_IDLE_STAND)
				return SCHED_GASCIT_FACE_TARGET;	// override this for different target face behavior
			else
				return baseType;
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
		return SCHED_GASCIT_FOLLOW;
		break;

	case SCHED_IDLE_STAND:
		{
			// call base class default so that scientist will talk
			// when standing during idle
			baseType = BaseClass::TranslateSchedule(scheduleType);

			if (baseType == SCHED_IDLE_STAND)
			{
				// just look straight ahead
				return SCHED_GASCIT_STAND;
			}
			return baseType;
			break;

		}
	case SCHED_FAIL_ESTABLISH_LINE_OF_FIRE:
		{
				return SCHED_GASCIT_ROLL2;
		}
		break;

	}
	return BaseClass::TranslateSchedule( scheduleType );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_Gascit::SelectSchedule ( void )
{
	// These things are done in any state but dead and prone
	if (m_NPCState != NPC_STATE_DEAD && m_NPCState != NPC_STATE_PRONE)
	{
		if ( HasCondition( COND_HEAR_DANGER ) )
		{
			return SCHED_TAKE_COVER_FROM_BEST_SOUND;
		}

//		if ( HasCondition( COND_HEAR_DANGER ) )
//		{
//				return SCHED_GASCIT_ROLL2;
//		}
		if ( HasCondition( COND_ENEMY_DEAD ) && IsOkToCombatSpeak() )
		{
			Speak( GASCIT_KILL );
		}
	}
	switch( m_NPCState )
	{
	case NPC_STATE_PRONE:
		{
			if (m_bInBarnacleMouth)
			{
				return SCHED_GASCIT_BARNACLE_CHOMP;
			}
			else
			{
				return SCHED_GASCIT_BARNACLE_HIT;
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

			if ( HasCondition( COND_LIGHT_DAMAGE ) )
					return SCHED_GASCIT_ROLL2;

			// wait for one schedule to draw gun
			if (!m_fGunDrawn )
				return SCHED_GASCIT_DRAW;

			if ( HasCondition( COND_HEAVY_DAMAGE ) )
				//return SCHED_TAKE_COVER_FROM_ENEMY;
				return SCHED_GASCIT_ROLL2;

			// ---------------------
			// no ammo
			// ---------------------
			if ( HasCondition ( COND_NO_PRIMARY_AMMO ) )
			{
				Speak( GASCIT_OUT_AMMO );
				return SCHED_HIDE_AND_RELOAD;
			}
			else if (!HasCondition(COND_CAN_RANGE_ATTACK1) && HasCondition( COND_NO_SECONDARY_AMMO ))
			{
				Speak( GASCIT_OUT_AMMO );
				return SCHED_HIDE_AND_RELOAD;
			}

			/* UNDONE: check distance for genade attacks...
			// If player is next to what I'm trying to attack...
			if ( HasCondition( COND_WEAPON_PLAYER_NEAR_TARGET ))
			{
				return SCHED_GASCIT_AIM;
			}
			*/	

			// -------------------------------------------
			// If I might hit the player shooting...
			// -------------------------------------------
			if ( HasCondition( COND_WEAPON_PLAYER_IN_SPREAD ))
			{
				if ( IsOkToCombatSpeak() && m_nextLineFireTime	< gpGlobals->curtime)
				{
					Speak( GASCIT_LINE_FIRE );
					m_nextLineFireTime = gpGlobals->curtime + 3.0f;
				}

				// Run to a new location or stand and aim
				if (random->RandomInt(0,2) == 0)
				{
					return SCHED_ESTABLISH_LINE_OF_FIRE;
				}
				else
				{
					return SCHED_GASCIT_AIM;
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
		TrySmellTalk();
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
bool CNPC_Gascit::HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt)
{
	if (interactionType == g_interactionBarnacleVictimDangle)
	{
		// Force choosing of a new schedule
		ClearSchedule("Released by Barnacle");
		m_bInBarnacleMouth	= true;
		return true;
	}
	else if ( interactionType == g_interactionBarnacleVictimReleased )
	{
		m_IdealNPCState = NPC_STATE_IDLE;

		CPASAttenuationFilter filter( this );

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
		PainSound();
		return true;
	}
	return false;
}

void CNPC_Gascit::DeclineFollowing( void )
{
	Speak( GASCIT_POK );
}

WeaponProficiency_t CNPC_Gascit::CalcWeaponProficiency( CBaseCombatWeapon *pWeapon )
{
	if( FClassnameIs( pWeapon, "weapon_pistol" ) )
	{
		return WEAPON_PROFICIENCY_PERFECT;
	}

	if( FClassnameIs( pWeapon, "weapon_smg1" ) )
	{
		return WEAPON_PROFICIENCY_VERY_GOOD;
	}

	if( FClassnameIs( pWeapon, "weapon_oicw" ) )
	{
		return WEAPON_PROFICIENCY_PERFECT;
	}


	if( FClassnameIs( pWeapon, "weapon_tommygun" ) )
	{
		return WEAPON_PROFICIENCY_VERY_GOOD;
	}

	if( FClassnameIs( pWeapon, "weapon_dbarrel" ) )
	{
		return WEAPON_PROFICIENCY_PERFECT;
	}

	else
	{
		return WEAPON_PROFICIENCY_GOOD;
	}

	return BaseClass::CalcWeaponProficiency( pWeapon );
}


//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

//=========================================================
// > SCHED_GASCIT_FOLLOW
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_GASCIT_FOLLOW,

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
//  > SCHED_GASCIT_DRAW
//		much better looking draw schedule for when
//		gascit knows who he's gonna attack.
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_GASCIT_DRAW,

	"	Tasks"
	"		 TASK_STOP_MOVING					0"
	"		 TASK_FACE_ENEMY					0"
	"		 TASK_PLAY_SEQUENCE_FACE_ENEMY		ACTIVITY:ACT_ARM "
	""
	"	Interrupts"
);

AI_DEFINE_SCHEDULE
(
	SCHED_GASCIT_ROLL1,

	"	Tasks"
	"		 TASK_STOP_MOVING					0"
	"		 TASK_FACE_ENEMY					0"
	"		 TASK_PLAY_SEQUENCE_FACE_ENEMY		ACTIVITY:ACT_SIGNAL1"
	"		 TASK_FIND_COVER_FROM_ENEMY		0"
	"		 TASK_RUN_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"		TASK_FACE_ENEMY					0"
	"		TASK_WAIT_RANDOM				3"
	"	"
	
	""
	"	Interrupts"
);

AI_DEFINE_SCHEDULE
(
	SCHED_GASCIT_ROLL2,

	"	Tasks"
	"		 TASK_STOP_MOVING					0"
	"		 TASK_FACE_ENEMY					0"
	"		 TASK_PLAY_SEQUENCE			ACTIVITY:ACT_SIGNAL2"
	"		 TASK_FIND_COVER_FROM_ENEMY		0"
	"		 TASK_RUN_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"		TASK_FACE_ENEMY					0"
	"		TASK_WAIT_RANDOM				3"
	"	"
	
	""
	"	Interrupts"
);

//===============================================
//	> SCHED_GASCIT_AIM
//
//	Stand in place and aim at enemy (used when
//  line of sight blocked by player)
//===============================================
AI_DEFINE_SCHEDULE
(
	SCHED_GASCIT_AIM,

	"	Tasks"
	"		TASK_STOP_MOVING		0"
	"		TASK_FACE_ENEMY			0"
	"		TASK_PLAY_SEQUENCE		ACTIVITY:ACT_CONSCRIPT_AIM"
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
// > SCHED_GASCIT_FACE_TARGET
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_GASCIT_FACE_TARGET,

	"	Tasks"
	"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
	"		TASK_FACE_TARGET			0"
	"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
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
// > SCHED_GASCIT_STAND
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_GASCIT_STAND,

	"	Tasks"
	"		TASK_STOP_MOVING			0"
	"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE "
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

/*AI_DEFINE_SCHEDULE
(
		SCHED_GASCIT_MELEE_CHARGE,

		"	Tasks"
		"		 TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY_FAILED"
		"		 TASK_SET_TOLERANCE_DISTANCE	14"
		"		 TASK_GET_CHASE_PATH_TO_ENEMY	600"
		"		 TASK_RUN_PATH					0"
		"		 TASK_WAIT_FOR_MOVEMENT			0"
		"		 TASK_FACE_ENEMY				0"
		"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_MELEE_ATTACK1"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_ENEMY_UNREACHABLE"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK1"
		//"		COND_CAN_RANGE_ATTACK2"
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_TOO_CLOSE_TO_ATTACK"
		"		COND_TASK_FAILED"
		"		COND_HEAVY_DAMAGE"
);*/

//=========================================================
// > SCHED_GASCIT_BARNACLE_HIT
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_GASCIT_BARNACLE_HIT,

	"	Tasks"
	"		TASK_STOP_MOVING			0"
	"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_BARNACLE_HIT"
	"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_GASCIT_BARNACLE_PULL"
	""
	"	Interrupts"
);

//=========================================================
// > SCHED_GASCIT_BARNACLE_PULL
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_GASCIT_BARNACLE_PULL,

	"	Tasks"
	"		 TASK_PLAY_SEQUENCE			ACTIVITY:ACT_BARNACLE_PULL"
	""
	"	Interrupts"
);

//=========================================================
// > SCHED_GASCIT_BARNACLE_CHOMP
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_GASCIT_BARNACLE_CHOMP,

	"	Tasks"
	"		 TASK_PLAY_SEQUENCE			ACTIVITY:ACT_BARNACLE_CHOMP"
	"		 TASK_SET_SCHEDULE			SCHEDULE:SCHED_GASCIT_BARNACLE_CHEW"
	""
	"	Interrupts"
);

//=========================================================
// > SCHED_GASCIT_BARNACLE_CHEW
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_GASCIT_BARNACLE_CHEW,

	"	Tasks"
	"		 TASK_PLAY_SEQUENCE			ACTIVITY:ACT_BARNACLE_CHEW"
);


//#endif
