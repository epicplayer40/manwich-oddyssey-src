//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Aelot Marksman
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//



#include	"cbase.h"
#include	"ai_default.h"
#include	"ai_task.h"
#include	"ai_schedule.h"
#include	"ai_node.h"
#include	"ai_hull.h"
#include	"ai_hint.h"
#include	"ai_memory.h"
#include	"ai_route.h"
#include	"ai_motor.h"
#include	"ai_squad.h"
#include	"ai_squadslot.h"
#include	"beam_shared.h"
#include	"hl1_npc_alien_trooper.h"
#include	"soundent.h"
#include	"game.h"
#include	"npcevent.h"
#include	"entitylist.h"
#include	"activitylist.h"
#include	"animation.h"
#include	"basecombatweapon.h"
#include	"IEffects.h"
#include	"vstdlib/random.h"
#include	"engine/IEngineSound.h"
#include	"ammodef.h"
#include	"ai_behavior_follow.h"
#include	"AI_Criteria.h"
#include	"SoundEmitterSystem/isoundemittersystembase.h"

#include 	<time.h>

#include "effect_dispatch_data.h"
#include "te_effect_dispatch.h"

#define MK_ATTACK	"MK_ATTACK" //spotted enemy
#define MK_MAD		"MK_MAD" //retaliating against player
#define MK_SHOT		"MK_SHOT" // what the hell, player?
#define MK_KILL		"MK_KILL" // killed an enemy
#define MK_POK		"MK_POK" // pre-disaster dialogue (unused in MCM2)

ConVar	sk_alien_trooperhl1_health( "sk_alien_trooperhl1_health","120");

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
// first flag is alien_trooper dying for scripted sequences?
#define		ALIEN_TROOPER_AE_DRAW		( 2 )
#define		ALIEN_TROOPER_AE_SHOOT		( 3 )
#define		ALIEN_TROOPER_AE_HOLSTER	( 4 )
#define		ALIEN_TROOPER_AE_STOPLASER	( 5 )

#define		ALIEN_TROOPER_BODY_GUNHOLSTERED	0
#define		ALIEN_TROOPER_BODY_GUNDRAWN		1
#define		ALIEN_TROOPER_BODY_GUNGONE			2


enum Alien_TrooperSquadSlot_T
{	
	ALIEN_TROOPER_SQUAD_SLOT_ATTACK1 = LAST_SHARED_SQUADSLOT,
	ALIEN_TROOPER_SQUAD_SLOT_ATTACK2,
	ALIEN_TROOPER_SQUAD_SLOT_INVESTIGATE_SOUND,
	ALIEN_TROOPER_SQUAD_SLOT_CHASE,
};


//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_HL1Alien_Trooper )
	DEFINE_FIELD( m_fGunDrawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flPainTime, FIELD_TIME ),
	DEFINE_ARRAY( m_pBeam, FIELD_CLASSPTR, ALIEN_TROOPER_MAX_BEAMS ),
	DEFINE_FIELD( m_iBeams, FIELD_INTEGER ),
	DEFINE_FIELD( m_flCheckAttackTime, FIELD_TIME ),
	DEFINE_FIELD( m_fLastAttackCheck, FIELD_BOOLEAN ),

	DEFINE_THINKFUNC( SUB_LVFadeOut ),

	//DEFINE_FIELD( m_iAmmoType, FIELD_INTEGER ),
END_DATADESC()


LINK_ENTITY_TO_CLASS( npc_alien_trooper, CNPC_HL1Alien_Trooper );


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
void CNPC_HL1Alien_Trooper::Spawn()
{
	Precache( );

	SetModel( "models/alien_trooper.mdl");

	SetRenderColor( 255, 255, 255, 255 );
	
	
	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	m_bloodColor		= BLOOD_COLOR_YELLOW;
	m_iHealth			= sk_alien_trooperhl1_health.GetFloat();
	SetViewOffset( Vector ( 0, 0, 100 ) );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_NPCState			= NPC_STATE_NONE;

	SetBodygroup( 1, 0 );

	m_fGunDrawn			= false;

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS | bits_CAP_USE | bits_CAP_DOORS_GROUP);
	CapabilitiesAdd( bits_CAP_INNATE_RANGE_ATTACK1 | bits_CAP_TURN_HEAD | bits_CAP_ANIMATEDFACE );
	CapabilitiesAdd( bits_CAP_SQUAD | bits_CAP_NO_HIT_SQUADMATES | bits_CAP_FRIENDLY_DMG_IMMUNE  );
	
	NPCInit();

}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CNPC_HL1Alien_Trooper::Precache()
{
	m_iAmmoType = GetAmmoDef()->Index("Pistol");
	PrecacheModel( STRING( GetModelName() ) );

	PrecacheModel( "models/alien_trooper.mdl" );

	PrecacheScriptSound( "Maluk.FireWeapon" );
	PrecacheScriptSound( "Alien_Trooper.Pain" );
	PrecacheScriptSound( "Alien_Trooper.Die" );

	// every new alien_trooper must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	BaseClass::Precache();
}	

