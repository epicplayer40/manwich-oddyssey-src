//========= Copyright © LOLOLOL, All rights reserved. ============//
//
// Purpose: Bigger, badder Zappas with miniguns instead of the usual laser rifle.
//
//
//=============================================================================//

//TODO: 
//Taunt the player if he's in cover for too long
//A system where monster_alien_slaves (regular zappas) follow him around when they see him
//Make the model changeable thru hammer for when the Zappa Leader skin is made
//Fix the weird bug where the melee attack makes a flash appear between his feet


#include "cbase.h"
#include "AI_Task.h"
#include "AI_Default.h"
#include "AI_Schedule.h"
#include "AI_Hull.h"
#include "AI_Motor.h"
#include "AI_Memory.h"
#include "npc_combine.h"
#include "physics.h"
#include "bitstring.h"
#include "activitylist.h"
#include "game.h"
#include "NPCEvent.h"
#include "Player.h"
#include "EntityList.h"
#include "AI_Interactions.h"
#include "soundent.h"
#include "Gib.h"
#include "shake.h"
#include "ammodef.h"
#include "Sprite.h"
#include "explode.h"
#include "grenade_homer.h"
#include "AI_BaseNPC.h"
#include "soundenvelope.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "te_effect_dispatch.h"
#include "te_particlesystem.h"

#define SYNTHELITE_GIB_COUNT			5 

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar sk_islaveboss_health( "sk_islaveboss_health", "500" );

class CSprite;

extern void CreateConcussiveBlast( const Vector &origin, const Vector &surfaceNormal, CBaseEntity *pOwner, float magnitude );

#define	SYNTHELITE_MODEL	"models/islave_boss.mdl"

#define SYNTHELITE_MSG_SHOT	1
#define SYNTHELITE_MSG_SHOT_START 2

enum 
{
	SYNTHELITE_REF_INVALID = 0,
	SYNTHELITE_REF_MUZZLE,
	SYNTHELITE_REF_LEFT_SHOULDER,
	SYNTHELITE_REF_HUMAN_HEAD,
	SYNTHELITE_REF_RIGHT_ARM_HIGH,
	SYNTHELITE_REF_RIGHT_ARM_LOW,
	SYNTHELITE_REF_LEFT_ARM_HIGH,
	SYNTHELITE_REF_LEFT_ARM_LOW,
	SYNTHELITE_REF_TORSO,
	SYNTHELITE_REF_LOWER_TORSO,
	SYNTHELITE_REF_RIGHT_THIGH_HIGH,
	SYNTHELITE_REF_RIGHT_THIGH_LOW,
	SYNTHELITE_REF_LEFT_THIGH_HIGH,
	SYNTHELITE_REF_LEFT_THIGH_LOW,
//	SYNTHELITE_SHOVE,
	SYNTHELITE_REF_RIGHT_SHIN_HIGH,
	SYNTHELITE_REF_RIGHT_SHIN_LOW,
	SYNTHELITE_REF_LEFT_SHIN_HIGH,
	SYNTHELITE_REF_LEFT_SHIN_LOW,
	SYNTHELITE_REF_RIGHT_SHOULDER,
	
	NUM_SYNTHELITE_ATTACHMENTS,
};




class CNPC_SynthElite : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_SynthElite, CAI_BaseNPC );
public:


	CNPC_SynthElite( void );

	int	OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void Event_Killed( const CTakeDamageInfo &info );
	int	TranslateSchedule( int type );
	int	MeleeAttack1Conditions( float flDot, float flDist );
	int	RangeAttack1Conditions( float flDot, float flDist );
	void Gib(void);
	void ShootMG( void );

	void Precache( void );
	void Spawn( void );
	void PrescheduleThink( void );
	void TraceAttack( CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, trace_t *ptr, int bitsDamageType );
	void HandleAnimEvent( animevent_t *pEvent );
	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );

	void			DoMuzzleFlash( void );


	void PainSound( const CTakeDamageInfo &info );
	void DeathSound( const CTakeDamageInfo &info );
	void AlertSound( void );



	float MaxYawSpeed( void );

	//Class_T Classify( void ) { return CLASS_ANTLION; } no need for this
	Class_T	Classify( void );

	Activity NPC_TranslateActivity( Activity baseAct );

	virtual int	SelectSchedule( void );

	void BuildScheduleTestBits( void );



	bool m_fOffBalance;
	float m_flNextClobberTime;

	int	GetReferencePointForBodyGroup( int bodyGroup );

	void Shove( void );
	void FireRangeWeapon( void );
	void	MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType );

	CSprite *m_pGlowSprite[NUM_SYNTHELITE_ATTACHMENTS];


	int	m_nGibModel;

	int		m_iAmmoType;

	float m_flGlowTime;
	float m_flLastRangeTime;

	float m_aimYaw;
	float m_aimPitch;

	int	m_YawControl;
	int	m_PitchControl;
	int	m_MuzzleAttachment;


	DECLARE_DATADESC();

