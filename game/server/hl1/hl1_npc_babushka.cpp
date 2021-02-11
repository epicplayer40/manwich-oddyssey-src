//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Well-meaning citizens arming themselves to keep the crime-ridden streets of Mars safe 
//			This version carries an OICW around
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//


//TODO:
//1: Add an env_global flag that makes these guys hostile to the player and npc_gascit (not yet implemented)
//2: Remove firerate increase when hostile to player (not yet implemented)
//3: Make them put their guns away again when threats are eliminated like in HL1 (not yet implemented)
//4: Make a version with its own CLASS_ faction for each of the 5 variants for after the militia breaks off, monster_babushka_team1 with CLASS_TEAM1, monster_babushka_team2 with CLASS_TEAM2, etc. (Haven't started yet, it'll be tedious)
//5: Make an option to force the character to use a specific model in hammer since there are 2 different animsets for the 5 variants (not yet implemented)

#include	"cbase.h"
#include	"AI_Default.h"
#include	"AI_Task.h"
#include	"AI_Schedule.h"
#include	"AI_Node.h"
#include	"AI_Hull.h"
#include	"AI_Hint.h"
#include	"AI_Memory.h"
#include	"AI_Route.h"
#include	"AI_Motor.h"
#include	"AI_Squad.h"
#include	"AI_SquadSlot.h"
#include	"hl1_NPC_Babushka.h"
#include	"soundent.h"
#include	"game.h"
#include	"NPCEvent.h"
#include	"EntityList.h"
#include	"activitylist.h"
#include	"animation.h"
#include	"basecombatweapon.h"
#include	"IEffects.h"
#include	"vstdlib/random.h"
#include	"engine/IEngineSound.h"
#include	"ammodef.h"
#include	"AI_Behavior_Follow.h"
#include	"AI_Criteria.h"
#include	"soundemittersystem/isoundemittersystembase.h"

#include 	<time.h>

#include "effect_dispatch_data.h"
#include "te_effect_dispatch.h"

#define BA_ATTACK	"BA_ATTACK"
#define BA_MAD		"BA_MAD"
#define BA_SHOT		"BA_SHOT"
#define BA_KILL		"BA_KILL"
#define BA_POK		"BA_POK"

ConVar	sk_babushkahl1_health( "sk_babushkahl1_health","500");

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
// first flag is babushka dying for scripted sequences?
#define		BABUSHKA_AE_DRAW		( 2 )
#define		BABUSHKA_AE_SHOOT		( 3 )
#define		BABUSHKA_AE_HOLSTER	( 4 )

#define		BABUSHKA_BODY_GUNHOLSTERED	0
#define		BABUSHKA_BODY_GUNDRAWN		1
#define		BABUSHKA_BODY_GUNGONE			2


enum BabushkaSquadSlot_T
{	
	BABUSHKA_SQUAD_SLOT_ATTACK1 = LAST_SHARED_SQUADSLOT,
	BABUSHKA_SQUAD_SLOT_ATTACK2,
	BABUSHKA_SQUAD_SLOT_INVESTIGATE_SOUND,
	BABUSHKA_SQUAD_SLOT_CHASE,
};


//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_HL1Babushka )
	DEFINE_FIELD( m_fGunDrawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flPainTime, FIELD_TIME ),
	DEFINE_FIELD( m_flCheckAttackTime, FIELD_TIME ),
	DEFINE_FIELD( m_fLastAttackCheck, FIELD_BOOLEAN ),

	DEFINE_THINKFUNC( SUB_LVFadeOut ),

	//DEFINE_FIELD( m_iAmmoType, FIELD_INTEGER ),
END_DATADESC()


LINK_ENTITY_TO_CLASS( monster_babushka, CNPC_HL1Babushka );


