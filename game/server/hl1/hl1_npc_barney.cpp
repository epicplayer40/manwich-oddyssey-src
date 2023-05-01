//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: The Conscripts
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
//1: Make a spawnflag where they start with their guns drawn (COMPLETE)
//2: Weapon Variety (Bolt Action Rifle, Shotgun, a few others) (COMPLETE)
//3: Make a spawnflag where they automatically follow the player
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
#include	"hl1_npc_barney.h"
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

#define BA_ATTACK	"BA_ATTACK"
#define BA_MAD		"BA_MAD"
#define BA_SHOT		"BA_SHOT"
#define BA_KILL		"BA_KILL"
#define BA_POK		"BA_POK"

#define SF_START_WITH_GUN_DRAWN	( 32 )
#define SF_AUTO_FOLLOW ( 21166 )

ConVar	sk_barneyhl1_health( "sk_barneyhl1_health","80");

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
// first flag is barney dying for scripted sequences?
#define		BARNEY_AE_RELOAD		( 1 )
#define		BARNEY_AE_DRAW		( 2 )
#define		BARNEY_AE_SHOOT		( 3 )
#define		BARNEY_AE_HOLSTER	( 4 )

#define		BARNEY_BODY_GUNHOLSTERED	0
#define		BARNEY_BODY_GUNDRAWN		1
#define		BARNEY_BODY_GUNGONE			2
#define		BARNEY_BODY_SHOTGUN		3
#define		BARNEY_BODY_GARAND		4
#define		BARNEY_BODY_SHOTGUNHOLSTERED 5
#define		BARNEY_BODY_GARANDHOLSTERED 6

#define BARNEY_AR2				( 1 << 0)
#define BARNEY_SHOTGUN			( 1 << 1)
#define BARNEY_GARAND			( 1 << 2)


enum BarneySquadSlot_T
{	
	BARNEY_SQUAD_SLOT_ATTACK1 = LAST_SHARED_SQUADSLOT,
	BARNEY_SQUAD_SLOT_ATTACK2,
	BARNEY_SQUAD_SLOT_INVESTIGATE_SOUND,
	BARNEY_SQUAD_SLOT_CHASE,
};

int ACT_RANGE_ATTACK1_OTHER;
int ACT_WALK_UNARMED;
int ACT_RUN_UNARMED;
//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_HL1Barney )
	DEFINE_FIELD( m_fGunDrawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flPainTime, FIELD_TIME ),
	DEFINE_FIELD( m_flCheckAttackTime, FIELD_TIME ),
	DEFINE_FIELD( m_fLastAttackCheck, FIELD_BOOLEAN ),
	DEFINE_FIELD (m_iClipSize, FIELD_INTEGER),
	DEFINE_THINKFUNC( SUB_LVFadeOut ),
	DEFINE_KEYFIELD(m_iWeapons, FIELD_INTEGER, "weapons"),

	//DEFINE_FIELD( m_iAmmoType, FIELD_INTEGER ),
END_DATADESC()


LINK_ENTITY_TO_CLASS( monster_barney, CNPC_HL1Barney );

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

bool CNPC_HL1Barney::IsJumpLegal(const Vector &startPos, const Vector &apex, const Vector &endPos) const
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


void CNPC_HL1Barney::SetAim(const Vector &aimDir)
{
	QAngle angDir;
	VectorAngles(aimDir, angDir);

	float curPitch = GetPoseParameter("XR");
	float newPitch = curPitch + UTIL_AngleDiff(UTIL_ApproachAngle(angDir.x, curPitch, 60), curPitch);

	SetPoseParameter("XR", -newPitch); //-newpitch by default
}