private:


	DEFINE_CUSTOM_AI;
};

void CNPC_SynthElite::PainSound( const CTakeDamageInfo &info )
{

	EmitSound( "VortBoss.Pain" );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_SynthElite::DeathSound( const CTakeDamageInfo &info ) 
{
	EmitSound( "VortBoss.Die" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_SynthElite::AlertSound( void )
{
	EmitSound( "VortBoss.Alert" );

}


void CNPC_SynthElite::ShootMG( void )
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
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_15DEGREES, 1024, m_iAmmoType  ); //originally 2 bullets at a time w/ 10 degree spread
//	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "hass/minigun_shoot.wav", 1, ATTN_NORM, 0, 100 );
}

void CNPC_SynthElite::MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType )
{
	float flTracerDist;
	Vector vecDir;
	Vector vecEndPos;

	vecDir = tr.endpos - vecTracerSrc;

	flTracerDist = VectorNormalize( vecDir );

	CEffectData data;

	data.m_nAttachmentIndex = LookupAttachment( "Muzzle" );
	data.m_nEntIndex = entindex();

	UTIL_Tracer( vecTracerSrc, tr.endpos, 0, TRACER_DONT_USE_ATTACHMENT, 5000, true, "GunshipTracer" );
}

#define	SYNTHELITE_DEFAULT_ARMOR_HEALTH	50

#define	SYNTHELITE_MELEE1_RANGE	92
#define	SYNTHELITE_MELEE1_CONE	0.5f

#define	SYNTHELITE_RANGE1_RANGE	1024
#define	SYNTHELITE_RANGE1_CONE	0.0f

#define	SYNTHELITE_GLOW_TIME			0.5f

BEGIN_DATADESC( CNPC_SynthElite )

	DEFINE_FIELD( m_fOffBalance, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flNextClobberTime, FIELD_TIME ),
	DEFINE_ARRAY( m_pGlowSprite, FIELD_CLASSPTR, NUM_SYNTHELITE_ATTACHMENTS ),
	DEFINE_FIELD( m_nGibModel, FIELD_INTEGER ),
	DEFINE_FIELD( m_flGlowTime,	FIELD_TIME ),
	DEFINE_FIELD( m_flLastRangeTime, FIELD_TIME ),
	DEFINE_FIELD( m_aimYaw,	FIELD_FLOAT ),
	DEFINE_FIELD( m_aimPitch, FIELD_FLOAT ),
	DEFINE_FIELD( m_YawControl,	FIELD_INTEGER ),
	DEFINE_FIELD( m_PitchControl, FIELD_INTEGER ),
	DEFINE_FIELD( m_MuzzleAttachment, FIELD_INTEGER ),
	DEFINE_FIELD( m_iAmmoType, FIELD_INTEGER ),

END_DATADESC()

enum CombineGuardSchedules 
{	
	SCHED_SYNTHELITE_RANGE_ATTACK1 = LAST_SHARED_SCHEDULE,
	SCHED_SYNTHELITE_CLOBBERED,
	SCHED_SYNTHELITE_TOPPLE,
	SCHED_SYNTHELITE_HELPLESS,
};

enum CombineGuardTasks 
{	
	TASK_SYNTHELITE_RANGE_ATTACK1 = LAST_SHARED_TASK,
	TASK_SYNTHELITE_SET_BALANCE,
};

enum CombineGuardConditions
{	
	COND_SYNTHELITE_CLOBBERED = LAST_SHARED_CONDITION,
};