static BOOL IsFacing( CBaseEntity *pevTest, const Vector &reference )
{
	Vector vecDir = (reference - pevTest->GetAbsOrigin());
	vecDir.z = 0;
	VectorNormalize( vecDir );
	Vector forward;
	QAngle angle;
	angle = pevTest->GetAbsAngles();
	angle.x = 0;
	AngleVectors( angle, &forward );
	// He's facing me, he meant it
	if ( DotProduct( forward, vecDir ) > 0.96 )	// +/- 15 degrees or so
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// Spawn
//=========================================================
void CNPC_HL1Babushka::Spawn()
{
	Precache( );

	SetModel( "models/squad_babushka.mdl");

	SetRenderColor( 255, 255, 255, 255 );
	
	
	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	m_bloodColor		= BLOOD_COLOR_RED;
	m_iHealth			= sk_babushkahl1_health.GetFloat();
	SetViewOffset( Vector ( 0, 0, 100 ) );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_NPCState			= NPC_STATE_NONE;

	SetBodygroup( 1, 0 );

	m_fGunDrawn			= false;

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS | bits_CAP_USE | bits_CAP_DOORS_GROUP);
	CapabilitiesAdd( bits_CAP_INNATE_RANGE_ATTACK1 | bits_CAP_TURN_HEAD | bits_CAP_ANIMATEDFACE | bits_CAP_NO_HIT_PLAYER  );
	CapabilitiesAdd( bits_CAP_SQUAD | bits_CAP_NO_HIT_SQUADMATES | bits_CAP_FRIENDLY_DMG_IMMUNE | bits_CAP_MOVE_JUMP | bits_CAP_MOVE_CLIMB  );
	
	NPCInit();
	
	SetUse( &CNPC_HL1Babushka::FollowerUse );
}