//=========================================================
// Spawn
//=========================================================
void CNPC_HL1Barney::Spawn()
{
	Precache( );

	SetModelScale(random->RandomFloat(0.96, 1.04) );

	SetRenderColor( 255, 255, 255, 255 );


	SetModel( "models/bahl1.mdl" );   
	
	AddSpawnFlags(SF_NPC_LONG_RANGE);

	
	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	m_bloodColor		= BLOOD_COLOR_RED;
	m_iHealth			= sk_barneyhl1_health.GetFloat();
	SetViewOffset( Vector ( 0, 0, 100 ) );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_NPCState			= NPC_STATE_NONE;

	m_fGunDrawn			= false;
	m_iClipSize = 30;
	m_cAmmoLoaded = m_iClipSize;

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS | bits_CAP_USE | bits_CAP_DOORS_GROUP);
	CapabilitiesAdd( bits_CAP_INNATE_RANGE_ATTACK1 | bits_CAP_TURN_HEAD | bits_CAP_ANIMATEDFACE );
	CapabilitiesAdd( bits_CAP_SQUAD | bits_CAP_NO_HIT_SQUADMATES | bits_CAP_FRIENDLY_DMG_IMMUNE | bits_CAP_MOVE_JUMP | bits_CAP_MOVE_CLIMB );

	if (m_iWeapons == 0)
	{

/*	int min = 1;
	int max = 3;
	double scaled = (double)rand()/RAND_MAX;
	int r = (max - min +1)*scaled + min;
	
	if (r == 1)
	{*/
		m_iWeapons = BARNEY_AR2;
	/*}
	else if (r == 2)
	{
		m_iWeapons = BARNEY_SHOTGUN;
	}
	else if (r == 3)
	{
		m_iWeapons = BARNEY_GARAND;
	}*/

	}

	if (FBitSet(m_iWeapons, BARNEY_AR2))
	{
		SetBodygroup( 1, BARNEY_BODY_GUNHOLSTERED	);
			m_iClipSize = 30;
	}
	if (FBitSet(m_iWeapons, BARNEY_SHOTGUN))
	{
		SetBodygroup( 1, BARNEY_BODY_SHOTGUNHOLSTERED	);
			m_iClipSize = 6;
	}
	if (FBitSet(m_iWeapons, BARNEY_GARAND))
	{
		SetBodygroup( 1, BARNEY_BODY_GARANDHOLSTERED	);
			m_iClipSize = 8;
	}

		if (GetSpawnFlags() & SF_START_WITH_GUN_DRAWN)
		{
			if (FBitSet(m_iWeapons, BARNEY_AR2))
			{
				SetBodygroup( 1, BARNEY_BODY_GUNDRAWN	);
			}
			if (FBitSet(m_iWeapons, BARNEY_SHOTGUN))
			{
				SetBodygroup( 1, BARNEY_BODY_SHOTGUN	);
			}
			if (FBitSet(m_iWeapons, BARNEY_GARAND))
			{
				SetBodygroup( 1, BARNEY_BODY_GARAND	);
			}
				m_fGunDrawn = true;
		}

	
	NPCInit();
	
	SetUse( &CNPC_HL1Barney::FollowerUse );
}