int	ACT_SYNTHELITE_IDLE_TO_ANGRY;
int ACT_SYNTHELITE_CLOBBERED;
int ACT_SYNTHELITE_TOPPLE;
int ACT_SYNTHELITE_GETUP;
int ACT_SYNTHELITE_HELPLESS;

#define	SYNTHELITE_AE_SHOVE	11
#define	SYNTHELITE_AE_FIRE 12
#define	SYNTHELITE_AE_FIRE_START 13
#define	SYNTHELITE_AE_GLOW 14
#define SYNTHELITE_AE_LEFTFOOT 20
#define SYNTHELITE_AE_RIGHTFOOT	21
#define SYNTHELITE_AE_SHAKEIMPACT 22

CNPC_SynthElite::CNPC_SynthElite( void )
{
}

Class_T	CNPC_SynthElite::Classify ( void )
{
	if (FClassnameIs(this, "npc_combine_synthelite"))
	{
		return	CLASS_COMBINE;
	}

	if (FClassnameIs(this, "npc_combine_synthelite_ally"))
	{
		return	CLASS_CONSCRIPT;
	}

	return CLASS_NONE;

}

LINK_ENTITY_TO_CLASS( npc_islaveboss, CNPC_SynthElite ); //Obsolete, leaving this in anyway
LINK_ENTITY_TO_CLASS( npc_combine_synthelite, CNPC_SynthElite );
LINK_ENTITY_TO_CLASS( npc_combine_synthelite_ally, CNPC_SynthElite );

void CNPC_SynthElite::Precache( void )
{

	m_iAmmoType = GetAmmoDef()->Index("AR2");

	PrecacheModel( SYNTHELITE_MODEL );
	PrecacheModel("models/gibs/Antlion_gib_Large_2.mdl"); //Antlion gibs for now

	PrecacheModel( "sprites/blueflare1.vmt" ); //For some reason this appears between his feet.

	PrecacheScriptSound( "NPC_CombineGuard.FootstepLeft" );
	PrecacheScriptSound( "NPC_CombineGuard.FootstepRight" );
	PrecacheScriptSound( "NPC_Vortigaunt.Minigun" );
	PrecacheScriptSound( "NPC_CombineGuard.Charging" );

	PrecacheScriptSound( "VortBoss.Pain" );
	PrecacheScriptSound( "VortBoss.Die" );
	PrecacheScriptSound( "VortBoss.Alert" );
	PrecacheScriptSound( "VortBoss.Idle" );

	BaseClass::Precache();
}

void CNPC_SynthElite::Spawn( void )
{
	Precache();

	SetModel( SYNTHELITE_MODEL );

	SetHullType( HULL_WIDE_HUMAN );
	
	SetNavType( NAV_GROUND );
	m_NPCState = NPC_STATE_NONE;
	SetBloodColor( BLOOD_COLOR_BLUE );

	m_iHealth = m_iMaxHealth = sk_islaveboss_health.GetFloat();
	m_flFieldOfView = VIEW_FIELD_WIDE;

	
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );

	m_flGlowTime = gpGlobals -> curtime;
	m_flLastRangeTime = gpGlobals -> curtime;
	m_aimYaw = 0;
	m_aimPitch = 0;

	m_flNextClobberTime	= gpGlobals -> curtime;

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_INNATE_RANGE_ATTACK1 | bits_CAP_SQUAD );

	for ( int i = 1; i < NUM_SYNTHELITE_ATTACHMENTS; i++ )
	{
		m_pGlowSprite[i] = CSprite::SpriteCreate( "sprites/blueflare1.vmt", GetAbsOrigin(), false );
		
		Assert( m_pGlowSprite[i] );

		if ( m_pGlowSprite[i] == NULL )
			return;

		m_pGlowSprite[i] -> TurnOff();
		m_pGlowSprite[i] -> SetTransparency( kRenderGlow, 16, 16, 16, 255, kRenderFxNoDissipation );
		m_pGlowSprite[i] -> SetScale( 1.0f );
		m_pGlowSprite[i] -> SetAttachment( this, i );
	}

	NPCInit();
	
	
	SetBodygroup( 1, true );

	m_YawControl = LookupPoseParameter( "aim_yaw" );
	m_PitchControl = LookupPoseParameter( "aim_pitch" );
	m_MuzzleAttachment = LookupAttachment( "muzzle" );

	m_fOffBalance = false;

	BaseClass::Spawn();
}