bool CNPC_HL1Babushka::IsJumpLegal(const Vector &startPos, const Vector &apex, const Vector &endPos) const
{
	const float MAX_JUMP_RISE		= 220.0f;
	const float MAX_JUMP_DISTANCE	= 512.0f;
	const float MAX_JUMP_DROP		= 384.0f;

	if ( BaseClass::IsJumpLegal( startPos, apex, endPos, MAX_JUMP_RISE, MAX_JUMP_DROP, MAX_JUMP_DISTANCE ) )
	{
		// Hang onto the jump distance. The AI is going to want it.
		m_flJumpDist = (startPos - endPos).Length();

		//If someone's at my jump destination, make them move out of the way
		CSoundEnt::InsertSound ( SOUND_DANGER, endPos, 32, 0.3 ); 

		return true;
	}
	return false;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CNPC_HL1Babushka::Precache()
{
	m_iAmmoType = GetAmmoDef()->Index("Pistol");
	PrecacheModel( STRING( GetModelName() ) );

	PrecacheModel( "models/squad_babushka.mdl" );

	PrecacheScriptSound( "Babushka.FireWeapon" );
	PrecacheScriptSound( "Babushka.Pain" );
	PrecacheScriptSound( "Babushka.Die" );

	// every new babushka must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	BaseClass::Precache();
}	

void CNPC_HL1Babushka::ModifyOrAppendCriteria( AI_CriteriaSet& criteriaSet )
{
	BaseClass::ModifyOrAppendCriteria( criteriaSet );

	bool predisaster = FBitSet( m_spawnflags, SF_NPC_PREDISASTER ) ? true : false;

	criteriaSet.AppendCriteria( "disaster", predisaster ? "[disaster::pre]" : "[disaster::post]" );
}

// Init talk data
void CNPC_HL1Babushka::TalkInit()
{
	BaseClass::TalkInit();

	// get voice for head - just one babushka voice for now
	GetExpresser()->SetVoicePitch( 100 );
}


//=========================================================
// GetSoundInterests - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CNPC_HL1Babushka::GetSoundInterests ( void) 
{
	return	SOUND_WORLD	|
			SOUND_COMBAT	|
			SOUND_CARCASS	|
			SOUND_MEAT		|
			SOUND_GARBAGE	|
			SOUND_DANGER	|
			SOUND_PLAYER;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
Class_T	CNPC_HL1Babushka::Classify ( void )
{
	return	CLASS_CONSCRIPT;
}

//=========================================================
// ALertSound - babushka says "Freeze!"
//=========================================================
void CNPC_HL1Babushka::AlertSound( void )
{
	if ( GetEnemy() != NULL )
	{
	//	if ( IsOkToSpeak() )
	//	{
			Speak( BA_ATTACK );
	//	}
	}

}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CNPC_HL1Babushka::SetYawSpeed ( void )
{
	int ys;

	ys = 0;

	switch ( GetActivity() )
	{
	case ACT_IDLE:		
		ys = 70;
		break;
	case ACT_WALK:
		ys = 70;
		break;
	case ACT_RUN:
		ys = 90;
		break;
	default:
		ys = 70;
		break;
	}

	GetMotor()->SetYawSpeed( ys );
}

//=========================================================
// CheckRangeAttack1
//=========================================================
bool CNPC_HL1Babushka::CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( gpGlobals->curtime > m_flCheckAttackTime )
	{
		trace_t tr;
		
		Vector shootOrigin = GetAbsOrigin() + Vector( 0, 0, 55 );
		CBaseEntity *pEnemy = GetEnemy();
		Vector shootTarget = ( (pEnemy->BodyTarget( shootOrigin ) - pEnemy->GetAbsOrigin()) + GetEnemyLKP() );
		
		UTIL_TraceLine ( shootOrigin, shootTarget, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr);
		m_flCheckAttackTime = gpGlobals->curtime + 1;
		if ( tr.fraction == 1.0 || ( tr.m_pEnt != NULL && tr.m_pEnt == pEnemy) )
			m_fLastAttackCheck = TRUE;
		else
			m_fLastAttackCheck = FALSE;

		m_flCheckAttackTime = gpGlobals->curtime + 1.5;
	}
	
	return m_fLastAttackCheck;
}

//------------------------------------------------------------------------------
// Purpose : For innate range attack
// Input   :
// Output  :
//------------------------------------------------------------------------------
int CNPC_HL1Babushka::RangeAttack1Conditions( float flDot, float flDist )
{
	if (GetEnemy() == NULL)
	{
		return( COND_NONE );
	}
	
	else if ( flDist > 2048 ) //1024 originally
	{
		return( COND_TOO_FAR_TO_ATTACK );
	}
	else if ( flDot < 0.5 )
	{
		return( COND_NOT_FACING_ATTACK );
	}

	if ( CheckRangeAttack1 ( flDot, flDist ) )
		return( COND_CAN_RANGE_ATTACK1 );

	return COND_NONE;
}



//=========================================================
// BabushkaFirePistol - shoots one round from the pistol at
// the enemy babushka is facing.
//=========================================================
void CNPC_HL1Babushka::BabushkaFirePistol ( void )
{
	Vector vecShootOrigin;
	
	vecShootOrigin = GetAbsOrigin() + Vector( 0, 0, 55 );
	Vector vecShootDir = GetShootEnemyDir( vecShootOrigin );

	QAngle angDir;
	
	VectorAngles( vecShootDir, angDir );
//	SetBlending( 0, angDir.x );
	DoMuzzleFlash();

	FireBullets(3, vecShootOrigin, vecShootDir, VECTOR_CONE_5DEGREES, 2048, m_iAmmoType ); //raised gun range to 1200 in accordance with range increase

	//FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 1024, m_iAmmoType ); Automatic Mode - disabled for now
	
	int pitchShift = random->RandomInt( 0, 20 );
	
	// Only shift about half the time
	if ( pitchShift > 10 )
		pitchShift = 0;
	else
		pitchShift -= 5;

	CPASAttenuationFilter filter( this );

	EmitSound_t params;
	params.m_pSoundName = "Babushka.FireWeapon";
	params.m_flVolume = 1;
	params.m_nChannel= CHAN_WEAPON;
	params.m_SoundLevel = SNDLVL_NORM;
	params.m_nPitch = 100 + pitchShift;
	EmitSound( filter, entindex(), params );

	CSoundEnt::InsertSound ( SOUND_COMBAT, GetAbsOrigin(), 384, 0.3 );

	// UNDONE: Reload?
	m_cAmmoLoaded--;// take away a bullet!
}

int CNPC_HL1Babushka::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
{
	// make sure friends talk about it if player hurts talkmonsters...
	int ret = BaseClass::OnTakeDamage_Alive( inputInfo );
	
	if ( !IsAlive() || m_lifeState == LIFE_DYING )
		  return ret;

	if ( m_NPCState != NPC_STATE_PRONE && ( inputInfo.GetAttacker()->GetFlags() & FL_CLIENT ) )
	{
		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if ( GetEnemy() == NULL )
		{
			// If the player was facing directly at me, or I'm already suspicious, get mad
			if ( HasMemory( bits_MEMORY_SUSPICIOUS ) || IsFacing( inputInfo.GetAttacker(), GetAbsOrigin() ) )
			{
				// Alright, now I'm pissed!
				Speak( BA_MAD );
				CapabilitiesRemove( bits_CAP_NO_HIT_PLAYER );
				Remember( bits_MEMORY_PROVOKED );
				StopFollowing();
			}
			else
			{
				// Hey, be careful with that
				Speak( BA_SHOT );
				Remember( bits_MEMORY_SUSPICIOUS );
			}
		}
		else if ( !(GetEnemy()->IsPlayer()) && m_lifeState == LIFE_ALIVE )
		{
			Speak( BA_SHOT );
		}
	}

	return ret;
}

//=========================================================
// PainSound
//=========================================================
void CNPC_HL1Babushka::PainSound( const CTakeDamageInfo &info )
{
	if (gpGlobals->curtime < m_flPainTime)
		return;
	
	m_flPainTime = gpGlobals->curtime + random->RandomFloat( 0.5, 0.75 );

	CPASAttenuationFilter filter( this );

	CSoundParameters params;
	if ( GetParametersForSound( "Babushka.Pain", params, NULL ) )
	{
		params.pitch = GetExpresser()->GetVoicePitch();

		EmitSound_t ep( params );

		EmitSound( filter, entindex(), ep );
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CNPC_HL1Babushka::DeathSound( const CTakeDamageInfo &info )
{
	CPASAttenuationFilter filter( this );

	CSoundParameters params;
	if ( GetParametersForSound( "Babushka.Die", params, NULL ) )
	{
		params.pitch = GetExpresser()->GetVoicePitch();

		EmitSound_t ep( params );

		EmitSound( filter, entindex(), ep );
	}
}

void CNPC_HL1Babushka::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr )
{
	CTakeDamageInfo info = inputInfo;

	switch( ptr->hitgroup )
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if ( info.GetDamageType() & (DMG_BULLET | DMG_SLASH | DMG_BLAST) )
		{
			info.ScaleDamage( 0.5f );
		}
		break;
	case 10:
		if ( info.GetDamageType() & (DMG_BULLET | DMG_SLASH | DMG_CLUB) )
		{
			info.SetDamage( info.GetDamage() - 20 );
			if ( info.GetDamage() <= 0 )
			{
				g_pEffects->Ricochet( ptr->endpos, ptr->plane.normal );
				info.SetDamage( 0.01 );
			}
		}
		// always a head shot
		ptr->hitgroup = HITGROUP_HEAD;
		break;
	}

	BaseClass::TraceAttack( info, vecDir, ptr );
}

void CNPC_HL1Babushka::Event_Killed( const CTakeDamageInfo &info )
{
	if ( m_nBody < BABUSHKA_BODY_GUNGONE )
	{
		// drop the gun!
		Vector vecGunPos;
		QAngle angGunAngles;
		CBaseEntity *pGun = NULL;

		SetBodygroup( 1, BABUSHKA_BODY_GUNGONE);

		GetAttachment( "0", vecGunPos, angGunAngles );
		
		angGunAngles.y += 180;
		pGun = DropItem( "weapon_oicw", vecGunPos, angGunAngles );
	}

	SetUse( NULL );	
	BaseClass::Event_Killed( info  );

	if ( UTIL_IsLowViolence() )
	{
		SUB_StartLVFadeOut( 0.0f );
	}
}

void CNPC_HL1Babushka::SUB_StartLVFadeOut( float delay, bool notSolid )
{
	SetThink( &CNPC_HL1Babushka::SUB_LVFadeOut );
	SetNextThink( gpGlobals->curtime + delay );
	SetRenderColorA( 255 );
	m_nRenderMode = kRenderNormal;

	if ( notSolid )
	{
		AddSolidFlags( FSOLID_NOT_SOLID );
		SetLocalAngularVelocity( vec3_angle );
	}
}

void CNPC_HL1Babushka::SUB_LVFadeOut( void  )
{
	if( VPhysicsGetObject() )
	{
		if( VPhysicsGetObject()->GetGameFlags() & FVPHYSICS_PLAYER_HELD || GetEFlags() & EFL_IS_BEING_LIFTED_BY_BARNACLE )
		{
			// Try again in a few seconds.
			SetNextThink( gpGlobals->curtime + 5 );
			SetRenderColorA( 255 );
			return;
		}
	}

	float dt = gpGlobals->frametime;
	if ( dt > 0.1f )
	{
		dt = 0.1f;
	}
	m_nRenderMode = kRenderTransTexture;
	int speed = max(3,256*dt); // fade out over 3 seconds
	SetRenderColorA( UTIL_Approach( 0, m_clrRender->a, speed ) );
	NetworkStateChanged();

	if ( m_clrRender->a == 0 )
	{
		UTIL_Remove(this);
	}
	else
	{
		SetNextThink( gpGlobals->curtime );
	}
}

void CNPC_HL1Babushka::StartTask( const Task_t *pTask )
{
	BaseClass::StartTask( pTask );	
}

void CNPC_HL1Babushka::RunTask( const Task_t *pTask )
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
//		m_flPlaybackRate = 2.5; //fire rate increased
		BaseClass::RunTask( pTask );
		break;
	}
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CNPC_HL1Babushka::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
	case BABUSHKA_AE_SHOOT:
		BabushkaFirePistol();
		break;

	case BABUSHKA_AE_DRAW:
		// babushka's bodygroup switches here so he can pull gun from holster
		SetBodygroup( 1, BABUSHKA_BODY_GUNDRAWN);
		m_fGunDrawn = true;
		break;

	case BABUSHKA_AE_HOLSTER:
		// change bodygroup to replace gun in holster
		SetBodygroup( 1, BABUSHKA_BODY_GUNHOLSTERED);
		m_fGunDrawn = false;
		break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
	}
}