void CNPC_HL1Alien_Trooper::ModifyOrAppendCriteria( AI_CriteriaSet& criteriaSet )
{
	BaseClass::ModifyOrAppendCriteria( criteriaSet );

	bool predisaster = FBitSet( m_spawnflags, SF_NPC_PREDISASTER ) ? true : false;

	criteriaSet.AppendCriteria( "disaster", predisaster ? "[disaster::pre]" : "[disaster::post]" );
}

// Init talk data
void CNPC_HL1Alien_Trooper::TalkInit()
{
	BaseClass::TalkInit();

	// get voice for head - just one alien_trooper voice for now
	GetExpresser()->SetVoicePitch( 100 );
}


//=========================================================
// GetSoundInterests - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CNPC_HL1Alien_Trooper::GetSoundInterests ( void) 
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
Class_T	CNPC_HL1Alien_Trooper::Classify ( void )
{
	return	CLASS_TEAM2;
}

//=========================================================
// ALertSound - alien_trooper says "Freeze!"
//=========================================================
void CNPC_HL1Alien_Trooper::AlertSound( void )
{
	if ( GetEnemy() != NULL )
	{
	//	if ( IsOkToSpeak() )
	//	{
			Speak( MK_ATTACK );
	//	}
	}

}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CNPC_HL1Alien_Trooper::SetYawSpeed ( void )
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
bool CNPC_HL1Alien_Trooper::CheckRangeAttack1 ( float flDot, float flDist )
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
int CNPC_HL1Alien_Trooper::RangeAttack1Conditions( float flDot, float flDist )
{
	if (GetEnemy() == NULL)
	{
		return( COND_NONE );
	}
	
	else if ( flDist > 3500 ) //1024 originally
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

void CNPC_HL1Alien_Trooper::ZapBeam( int side )
{
	Vector vecSrc, vecAim;
	trace_t tr;
	CBaseEntity *pEntity;

	if ( m_iBeams >= ALIEN_TROOPER_MAX_BEAMS )
		 return;

	Vector forward, right, up;
	AngleVectors( GetAbsAngles(), &forward, &right, &up );

	vecSrc = GetAbsOrigin() + up * 36;
	vecAim = GetShootEnemyDir( vecSrc );
	float deflection = 0.012; //is this for accuracy??? 0.01 by default +++++update: Yep, it is.
	vecAim = vecAim + side * right * random->RandomFloat( 0, deflection ) + up * random->RandomFloat( -deflection, deflection );
	UTIL_TraceLine ( vecSrc, vecSrc + vecAim * 3500, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr);
	
	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/laser.vmt", 5.0f );
	if ( m_pBeam[m_iBeams] == NULL )
		 return;

	m_pBeam[m_iBeams]->PointEntInit( tr.endpos, this );
	m_pBeam[m_iBeams]->SetEndAttachment( side < 0 ? 2 : 1 );
	m_pBeam[m_iBeams]->SetColor( 0, 137, 255 );
	m_pBeam[m_iBeams]->SetBrightness( 255 );
	m_pBeam[m_iBeams]->SetNoise( 0.1f );
	m_pBeam[m_iBeams]->AddSpawnFlags( SF_BEAM_TEMPORARY );
	m_iBeams++;

	CPASAttenuationFilter filter( this );


	int pitchShift = random->RandomInt( 0, 20 );
	
	// Only shift about half the time
	if ( pitchShift > 10 )
		pitchShift = 0;
	else
		pitchShift -= 5;

	EmitSound_t params;
	params.m_pSoundName = "Maluk.FireWeapon";
	params.m_flVolume = 1;
	params.m_nChannel= CHAN_WEAPON;
	params.m_SoundLevel = SNDLVL_NORM;
	params.m_nPitch = 100 + pitchShift;
	EmitSound( filter, entindex(), params );

	CSoundEnt::InsertSound ( SOUND_COMBAT, GetAbsOrigin(), 384, 0.3 );

	pEntity = tr.m_pEnt;

	if ( pEntity != NULL && m_takedamage )
	{
		CTakeDamageInfo info( this, this, 5, DMG_DISSOLVE );
		CalculateMeleeDamageForce( &info, vecAim, tr.endpos );
		pEntity->DispatchTraceAttack( info, vecAim, &tr );
	}
}



//=========================================================
// Alien_TrooperFirePistol - shoots one round from the pistol at
// the enemy alien_trooper is facing.
//=========================================================
void CNPC_HL1Alien_Trooper::Alien_TrooperFirePistol ( void )
{
	Vector vecShootOrigin;
	
	vecShootOrigin = GetAbsOrigin() + Vector( 0, 0, 55 );
	Vector vecShootDir = GetShootEnemyDir( vecShootOrigin );

	QAngle angDir;
	
	VectorAngles( vecShootDir, angDir );
//	SetBlending( 0, angDir.x );
	DoMuzzleFlash();

	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_15DEGREES, 3000, m_iAmmoType ); //raised gun range to 1200 in accordance with range increase

	//FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 1024, m_iAmmoType ); Automatic Mode - disabled for now
	
	int pitchShift = random->RandomInt( 0, 20 );
	
	// Only shift about half the time
	if ( pitchShift > 10 )
		pitchShift = 0;
	else
		pitchShift -= 5;

	CPASAttenuationFilter filter( this );

	EmitSound_t params;
	params.m_pSoundName = "Alien_Trooper.FireWeapon";
	params.m_flVolume = 1;
	params.m_nChannel= CHAN_WEAPON;
	params.m_SoundLevel = SNDLVL_NORM;
	params.m_nPitch = 100 + pitchShift;
	EmitSound( filter, entindex(), params );

	CSoundEnt::InsertSound ( SOUND_COMBAT, GetAbsOrigin(), 384, 0.3 );

	// UNDONE: Reload?
	m_cAmmoLoaded--;// take away a bullet!
}