void CNPC_SynthElite::DoMuzzleFlash( void )
{
	BaseClass::DoMuzzleFlash();
	
	CEffectData data;

	data.m_nAttachmentIndex = LookupAttachment( "Muzzle" );
	data.m_nEntIndex = entindex();
	DispatchEffect( "StriderMuzzleFlash", data );
}


void CNPC_SynthElite::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();
	
	if( HasCondition( COND_SYNTHELITE_CLOBBERED ) )
	{
		Msg( "CLOBBERED!\n" );
	}

	for ( int i = 1; i < NUM_SYNTHELITE_ATTACHMENTS; i++ )
	{
		if ( m_pGlowSprite[i] == NULL )
			continue;

		if ( m_flGlowTime > gpGlobals->curtime )
		{
			float brightness;
			float perc = ( m_flGlowTime - gpGlobals->curtime ) / SYNTHELITE_GLOW_TIME;

			m_pGlowSprite[i] -> TurnOn();

			brightness = perc * 92.0f;
			m_pGlowSprite[i] -> SetTransparency( kRenderGlow, brightness, brightness, brightness, 255, kRenderFxNoDissipation );
			
			m_pGlowSprite[i] -> SetScale( perc * 1.0f );
		}
		else
		{
			m_pGlowSprite[i] -> TurnOff();
		}
	}


	Vector vecDamagePoint;
	QAngle vecDamageAngles;
}





void CNPC_SynthElite::Event_Killed( const CTakeDamageInfo &info )
{
    BaseClass::Event_Killed( info );

//	UTIL_Remove(this);

//	ExplosionCreate(GetAbsOrigin(), GetAbsAngles(), NULL, random->RandomInt(30, 40), 0, true);

  //  Gib();
}

void CNPC_SynthElite::Gib(void)
{
	// Sparks
	for (int i = 0; i < 4; i++)
	{
		Vector sparkPos = GetAbsOrigin();
		sparkPos.x += random->RandomFloat(-12,12);
		sparkPos.y += random->RandomFloat(-12,12);
		sparkPos.z += random->RandomFloat(-12,12);
		g_pEffects->Sparks(sparkPos);
	}
	// Smoke
	UTIL_Smoke(GetAbsOrigin(), random->RandomInt(10, 15), 10);

	// Light
	CBroadcastRecipientFilter filter;
	te->DynamicLight( filter, 0.0,
			&GetAbsOrigin(), 255, 180, 100, 0, 100, 0.1, 0 );

	// Throw gibs
	CGib::SpawnSpecificGibs( this, SYNTHELITE_GIB_COUNT, 800, 1000, "models/gibs/Antlion_gib_Large_2.mdl");


	UTIL_Remove(this);
}


void CNPC_SynthElite::HandleAnimEvent( animevent_t *pEvent )
{
	switch ( pEvent -> event )
	{
	case SYNTHELITE_AE_SHAKEIMPACT:
		Shove();
		UTIL_ScreenShake( GetAbsOrigin(), 25.0, 150.0, 1.0, 750, SHAKE_START );
		break;

	case SYNTHELITE_AE_LEFTFOOT:
		{
			EmitSound( "NPC_CombineGuard.FootstepLeft" );
		}
		break;

	case SYNTHELITE_AE_RIGHTFOOT:
		{
			EmitSound( "NPC_CombineGuard.FootstepRight" );
		}
		break;
	
	case SYNTHELITE_AE_SHOVE:
		Shove();
		break;

	case SYNTHELITE_AE_FIRE:
		{
			m_flLastRangeTime = gpGlobals->curtime + 1.0f;
			//FireRangeWeapon();
			ShootMG();
			DoMuzzleFlash();
			
			EmitSound( "NPC_Vortigaunt.Minigun" );

			EntityMessageBegin( this, true );
				WRITE_BYTE( SYNTHELITE_MSG_SHOT );
			MessageEnd();
		}
		break;

	case SYNTHELITE_AE_FIRE_START:
		{
			EmitSound( "NPC_CombineGuard.Charging" );

			EntityMessageBegin( this, true );
				WRITE_BYTE( SYNTHELITE_MSG_SHOT_START );
			MessageEnd();
		}
		break;

	case SYNTHELITE_AE_GLOW:
		m_flGlowTime = gpGlobals->curtime + SYNTHELITE_GLOW_TIME;
		break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
		return;
	}
}