//=========================================================
// AI Schedules Specific to this monster
//=========================================================

int CNPC_HL1Babushka::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{
	case SCHED_ARM_WEAPON:
		if ( GetEnemy() != NULL )
		{
			// face enemy, then draw.
			return SCHED_BABUSHKA_ENEMY_DRAW;
		}
		break;

	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		{
			int	baseType;

			// call base class default so that scientist will talk
			// when 'used' 
			baseType = BaseClass::TranslateSchedule( scheduleType );
			
			if ( baseType == SCHED_IDLE_STAND )
				return SCHED_BABUSHKA_FACE_TARGET;
			else
				return baseType;
		}
		break;

	case SCHED_TARGET_CHASE:
	{
		return SCHED_BABUSHKA_FOLLOW;
		break;
	}

	case SCHED_IDLE_STAND:
		{
			int	baseType;

			// call base class default so that scientist will talk
			// when 'used' 
			baseType = BaseClass::TranslateSchedule( scheduleType );
			
			if ( baseType == SCHED_IDLE_STAND )
				return SCHED_BABUSHKA_IDLE_STAND;
			else
				return baseType;
		}
		break;

	case SCHED_TAKE_COVER_FROM_ENEMY:
	case SCHED_CHASE_ENEMY:
		{
			if ( HasCondition( COND_HEAVY_DAMAGE ) )
				 return SCHED_TAKE_COVER_FROM_ENEMY;

			// No need to take cover since I can see him
			// SHOOT!
			if ( HasCondition( COND_CAN_RANGE_ATTACK1 ) && m_fGunDrawn )
				 return SCHED_RANGE_ATTACK1;
		}
		break;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}