int CNPC_HL1Alien_Trooper::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
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
				Speak( MK_MAD );
				CapabilitiesRemove( bits_CAP_NO_HIT_PLAYER );
				Remember( bits_MEMORY_PROVOKED );
				StopFollowing();
			}
			else
			{
				// Hey, be careful with that
				Speak( MK_SHOT );
				Remember( bits_MEMORY_SUSPICIOUS );
			}
		}
		else if ( !(GetEnemy()->IsPlayer()) && m_lifeState == LIFE_ALIVE )
		{
			Speak( MK_SHOT );
		}
	}

	return ret;
}

//=========================================================
// PainSound
//=========================================================
void CNPC_HL1Alien_Trooper::PainSound( const CTakeDamageInfo &info )
{
	if (gpGlobals->curtime < m_flPainTime)
		return;
	
	m_flPainTime = gpGlobals->curtime + random->RandomFloat( 0.5, 0.75 );

	CPASAttenuationFilter filter( this );

	CSoundParameters params;
	if ( GetParametersForSound( "Alien_Trooper.Pain", params, NULL ) )
	{
		params.pitch = GetExpresser()->GetVoicePitch();

		EmitSound_t ep( params );

		EmitSound( filter, entindex(), ep );
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CNPC_HL1Alien_Trooper::DeathSound( const CTakeDamageInfo &info )
{
	CPASAttenuationFilter filter( this );

	CSoundParameters params;
	if ( GetParametersForSound( "Alien_Trooper.Die", params, NULL ) )
	{
		params.pitch = GetExpresser()->GetVoicePitch();

		EmitSound_t ep( params );

		EmitSound( filter, entindex(), ep );
	}
}

void CNPC_HL1Alien_Trooper::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr )
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

void CNPC_HL1Alien_Trooper::Event_Killed( const CTakeDamageInfo &info )
{
	if ( m_nBody < ALIEN_TROOPER_BODY_GUNGONE )
	{
		// drop the gun!
		Vector vecGunPos;
		QAngle angGunAngles;
		// ?
		//CBaseEntity *pGun = NULL;

		SetBodygroup( 1, ALIEN_TROOPER_BODY_GUNGONE);

		GetAttachment( "0", vecGunPos, angGunAngles );
		
		angGunAngles.y += 180;
		//pGun = DropItem( "weapon_oicw", vecGunPos, angGunAngles );
	}

	ClearBeams( );

	SetUse( NULL );	
	BaseClass::Event_Killed( info  );

	if ( UTIL_IsLowViolence() )
	{
		SUB_StartLVFadeOut( 0.0f );
	}
}

void CNPC_HL1Alien_Trooper::SUB_StartLVFadeOut( float delay, bool notSolid )
{
	SetThink( &CNPC_HL1Alien_Trooper::SUB_LVFadeOut );
	SetNextThink( gpGlobals->curtime + delay );
	SetRenderColorA( 255 );
	m_nRenderMode = kRenderNormal;

	if ( notSolid )
	{
		AddSolidFlags( FSOLID_NOT_SOLID );
		SetLocalAngularVelocity( vec3_angle );
	}
}

void CNPC_HL1Alien_Trooper::SUB_LVFadeOut( void  )
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

void CNPC_HL1Alien_Trooper::StartTask( const Task_t *pTask )
{
		ClearBeams();
	BaseClass::StartTask( pTask );	
}

void CNPC_HL1Alien_Trooper::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
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


void CNPC_HL1Alien_Trooper::ClearBeams( )
{
	for (int i = 0; i < ALIEN_TROOPER_MAX_BEAMS; i++)
	{
		if (m_pBeam[i])
		{
			UTIL_Remove( m_pBeam[i] );
			m_pBeam[i] = NULL;
		}
	}

	m_iBeams = 0;
}


void CNPC_HL1Alien_Trooper::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
	case ALIEN_TROOPER_AE_SHOOT:
	//	Alien_TrooperFirePistol();
		ClearBeams( );
		ZapBeam( -1 );
		ZapBeam( 1 );

		break;

	case ALIEN_TROOPER_AE_DRAW:
		// alien_trooper's bodygroup switches here so he can pull gun from holster
		SetBodygroup( 1, ALIEN_TROOPER_BODY_GUNDRAWN);
		m_fGunDrawn = true;
		break;

	case ALIEN_TROOPER_AE_HOLSTER:
		// change bodygroup to replace gun in holster
		SetBodygroup( 1, ALIEN_TROOPER_BODY_GUNHOLSTERED);
		m_fGunDrawn = false;
		break;
		case ALIEN_TROOPER_AE_STOPLASER:
		{
			ClearBeams();
		}
		break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
	}
}