void CNPC_SynthElite::Shove( void ) // Doesn't work for some reason // It sure works now! -Dan
{
	if ( GetEnemy() == NULL )
		return;

	CBaseEntity *pHurt = NULL;

	Vector forward;
	AngleVectors( GetLocalAngles(), &forward );

	float flDist = ( GetEnemy()->GetAbsOrigin() - GetAbsOrigin() ).Length();
	Vector2D v2LOS = ( GetEnemy()->GetAbsOrigin() - GetAbsOrigin() ).AsVector2D();
	Vector2DNormalize( v2LOS );
	float flDot	= DotProduct2D ( v2LOS , forward.AsVector2D() );

	flDist -= GetEnemy() -> WorldAlignSize().x * 0.5f;

	if ( flDist < SYNTHELITE_MELEE1_RANGE && flDot >= SYNTHELITE_MELEE1_CONE )
	{
		Vector vStart = GetAbsOrigin();
		vStart.z += WorldAlignSize().z * 0.5;

		Vector vEnd = GetEnemy() -> GetAbsOrigin();
		vEnd.z += GetEnemy() -> WorldAlignSize().z * 0.5;

		pHurt = CheckTraceHullAttack( vStart, vEnd, Vector( -16, -16, 0 ), Vector( 16, 16, 24 ), 25, DMG_CLUB );
	}

	if ( pHurt )
	{
		pHurt -> ViewPunch( QAngle( -20, 0, 20 ) );

		UTIL_ScreenShake( pHurt -> GetAbsOrigin(), 100.0, 1.5, 1.0, 2, SHAKE_START );
		
		color32 white = { 255, 255, 255, 64 };
		UTIL_ScreenFade( pHurt, white, 0.5f, 0.1f, FFADE_IN );

		if ( pHurt->IsPlayer() )
		{
			Vector forward, up;
			AngleVectors( GetLocalAngles(), &forward, NULL, &up );
			pHurt->ApplyAbsVelocityImpulse( forward * 300 + up * 250 );
		}	
	}
}

int CNPC_SynthElite::SelectSchedule( void )
{

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

		if ( GetEnemy() == NULL) //&& GetFollowTarget() )
		{
			{

				return SCHED_TARGET_FACE;
			}
		}

		// try to say something about smells
		break;
	}
	
	return BaseClass::SelectSchedule();
}


void CNPC_SynthElite::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_SYNTHELITE_SET_BALANCE:
		if( pTask->flTaskData == 0.0 )
		{
			m_fOffBalance = false;
		}
		else
		{
			m_fOffBalance = true;
		}

		TaskComplete();

		break;

	case TASK_SYNTHELITE_RANGE_ATTACK1:
		{
			Vector flEnemyLKP = GetEnemyLKP();
			GetMotor()->SetIdealYawToTarget( flEnemyLKP );
		}
		SetActivity( ACT_RANGE_ATTACK1 );
		return;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}

void CNPC_SynthElite::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_SYNTHELITE_RANGE_ATTACK1:
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

Activity CNPC_SynthElite::NPC_TranslateActivity( Activity baseAct )
{

	if ( baseAct == ACT_IDLE && m_NPCState != NPC_STATE_IDLE )
	{
		return ( Activity ) ACT_IDLE_ANGRY;
	}

	return baseAct;
}

int CNPC_SynthElite::TranslateSchedule( int type ) 
{
	switch( type )
	{
	case SCHED_RANGE_ATTACK1:
		return SCHED_SYNTHELITE_RANGE_ATTACK1;
		break;
	}

	return BaseClass::TranslateSchedule( type );
}





int CNPC_SynthElite::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	CTakeDamageInfo newInfo = info;
	if( newInfo.GetDamageType() & DMG_BURN)
	{
		newInfo.ScaleDamage( 0.9 );
	}	

	int nDamageTaken = BaseClass::OnTakeDamage_Alive( newInfo );


	return nDamageTaken;
}