//=========================================================
// SelectSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
int CNPC_HL1Babushka::SelectSchedule( void )
{
	if ( m_NPCState == NPC_STATE_COMBAT || GetEnemy() != NULL )
	{
		// Priority action!
		if (!m_fGunDrawn )
			return SCHED_ARM_WEAPON;
	}

	if ( GetFollowTarget() == NULL )
	{
		if ( HasCondition( COND_PLAYER_PUSHING ) && !(GetSpawnFlags() & SF_NPC_PREDISASTER ) )	// Player wants me to move
			return SCHED_HL1TALKER_FOLLOW_MOVE_AWAY;
	}

	if ( BehaviorSelectSchedule() )
		return BaseClass::SelectSchedule();

	if ( HasCondition( COND_HEAR_DANGER ) )
	{
		CSound *pSound;
		pSound = GetBestSound();

		ASSERT( pSound != NULL );

		if ( pSound && pSound->IsSoundType( SOUND_DANGER ) )
			return SCHED_TAKE_COVER_FROM_BEST_SOUND;
	}
	if ( HasCondition( COND_ENEMY_DEAD ))// && IsOkToSpeak() )
	{
		Speak( BA_KILL );
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
					return SCHED_COMBAT_FACE;
				}
				else
				{
					return SCHED_CHASE_ENEMY;
				}
			}

			if ( HasCondition(COND_SEE_ENEMY) )
			{
				if( HasCondition( COND_TOO_FAR_TO_ATTACK ) )
				{
					return SCHED_CHASE_ENEMY;
				}
			}
		}
		break;

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
				// UNDONE: Comment about the recently dead player here?
				StopFollowing();
				break;
			}
			else
			{

				return SCHED_TARGET_FACE;
			}
		}

		// try to say something about smells
		TrySmellTalk();
		break;
	}
	
	return BaseClass::SelectSchedule();
}