void CNPC_HL1Barney::Activate()
{
	if (FBitSet(m_iWeapons, BARNEY_GARAND))
	{
		m_iClipSize = 8;
	}
	if (FBitSet(m_iWeapons, BARNEY_AR2))
	{
		m_iClipSize = 30;
	}
	if (FBitSet(m_iWeapons, BARNEY_SHOTGUN))
	{
		m_iClipSize = 6;
	}

	BaseClass::Activate();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CNPC_HL1Barney::Precache()
{

	if (FBitSet(m_iWeapons, BARNEY_GARAND))
	{
		m_iClipSize = 8;
	}
	if (FBitSet(m_iWeapons, BARNEY_AR2))
	{
		m_iClipSize = 30;
	}
	if (FBitSet(m_iWeapons, BARNEY_SHOTGUN))
	{
		m_iClipSize = 6;
	}
	m_iAmmoType = GetAmmoDef()->Index("Pistol");
	m_iClipSize = 30;
	m_cAmmoLoaded = m_iClipSize;
	PrecacheModel( STRING( GetModelName() ) );

	PrecacheModel( "models/bahl101.mdl" ); //red guy
	PrecacheModel( "models/bahl102.mdl" ); //gasmask citizen guy
	PrecacheModel( "models/bahl103.mdl" ); //brown guy
	PrecacheModel( "models/bahl104.mdl" ); //blue guy
	PrecacheModel( "models/bahl105.mdl" ); //green guy
	//Conscript animation set versions - Might not make it into final release
	PrecacheModel( "models/bahl106.mdl" );
	PrecacheModel( "models/bahl107.mdl" );
	PrecacheModel( "models/bahl108.mdl" );
	PrecacheModel( "models/bahl109.mdl" );
	PrecacheModel( "models/bahl110.mdl" );
		PrecacheModel( "models/bahl1.mdl" );

	PrecacheScriptSound( "Weapon_Pistol.NPC_Single" );
	PrecacheScriptSound( "Barney.FireShotgun" );
	PrecacheScriptSound( "Barney.FireGarand" );
	PrecacheScriptSound( "Barney.Pain" );
	PrecacheScriptSound( "Barney.Die" );

	// every new barney must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	BaseClass::Precache();
}	

void CNPC_HL1Barney::ModifyOrAppendCriteria( AI_CriteriaSet& criteriaSet )
{
	BaseClass::ModifyOrAppendCriteria( criteriaSet );

	bool predisaster = FBitSet( m_spawnflags, SF_NPC_PREDISASTER ) ? true : false;

	criteriaSet.AppendCriteria( "disaster", predisaster ? "[disaster::pre]" : "[disaster::post]" );

}

// Init talk data
void CNPC_HL1Barney::TalkInit()
{
	BaseClass::TalkInit();

	// get voice for head - just one barney voice for now
	GetExpresser()->SetVoicePitch( 100 );
}


//=========================================================
// GetSoundInterests - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CNPC_HL1Barney::GetSoundInterests ( void) 
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
Class_T	CNPC_HL1Barney::Classify ( void )
{
	return	CLASS_CONSCRIPT;
}

//=========================================================
// ALertSound - barney says "Freeze!"
//=========================================================
void CNPC_HL1Barney::AlertSound( void )
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
void CNPC_HL1Barney::SetYawSpeed ( void )
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
bool CNPC_HL1Barney::CheckRangeAttack1 ( float flDot, float flDist )
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
int CNPC_HL1Barney::RangeAttack1Conditions( float flDot, float flDist )
{
	if (GetEnemy() == NULL)
	{
		return( COND_NONE );
	}
	
	if (FBitSet(m_iWeapons, BARNEY_AR2))
	{
		if ( flDist > 1200 ) //1024 originally
		{
			return( COND_TOO_FAR_TO_ATTACK );
		}
	}

	if (FBitSet(m_iWeapons, BARNEY_GARAND))
	{
		if ( flDist > 2000 )
		{
			return( COND_TOO_FAR_TO_ATTACK );
		}
	}


	if (FBitSet(m_iWeapons, BARNEY_SHOTGUN))
	{
		if ( flDist > 1024 )
		{
			return( COND_TOO_FAR_TO_ATTACK );
		}
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
// BarneyFirePistol - shoots one round from the pistol at
// the enemy barney is facing.
//=========================================================
void CNPC_HL1Barney::BarneyFirePistol ( void )
{
	Vector vecShootOrigin;
	
	vecShootOrigin = GetAbsOrigin() + Vector( 0, 0, 55 );
	Vector vecShootDir = GetShootEnemyDir( vecShootOrigin );

	QAngle angDir;
	

	VectorAngles( vecShootDir, angDir );
//	SetBlending( 0, angDir.x );
	DoMuzzleFlash();

	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_2DEGREES, 1200, m_iAmmoType ); //raised gun range to 1200 in accordance with range increase
	//2degress by default - pinpoint accuracy
	//FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 1024, m_iAmmoType ); Automatic Mode - disabled for now
	
	int pitchShift = random->RandomInt( 0, 20 );
	
	// Only shift about half the time
	if ( pitchShift > 10 )
		pitchShift = 0;
	else
		pitchShift -= 5;

	CPASAttenuationFilter filter( this );

	EmitSound_t params;
	params.m_pSoundName = "Weapon_Pistol.NPC_Single";
	params.m_flVolume = 1;
	params.m_nChannel= CHAN_WEAPON;
	params.m_SoundLevel = SNDLVL_NORM;
	params.m_nPitch = 100 + pitchShift;
	EmitSound( filter, entindex(), params );

	CSoundEnt::InsertSound ( SOUND_COMBAT, GetAbsOrigin(), 384, 0.3 );

	Vector	muzzlePos;
	QAngle	muzzleAngle;


	GetAttachment( "0", muzzlePos, muzzleAngle );

	Vector	muzzleDir;
	
	if ( GetEnemy() == NULL )
	{
		AngleVectors( muzzleAngle, &muzzleDir );
	}
	else
	{
		muzzleDir = GetEnemy()->BodyTarget( muzzlePos ) - muzzlePos;
		VectorNormalize( muzzleDir );
	}
	UTIL_MuzzleFlash( muzzlePos, muzzleAngle, 1.0f, 1 );


	// UNDONE: Reload? +++++REDONE!!!
	m_cAmmoLoaded--;// take away a bullet!

	SetAim(vecShootDir);
}

void CNPC_HL1Barney::BarneyFireShotgun ( void )
{
	Vector vecShootOrigin;
	
	vecShootOrigin = GetAbsOrigin() + Vector( 0, 0, 55 );
	Vector vecShootDir = GetShootEnemyDir( vecShootOrigin );

	QAngle angDir;
	
	VectorAngles( vecShootDir, angDir );
//	SetBlending( 0, angDir.x );
	DoMuzzleFlash();

	FireBullets(7, vecShootOrigin, vecShootDir, VECTOR_CONE_15DEGREES, 800, m_iAmmoType ); //raised gun range to 1200 in accordance with range increase

	//FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 1024, m_iAmmoType ); Automatic Mode - disabled for now
	
	int pitchShift = random->RandomInt( 0, 20 );
	
	// Only shift about half the time
	if ( pitchShift > 10 )
		pitchShift = 0;
	else
		pitchShift -= 5;

	CPASAttenuationFilter filter( this );

	EmitSound_t params;
	params.m_pSoundName = "Barney.FireShotgun";
	params.m_flVolume = 1;
	params.m_nChannel= CHAN_WEAPON;
	params.m_SoundLevel = SNDLVL_NORM;
	params.m_nPitch = 100 + pitchShift;
	EmitSound( filter, entindex(), params );

	CSoundEnt::InsertSound ( SOUND_COMBAT, GetAbsOrigin(), 384, 0.3 );

	Vector	muzzlePos;
	QAngle	muzzleAngle;


	GetAttachment( "1", muzzlePos, muzzleAngle );

	Vector	muzzleDir;
	
	if ( GetEnemy() == NULL )
	{
		AngleVectors( muzzleAngle, &muzzleDir );
	}
	else
	{
		muzzleDir = GetEnemy()->BodyTarget( muzzlePos ) - muzzlePos;
		VectorNormalize( muzzleDir );
	}
	UTIL_MuzzleFlash( muzzlePos, muzzleAngle, 1.1f, 1 );

	// UNDONE: Reload?
	m_cAmmoLoaded--;// take away a bullet!

	SetAim(vecShootDir);
}

void CNPC_HL1Barney::BarneyFireGarand ( void )
{
	Vector vecShootOrigin;
	
	vecShootOrigin = GetAbsOrigin() + Vector( 0, 0, 55 );
	Vector vecShootDir = GetShootEnemyDir( vecShootOrigin );

	QAngle angDir;
	
	VectorAngles( vecShootDir, angDir );
//	SetBlending( 0, angDir.x );
	DoMuzzleFlash();

	FireBullets(5, vecShootOrigin, vecShootDir, VECTOR_CONE_1DEGREES, 2000, m_iAmmoType ); //raised gun range to 1200 in accordance with range increase

	//FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 1024, m_iAmmoType ); Automatic Mode - disabled for now
	
	int pitchShift = random->RandomInt( 0, 20 );
	
	// Only shift about half the time
	if ( pitchShift > 10 )
		pitchShift = 0;
	else
		pitchShift -= 5;

	CPASAttenuationFilter filter( this );

	EmitSound_t params;
	params.m_pSoundName = "Barney.FireGarand";
	params.m_flVolume = 1;
	params.m_nChannel= CHAN_WEAPON;
	params.m_SoundLevel = SNDLVL_NORM;
	params.m_nPitch = 100 + pitchShift;
	EmitSound( filter, entindex(), params );

	CSoundEnt::InsertSound ( SOUND_COMBAT, GetAbsOrigin(), 384, 0.3 );

		Vector	muzzlePos;
	QAngle	muzzleAngle;


	GetAttachment( "2", muzzlePos, muzzleAngle );

	Vector	muzzleDir;
	
	if ( GetEnemy() == NULL )
	{
		AngleVectors( muzzleAngle, &muzzleDir );
	}
	else
	{
		muzzleDir = GetEnemy()->BodyTarget( muzzlePos ) - muzzlePos;
		VectorNormalize( muzzleDir );
	}
	UTIL_MuzzleFlash( muzzlePos, muzzleAngle, 0.95f, 1 );

	// UNDONE: Reload?
	m_cAmmoLoaded--;// take away a bullet!

	SetAim(vecShootDir);
}

void CNPC_HL1Barney::DoMuzzleFlash( void )
{
	BaseClass::DoMuzzleFlash();
	
	CEffectData data;

	data.m_nAttachmentIndex = LookupAttachment( "0" );
	data.m_nEntIndex = entindex();
	DispatchEffect( "MuzzleFlash", data );
}


int CNPC_HL1Barney::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
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
void CNPC_HL1Barney::PainSound( const CTakeDamageInfo &info )
{
	if (gpGlobals->curtime < m_flPainTime)
		return;
	
	m_flPainTime = gpGlobals->curtime + random->RandomFloat( 0.5, 0.75 );

	CPASAttenuationFilter filter( this );

	CSoundParameters params;
	if ( GetParametersForSound( "Barney.Pain", params, NULL ) )
	{
		params.pitch = GetExpresser()->GetVoicePitch();

		EmitSound_t ep( params );

		EmitSound( filter, entindex(), ep );
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CNPC_HL1Barney::DeathSound( const CTakeDamageInfo &info )
{
	CPASAttenuationFilter filter( this );

	CSoundParameters params;
	if ( GetParametersForSound( "Barney.Die", params, NULL ) )
	{
		params.pitch = GetExpresser()->GetVoicePitch();

		EmitSound_t ep( params );

		EmitSound( filter, entindex(), ep );
	}
}

void CNPC_HL1Barney::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr )
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





Activity CNPC_HL1Barney::NPC_TranslateActivity(Activity eNewActivity)
{
	switch (eNewActivity)
	{
	case ACT_RANGE_ATTACK1:
		if (FBitSet(m_iWeapons, BARNEY_AR2))
		{
			{
				return (Activity)ACT_RANGE_ATTACK1;
			}
		}
		else
		{
			{
				return (Activity)ACT_RANGE_ATTACK1_OTHER;
			}
		}
		break;
	case ACT_WALK:
		if ( eNewActivity == ACT_WALK )
		{
			if ( m_fGunDrawn == false )
			{
				return (Activity)ACT_WALK_UNARMED;
			}
		}

		if (GetSpawnFlags() & SF_START_WITH_GUN_DRAWN)
		{
				eNewActivity = ACT_WALK;
		}
		break;
	case ACT_RUN:
		if ( eNewActivity == ACT_RUN )
		{
			if ( m_fGunDrawn == false )
			{
				return (Activity)ACT_RUN_UNARMED;
			}
		}

		if (GetSpawnFlags() & SF_START_WITH_GUN_DRAWN)
		{
				eNewActivity = ACT_RUN;
		}
		break;

	//	case ACT_IDLE:

	}

	return BaseClass::NPC_TranslateActivity(eNewActivity);
}




void CNPC_HL1Barney::Event_Killed( const CTakeDamageInfo &info )
{
	if ( m_nBody != BARNEY_BODY_GUNGONE )
	{
		// drop the gun!
		Vector vecGunPos;
		QAngle angGunAngles;
		CBaseEntity *pGun = NULL;

		SetBodygroup( 1, BARNEY_BODY_GUNGONE);

		GetAttachment( "0", vecGunPos, angGunAngles );
		
		angGunAngles.y += 180;
		if (FBitSet(m_iWeapons, BARNEY_AR2))
		{
			pGun = DropItem( "weapon_pistol", vecGunPos, angGunAngles );
		}

		if (FBitSet(m_iWeapons, BARNEY_SHOTGUN))
		{
			pGun = DropItem( "weapon_shotgun", vecGunPos, angGunAngles );
		}

		if (FBitSet(m_iWeapons, BARNEY_GARAND)) //placeholder til garand is properly implemented
		{
			pGun = DropItem( "weapon_garand", vecGunPos, angGunAngles );
		}
	}

	SetUse( NULL );	
	BaseClass::Event_Killed( info  );

	if ( UTIL_IsLowViolence() )
	{
		SUB_StartLVFadeOut( 0.0f );
	}
}

void CNPC_HL1Barney::SUB_StartLVFadeOut( float delay, bool notSolid )
{
	SetThink( &CNPC_HL1Barney::SUB_LVFadeOut );
	SetNextThink( gpGlobals->curtime + delay );
	SetRenderColorA( 255 );
	m_nRenderMode = kRenderNormal;

	if ( notSolid )
	{
		AddSolidFlags( FSOLID_NOT_SOLID );
		SetLocalAngularVelocity( vec3_angle );
	}
}

void CNPC_HL1Barney::SUB_LVFadeOut( void  )
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

void CNPC_HL1Barney::StartTask( const Task_t *pTask )
{
	switch (pTask->iTask)
	{
		case TASK_RELOAD:
		SetIdealActivity(ACT_RELOAD);
		break;
	default:
	BaseClass::StartTask( pTask );
	break;
	}
}

void CNPC_HL1Barney::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:

		if (FBitSet(m_iWeapons, BARNEY_AR2))
		{ //pistol has normal firerate
	//		m_flPlaybackRate = 1.0;
		}

		if (FBitSet(m_iWeapons, BARNEY_SHOTGUN))
		{ //shotgun has slightly lower firerate
			m_flPlaybackRate = 0.5;
		}

		if (FBitSet(m_iWeapons, BARNEY_GARAND))
		{ //M1 Garand has very low firerate
			m_flPlaybackRate = 0.3;
		}


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

void CNPC_HL1Barney::CheckAmmo(void)
{
	if (m_cAmmoLoaded <= 0)
		SetCondition(COND_NO_PRIMARY_AMMO);

}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CNPC_HL1Barney::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
	case BARNEY_AE_SHOOT:
	//	BarneyFirePistol();

		if (FBitSet(m_iWeapons, BARNEY_AR2))
		{
			BarneyFirePistol();
		}

		if (FBitSet(m_iWeapons, BARNEY_SHOTGUN))
		{
			BarneyFireShotgun();
		}
		if (FBitSet(m_iWeapons, BARNEY_GARAND))
		{
			BarneyFireGarand();
		}
		break;

	case BARNEY_AE_RELOAD:
	{
		CPASAttenuationFilter filter(this);
		EmitSound(filter, entindex(), "HGrunt.Reload");

		m_cAmmoLoaded = m_iClipSize;
		ClearCondition(COND_NO_PRIMARY_AMMO);
	}
	break;

	case BARNEY_AE_DRAW:
		// barney's bodygroup switches here so he can pull gun from holster
	//	SetBodygroup( 1, BARNEY_BODY_GUNDRAWN);

		if (FBitSet(m_iWeapons, BARNEY_AR2))
		{
			SetBodygroup( 1, BARNEY_BODY_GUNDRAWN);
		}

		if (FBitSet(m_iWeapons, BARNEY_SHOTGUN))
		{
			SetBodygroup( 1, BARNEY_BODY_SHOTGUN);
		}
		if (FBitSet(m_iWeapons, BARNEY_GARAND))
		{
			SetBodygroup( 1, BARNEY_BODY_GARAND);
		}

		m_fGunDrawn = true;
		break;

	case BARNEY_AE_HOLSTER:
		// change bodygroup to replace gun in holster
		SetBodygroup( 1, BARNEY_BODY_GUNHOLSTERED);
		m_fGunDrawn = false;
		break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
	}
}


//=========================================================
// AI Schedules Specific to this monster
//=========================================================

int CNPC_HL1Barney::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{
	case SCHED_ARM_WEAPON:
		if ( GetEnemy() != NULL )
		{
			// face enemy, then draw.
			return SCHED_BARNEY_ENEMY_DRAW;
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
				return SCHED_BARNEY_FACE_TARGET;
			else
				return baseType;
		}
		break;

	case SCHED_TARGET_CHASE:
	{
		return SCHED_BARNEY_FOLLOW;
		break;
	}

	case SCHED_IDLE_STAND:
		{
			int	baseType;

			// call base class default so that scientist will talk
			// when 'used' 
			baseType = BaseClass::TranslateSchedule( scheduleType );
			
			if ( baseType == SCHED_IDLE_STAND )
				return SCHED_BARNEY_IDLE_STAND;
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
int CNPC_HL1Barney::SelectSchedule( void )
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
		//		 return SCHED_TAKE_COVER_FROM_ENEMY;
					return SCHED_BARNEY_COVER;	

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

			else if (HasCondition(COND_NO_PRIMARY_AMMO))
			{
				//!!!KELLY - this individual just realized he's out of bullet ammo. 
				// He's going to try to find cover to run to and reload, but rarely, if 
				// none is available, he'll drop and reload in the open here. 
				return SCHED_BARNEY_HIDE_RELOAD;
			}


		}
		break;

	case NPC_STATE_IDLE:

		if ( HasCondition( COND_LIGHT_DAMAGE ) || HasCondition( COND_HEAVY_DAMAGE ) ) 
		{
			// flinch if hurt
			return SCHED_SMALL_FLINCH;
		}

		if (GetSpawnFlags() & SF_AUTO_FOLLOW)
		{
			return SCHED_BARNEY_FOLLOW;
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

NPC_STATE CNPC_HL1Barney::SelectIdealState ( void )
{
	return BaseClass::SelectIdealState();
}

void CNPC_HL1Barney::DeclineFollowing( void )
{
	if ( CanSpeakAfterMyself() )
	{
		Speak( BA_POK );
	}
}

bool CNPC_HL1Barney::CanBecomeRagdoll( void )
{
	if ( UTIL_IsLowViolence() )
	{
		return false;
	}

	return BaseClass::CanBecomeRagdoll();
}

bool CNPC_HL1Barney::ShouldGib( const CTakeDamageInfo &info )
{
//	if ( UTIL_IsLowViolence() )
//	{
//		return false;
//	}

	return BaseClass::ShouldGib( info );
}

bool CNPC_HL1Barney::CorpseGib( const CTakeDamageInfo &info )
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

AI_BEGIN_CUSTOM_NPC( monster_barney, CNPC_HL1Barney )

	DECLARE_SQUADSLOT( BARNEY_SQUAD_SLOT_ATTACK1 )
	DECLARE_SQUADSLOT( BARNEY_SQUAD_SLOT_ATTACK2 )
	DECLARE_SQUADSLOT( BARNEY_SQUAD_SLOT_CHASE )
	DECLARE_SQUADSLOT( BARNEY_SQUAD_SLOT_INVESTIGATE_SOUND )

	DECLARE_ACTIVITY(ACT_RANGE_ATTACK1_OTHER);
	DECLARE_ACTIVITY(ACT_WALK_UNARMED);
	DECLARE_ACTIVITY(ACT_RUN_UNARMED);

	//=========================================================
	// > SCHED_BARNEY_FOLLOW
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_BARNEY_FOLLOW,
	
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
		SCHED_BARNEY_ENEMY_DRAW,
	
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
		SCHED_BARNEY_FACE_TARGET,
	
		"	Tasks"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
		"		TASK_FACE_TARGET			0"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
		"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_BARNEY_FOLLOW"
		"	"
		"	Interrupts"
		"		COND_GIVE_WAY"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_PROVOKED"
		"		COND_HEAR_DANGER"
	)

	DEFINE_SCHEDULE
	(
	SCHED_BARNEY_HIDE_RELOAD,

	"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_BARNEY_RELOAD"
		"		TASK_FIND_COVER_FROM_ENEMY		0"
		"		TASK_RUN_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_REMEMBER					MEMORY:INCOVER"
		"		TASK_FACE_ENEMY					0"
		"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_RELOAD"
		"	"
		"	Interrupts"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
	)
	
	DEFINE_SCHEDULE
	(
	SCHED_BARNEY_RELOAD,

	"	Tasks"
	"		TASK_STOP_MOVING			0"
	"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_RELOAD"
	"	"
	"	Interrupts"
	"		COND_HEAVY_DAMAGE"
	)

	DEFINE_SCHEDULE
	(
		SCHED_BARNEY_COVER,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_SCI_PANIC"
		"		TASK_STOP_MOVING				0"
		"		TASK_FIND_COVER_FROM_ENEMY		0"
		"		TASK_RUN_PATH_SCARED			0"
		"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_BARNEY_HIDE"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
	)

	//=========================================================
	// > SCHED_SCI_HIDE
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_BARNEY_HIDE,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_COVER"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_COVER"
		"		TASK_WAIT_RANDOM			10"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_SEE_ENEMY"
		"		COND_SEE_HATE"
		"		COND_SEE_FEAR"
		"		COND_SEE_DISLIKE"
		"		COND_HEAR_DANGER"
	)


	//=========================================================
	// > SCHED_BARNEY_IDLE_STAND
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_BARNEY_IDLE_STAND,
	
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