//=========================================================
// AI Schedules Specific to this monster
//=========================================================

int CNPC_HL1Alien_Trooper::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{
	case SCHED_ARM_WEAPON:
		if ( GetEnemy() != NULL )
		{
			// face enemy, then draw.
			return SCHED_ALIEN_TROOPER_ENEMY_DRAW;
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
				return SCHED_ALIEN_TROOPER_FACE_TARGET;
			else
				return baseType;
		}
		break;

	case SCHED_TARGET_CHASE:
	{
		return SCHED_ALIEN_TROOPER_FOLLOW;
		break;
	}

	case SCHED_IDLE_STAND:
		{
			int	baseType;

			// call base class default so that scientist will talk
			// when 'used' 
			baseType = BaseClass::TranslateSchedule( scheduleType );
			
			if ( baseType == SCHED_IDLE_STAND )
				return SCHED_ALIEN_TROOPER_IDLE_STAND;
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
int CNPC_HL1Alien_Trooper::SelectSchedule( void )
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
		Speak( MK_KILL );
		ClearBeams();
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
				ClearBeams();
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

NPC_STATE CNPC_HL1Alien_Trooper::SelectIdealState ( void )
{
	return BaseClass::SelectIdealState();
}

void CNPC_HL1Alien_Trooper::DeclineFollowing( void )
{
	if ( CanSpeakAfterMyself() )
	{
		Speak( MK_POK );
	}
}

bool CNPC_HL1Alien_Trooper::CanBecomeRagdoll( void )
{
	if ( UTIL_IsLowViolence() )
	{
		return false;
	}

	return BaseClass::CanBecomeRagdoll();
}

bool CNPC_HL1Alien_Trooper::ShouldGib( const CTakeDamageInfo &info )
{
//	if ( UTIL_IsLowViolence() )
//	{
//		return false;
//	}

	return BaseClass::ShouldGib( info );
}

bool CNPC_HL1Alien_Trooper::CorpseGib( const CTakeDamageInfo &info )
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

AI_BEGIN_CUSTOM_NPC( monster_alien_trooper, CNPC_HL1Alien_Trooper )

	DECLARE_SQUADSLOT( ALIEN_TROOPER_SQUAD_SLOT_ATTACK1 )
	DECLARE_SQUADSLOT( ALIEN_TROOPER_SQUAD_SLOT_ATTACK2 )
	DECLARE_SQUADSLOT( ALIEN_TROOPER_SQUAD_SLOT_CHASE )
	DECLARE_SQUADSLOT( ALIEN_TROOPER_SQUAD_SLOT_INVESTIGATE_SOUND )

	//=========================================================
	// > SCHED_ALIEN_TROOPER_FOLLOW
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ALIEN_TROOPER_FOLLOW,
	
		"	Tasks"
//		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_ALIEN_TROOPER_STOP_FOLLOWING"
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
	// > SCHED_ALIEN_TROOPER_ENEMY_DRAW
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ALIEN_TROOPER_ENEMY_DRAW,
	
		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_FACE_ENEMY				0"
		"		TASK_PLAY_SEQUENCE_FACE_ENEMY		ACTIVITY:ACT_ARM"
		"	"
		"	Interrupts"
	)
	
	//=========================================================
	// > SCHED_ALIEN_TROOPER_FACE_TARGET
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ALIEN_TROOPER_FACE_TARGET,
	
		"	Tasks"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
		"		TASK_FACE_TARGET			0"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
		"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_ALIEN_TROOPER_FOLLOW"
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
	// > SCHED_ALIEN_TROOPER_IDLE_STAND
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ALIEN_TROOPER_IDLE_STAND,
	
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