NPC_STATE CNPC_HL1Babushka::SelectIdealState ( void )
{
	return BaseClass::SelectIdealState();
}

void CNPC_HL1Babushka::DeclineFollowing( void )
{
	if ( CanSpeakAfterMyself() )
	{
		Speak( BA_POK );
	}
}

bool CNPC_HL1Babushka::CanBecomeRagdoll( void )
{
	if ( UTIL_IsLowViolence() )
	{
		return false;
	}

	return BaseClass::CanBecomeRagdoll();
}

bool CNPC_HL1Babushka::ShouldGib( const CTakeDamageInfo &info )
{
//	if ( UTIL_IsLowViolence() )
//	{
//		return false;
//	}

	return BaseClass::ShouldGib( info );
}

bool CNPC_HL1Babushka::CorpseGib( const CTakeDamageInfo &info )
{
	CEffectData	data;
	
	data.m_vOrigin = WorldSpaceCenter();
	data.m_vNormal = data.m_vOrigin - info.GetDamagePosition();
	VectorNormalize( data.m_vNormal );
	
	data.m_flScale = RemapVal( m_iHealth, 0, -500, 1, 3 );
	data.m_flScale = clamp( data.m_flScale, 1, 3 );

    data.m_nMaterial = 1;
	data.m_nHitBox = -m_iHealth;

	data.m_nColor = BloodColor();
	
	DispatchEffect( "HL1Gib", data );

	CSoundEnt::InsertSound( SOUND_MEAT, GetAbsOrigin(), 256, 0.5f, this );

	BaseClass::CorpseGib( info );

	return true;
}