int CNPC_SynthElite::MeleeAttack1Conditions( float flDot, float flDist )
{
	if ( flDist > SYNTHELITE_MELEE1_RANGE )
		return COND_TOO_FAR_TO_ATTACK;
	
	if ( flDot < SYNTHELITE_MELEE1_CONE )
		return COND_NOT_FACING_ATTACK;

	return COND_CAN_MELEE_ATTACK1;
}

int CNPC_SynthElite::RangeAttack1Conditions( float flDot, float flDist )
{
	if ( flDist > 1600 )
		return COND_TOO_FAR_TO_ATTACK;
	
	if ( flDot < 0.0f )
		return COND_NOT_FACING_ATTACK;

	if ( m_flLastRangeTime > gpGlobals->curtime )
		return COND_TOO_FAR_TO_ATTACK;

	return COND_CAN_RANGE_ATTACK1;
}



#define	DEBUG_AIMING 0



float CNPC_SynthElite::MaxYawSpeed( void )
{
	float flYS = 0;

	switch ( GetActivity() )
	{
	case	ACT_WALK:			flYS = 30;	break;
	case	ACT_RUN:			flYS = 90;	break;
	case	ACT_IDLE:			flYS = 90;	break;
	default:
		flYS = 90;
		break;
	}

	return flYS;
}

void CNPC_SynthElite::BuildScheduleTestBits( void )
{
	SetCustomInterruptCondition( COND_SYNTHELITE_CLOBBERED );
}


AI_BEGIN_CUSTOM_NPC( npc_combine_synthelite, CNPC_SynthElite )

	DECLARE_TASK( TASK_SYNTHELITE_RANGE_ATTACK1 )
	DECLARE_TASK( TASK_SYNTHELITE_SET_BALANCE )
	
	DECLARE_CONDITION( COND_SYNTHELITE_CLOBBERED )

	DECLARE_ACTIVITY( ACT_SYNTHELITE_IDLE_TO_ANGRY )
	DECLARE_ACTIVITY( ACT_SYNTHELITE_CLOBBERED )
	DECLARE_ACTIVITY( ACT_SYNTHELITE_TOPPLE )
	DECLARE_ACTIVITY( ACT_SYNTHELITE_GETUP )
	DECLARE_ACTIVITY( ACT_SYNTHELITE_HELPLESS )

	DEFINE_SCHEDULE
	(
		SCHED_SYNTHELITE_RANGE_ATTACK1,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_FACE_ENEMY				0"
		"		TASK_ANNOUNCE_ATTACK		1"
		"		TASK_SYNTHELITE_RANGE_ATTACK1	0"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_NO_PRIMARY_AMMO"
	)

	DEFINE_SCHEDULE
	(
		SCHED_SYNTHELITE_CLOBBERED,

		"	Tasks"
		"		TASK_STOP_MOVING						0"
		"		TASK_SYNTHELITE_SET_BALANCE			1"
		"		TASK_PLAY_SEQUENCE						ACTIVITY:ACT_SYNTHELITE_CLOBBERED"
		"		TASK_SYNTHELITE_SET_BALANCE			0"
		"	"
		"	Interrupts"
	)

	DEFINE_SCHEDULE
	(
		SCHED_SYNTHELITE_TOPPLE,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_SYNTHELITE_TOPPLE"
		"		TASK_WAIT						1"
		"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_SYNTHELITE_GETUP"
		"		TASK_SYNTHELITE_SET_BALANCE	0"
		"	"
		"	Interrupts"
	)

	DEFINE_SCHEDULE
	(
		SCHED_SYNTHELITE_HELPLESS,

		"	Tasks"
		"	TASK_STOP_MOVING				0"
		"	TASK_PLAY_SEQUENCE				ACTIVITY:ACT_SYNTHELITE_TOPPLE"
		"	TASK_WAIT						2"
		"	TASK_PLAY_SEQUENCE				ACTIVITY:ACT_SYNTHELITE_HELPLESS"
		"	TASK_WAIT_INDEFINITE			0"
		"	"
		"	Interrupts"
	)

AI_END_CUSTOM_NPC()