//------------------------------------------------------------------------------
//
// Schedules
//
//------------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( monster_babushka, CNPC_HL1Babushka )

	DECLARE_SQUADSLOT( BABUSHKA_SQUAD_SLOT_ATTACK1 )
	DECLARE_SQUADSLOT( BABUSHKA_SQUAD_SLOT_ATTACK2 )
	DECLARE_SQUADSLOT( BABUSHKA_SQUAD_SLOT_CHASE )
	DECLARE_SQUADSLOT( BABUSHKA_SQUAD_SLOT_INVESTIGATE_SOUND )

	//=========================================================
	// > SCHED_BABUSHKA_FOLLOW
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_BABUSHKA_FOLLOW,
	
		"	Tasks"
//		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_BABUSHKA_STOP_FOLLOWING"
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
	// > SCHED_BABUSHKA_ENEMY_DRAW
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_BABUSHKA_ENEMY_DRAW,
	
		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_FACE_ENEMY				0"
		"		TASK_PLAY_SEQUENCE_FACE_ENEMY		ACTIVITY:ACT_ARM"
		"	"
		"	Interrupts"
	)
	
	//=========================================================
	// > SCHED_BABUSHKA_FACE_TARGET
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_BABUSHKA_FACE_TARGET,
	
		"	Tasks"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
		"		TASK_FACE_TARGET			0"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
		"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_BABUSHKA_FOLLOW"
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
	// > SCHED_BABUSHKA_IDLE_STAND
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_BABUSHKA_IDLE_STAND,
	
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


//=========================================================
// DEAD BABUSHKA PROP
//
// Designer selects a pose in worldcraft, 0 through num_poses-1
// this value is added to what is selected as the 'first dead pose'
// among the monster's normal animations. All dead poses must
// appear sequentially in the model file. Be sure and set
// the m_iFirstPose properly!
//
//=========================================================
class CNPC_DeadBabushka : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_DeadBabushka, CAI_BaseNPC );
public:

	void Spawn( void );
	Class_T	Classify ( void ) { return	CLASS_NONE; }

	bool KeyValue( const char *szKeyName, const char *szValue );
	float MaxYawSpeed ( void ) { return 8.0f; }

	int	m_iPose;// which sequence to display	-- temporary, don't need to save
	int m_iDesiredSequence;
	static char *m_szPoses[3];
	
	DECLARE_DATADESC();
};

char *CNPC_DeadBabushka::m_szPoses[] = { "lying_on_back", "lying_on_side", "lying_on_stomach" };

bool CNPC_DeadBabushka::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( FStrEq( szKeyName, "pose" ) )
		m_iPose = atoi( szValue );
	else 
		BaseClass::KeyValue( szKeyName, szValue );

	return true;
}

LINK_ENTITY_TO_CLASS( monster_babushka_dead, CNPC_DeadBabushka );

BEGIN_DATADESC( CNPC_DeadBabushka )
END_DATADESC()

//=========================================================
// ********** DeadBabushka SPAWN **********
//=========================================================
void CNPC_DeadBabushka::Spawn( void )
{
	PrecacheModel("models/babushkahl1.mdl");
	SetModel( "models/babushkahl1.mdl");

	ClearEffects();
	SetSequence( 0 );
	m_bloodColor		= BLOOD_COLOR_RED;

	SetRenderColor( 255, 255, 255, 255 );

	SetSequence( m_iDesiredSequence = LookupSequence( m_szPoses[m_iPose] ) );
	if ( GetSequence() == -1 )
	{
		Msg ( "Dead babushka with bad pose\n" );
	}
	// Corpses have less health
	m_iHealth			= 0.0;//gSkillData.babushkaHealth;

	NPCInitDead();
}
