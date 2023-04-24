//========= Copyright © LOLOLOL, All rights reserved. ============//
//
// Purpose: Militia Dudes with Miniguns
//
//
//=============================================================================//



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
#include "AI_Squad.h"
#include "soundenvelope.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "te_effect_dispatch.h"
#include "te_particlesystem.h"

#include "npc_playercompanion.h"


#include 	<time.h>

#define MILITIAGUNNER_GIB_COUNT			5 

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar sk_militiagunner_health( "sk_militiagunner_health", "800" );

#define CONCEPT_PLAYER_USE "FollowWhenUsed"

class CSprite;

extern void CreateConcussiveBlast( const Vector &origin, const Vector &surfaceNormal, CBaseEntity *pOwner, float magnitude );

#define	MILITIAGUNNER_MODEL	"models/hassault05.mdl"

#define MILITIAGUNNER_MSG_SHOT	1
#define MILITIAGUNNER_MSG_SHOT_START 2

enum 
{
	MILITIAGUNNER_REF_INVALID = 0,
	MILITIAGUNNER_REF_MUZZLE,
	MILITIAGUNNER_REF_LEFT_SHOULDER,
	MILITIAGUNNER_REF_HUMAN_HEAD,
	MILITIAGUNNER_REF_RIGHT_ARM_HIGH,
	MILITIAGUNNER_REF_RIGHT_ARM_LOW,
	MILITIAGUNNER_REF_LEFT_ARM_HIGH,
	MILITIAGUNNER_REF_LEFT_ARM_LOW,
	MILITIAGUNNER_REF_TORSO,
	MILITIAGUNNER_REF_LOWER_TORSO,
	MILITIAGUNNER_REF_RIGHT_THIGH_HIGH,
	MILITIAGUNNER_REF_RIGHT_THIGH_LOW,
	MILITIAGUNNER_REF_LEFT_THIGH_HIGH,
	MILITIAGUNNER_REF_LEFT_THIGH_LOW,
//	MILITIAGUNNER_SHOVE,
	MILITIAGUNNER_REF_RIGHT_SHIN_HIGH,
	MILITIAGUNNER_REF_RIGHT_SHIN_LOW,
	MILITIAGUNNER_REF_LEFT_SHIN_HIGH,
	MILITIAGUNNER_REF_LEFT_SHIN_LOW,
	MILITIAGUNNER_REF_RIGHT_SHOULDER,
	
	NUM_MILITIAGUNNER_ATTACHMENTS,
};




class CNPC_Militiagunner : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_Militiagunner, CAI_BaseNPC);
public:

	DECLARE_SERVERCLASS();

	CNPC_Militiagunner( void );

	int	OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void Event_Killed( const CTakeDamageInfo &info );
	int	TranslateSchedule( int type );
	int	MeleeAttack1Conditions( float flDot, float flDist );
	int	RangeAttack1Conditions( float flDot, float flDist );
	void Gib(void);
	void ShootMG( void );


	int		GetSoundInterests ( void );

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

	mutable float	m_flJumpDist;
	bool IsJumpLegal(const Vector &startPos, const Vector &apex, const Vector &endPos) const;



	bool m_fOffBalance;
	float m_flNextClobberTime;

	int	GetReferencePointForBodyGroup( int bodyGroup );

	bool CreateBehaviors( void );

	void Shove( void );
	void FireRangeWeapon( void );
	void	MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType );

	CSprite *m_pGlowSprite[NUM_MILITIAGUNNER_ATTACHMENTS];


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

void CNPC_Militiagunner::PainSound( const CTakeDamageInfo &info )
{

	EmitSound( "Hassault.Pain" );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Militiagunner::DeathSound( const CTakeDamageInfo &info ) 
{
	EmitSound( "Hassault.Die" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Militiagunner::AlertSound( void )
{
	EmitSound( "Hassault.Alert" );

}


void CNPC_Militiagunner::ShootMG( void )
{
	Vector vecShootOrigin;

//	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = GetAbsOrigin() + Vector( 0, 0, 55 );
	Vector vecShootDir = GetShootEnemyDir( vecShootOrigin );
	QAngle angDir;
	QAngle	angShootDir;

	CEffectData data;
	data.m_nAttachmentIndex = LookupAttachment( "Muzzle" );
	data.m_nEntIndex = entindex();
	GetAttachment("Muzzle", vecShootOrigin, angShootDir);
	

	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 8000, m_iAmmoType  ); //originally 2 bullets at a time w/ 10 degree spread

}

void CNPC_Militiagunner::MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType )
{
	float flTracerDist;
	Vector vecDir;
	Vector vecEndPos;
		Vector vecSrc;
		QAngle angAngles;

	vecDir = tr.endpos - vecTracerSrc;

	flTracerDist = VectorNormalize( vecDir );

	CEffectData data;

	data.m_nAttachmentIndex = LookupAttachment( "Muzzle" );
	data.m_nEntIndex = entindex();
	GetAttachment("Muzzle", vecSrc, angAngles);

	UTIL_Tracer( vecSrc, tr.endpos, 0, TRACER_DONT_USE_ATTACHMENT, 2500, true, "AR2Tracer" );
}

#define	MILITIAGUNNER_DEFAULT_ARMOR_HEALTH	50

#define	MILITIAGUNNER_MELEE1_RANGE	92
#define	MILITIAGUNNER_MELEE1_CONE	0.5f

#define	MILITIAGUNNER_RANGE1_RANGE	600
#define	MILITIAGUNNER_RANGE1_CONE	0.0f

#define	MILITIAGUNNER_GLOW_TIME			0.5f

IMPLEMENT_SERVERCLASS_ST(CNPC_Militiagunner, DT_MONSTER_MILITIAGUNNER)
END_SEND_TABLE()

BEGIN_DATADESC( CNPC_Militiagunner )

	DEFINE_FIELD( m_fOffBalance, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flNextClobberTime, FIELD_TIME ),
	DEFINE_ARRAY( m_pGlowSprite, FIELD_CLASSPTR, NUM_MILITIAGUNNER_ATTACHMENTS ),
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
	SCHED_MILITIAGUNNER_RANGE_ATTACK1 = LAST_SHARED_SCHEDULE,
	SCHED_MILITIAGUNNER_FOLLOW,
};

enum CombineGuardTasks 
{	
	TASK_MILITIAGUNNER_RANGE_ATTACK1 = LAST_SHARED_TASK,
	TASK_MILITIAGUNNER_SET_BALANCE,
};

enum CombineGuardConditions
{	
	COND_MILITIAGUNNER_CLOBBERED = LAST_SHARED_CONDITION,
};

int	ACT_MILITIAGUNNER_IDLE_TO_ANGRY;
int ACT_MILITIAGUNNER_CLOBBERED;
int ACT_MILITIAGUNNER_TOPPLE;
int ACT_MILITIAGUNNER_GETUP;
int ACT_MILITIAGUNNER_HELPLESS;

#define	MILITIAGUNNER_AE_SHOVE	11
#define	MILITIAGUNNER_AE_FIRE 12
#define	MILITIAGUNNER_AE_FIRE_START 13
#define	MILITIAGUNNER_AE_GLOW 14
#define MILITIAGUNNER_AE_LEFTFOOT 20
#define MILITIAGUNNER_AE_RIGHTFOOT	21
#define MILITIAGUNNER_AE_SHAKEIMPACT 22

CNPC_Militiagunner::CNPC_Militiagunner( void )
{
}

Class_T	CNPC_Militiagunner::Classify ( void )
{
	return	CLASS_CONSCRIPT;
}

LINK_ENTITY_TO_CLASS( monster_human_assault, CNPC_Militiagunner );

void CNPC_Militiagunner::Precache( void )
{

	m_iAmmoType = GetAmmoDef()->Index("AirboatGun"); //Set to AirboatGun so combine "vehicles" can be killed

	PrecacheModel( STRING( GetModelName() ) );
	PrecacheModel( "models/hassault01.mdl" );
	PrecacheModel( "models/hassault02.mdl" );
	PrecacheModel( "models/hassault03.mdl" );
	PrecacheModel( "models/hassault04.mdl" );
	PrecacheModel( "models/hassault05.mdl" );

	PrecacheModel("models/gibs/Antlion_gib_Large_2.mdl"); //Antlion gibs for now

	PrecacheModel( "sprites/blueflare1.vmt" ); //For some reason this appears between his feet.

	PrecacheScriptSound( "NPC_CombineGuard.FootstepLeft" );
	PrecacheScriptSound( "NPC_CombineGuard.FootstepRight" );
	PrecacheScriptSound( "Hassault.Minigun" );
	PrecacheScriptSound( "NPC_CombineGuard.Charging" );

	PrecacheScriptSound( "Hassault.Pain" );
	PrecacheScriptSound( "Hassault.Die" );
	PrecacheScriptSound( "Hassault.Alert" );
	PrecacheScriptSound( "Hassault.Idle" );

	BaseClass::Precache();
}

void CNPC_Militiagunner::Spawn( void )
{
	Precache();

	char *szModel = (char *)STRING( GetModelName() );
	if (!szModel || !*szModel)
	{
		int iRand = random->RandomInt( 1, 5 );
		switch (iRand)
		{
		case 1:
			szModel = "models/hassault01.mdl";
			break;
		case 2:
			szModel = "models/hassault02.mdl";
			break;
		case 3:
			szModel = "models/hassault03.mdl";
			break;
		case 4:
			szModel = "models/hassault04.mdl";
			break;
		case 5:
			szModel = "models/hassault05.mdl";
			break;
		}
	}

	SetModel( szModel );   

	SetHullType( HULL_WIDE_HUMAN );
	
	SetNavType( NAV_GROUND );
	m_NPCState = NPC_STATE_NONE;
	SetBloodColor( BLOOD_COLOR_RED );

	m_iHealth = m_iMaxHealth = sk_militiagunner_health.GetFloat();
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
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_MOVE_JUMP | bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_INNATE_RANGE_ATTACK1 | bits_CAP_SQUAD | bits_CAP_NO_HIT_SQUADMATES | bits_CAP_FRIENDLY_DMG_IMMUNE );

	for ( int i = 1; i < NUM_MILITIAGUNNER_ATTACHMENTS; i++ )
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

int CNPC_Militiagunner::GetSoundInterests ( void) 
{
	return	SOUND_WORLD	|
			SOUND_COMBAT	|
			SOUND_CARCASS	|
			SOUND_MEAT		|
			SOUND_GARBAGE	|
			SOUND_DANGER	|
			SOUND_PLAYER;
}


void CNPC_Militiagunner::DoMuzzleFlash( void )
{
	BaseClass::DoMuzzleFlash();
	
	CEffectData data;

	data.m_nAttachmentIndex = LookupAttachment( "Muzzle" );
	data.m_nEntIndex = entindex();
	DispatchEffect( "AirboatMuzzleFlash", data );
}


void CNPC_Militiagunner::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();
	
	if( HasCondition( COND_MILITIAGUNNER_CLOBBERED ) )
	{
		Msg( "CLOBBERED!\n" );
	}

	for ( int i = 1; i < NUM_MILITIAGUNNER_ATTACHMENTS; i++ )
	{
		if ( m_pGlowSprite[i] == NULL )
			continue;

		if ( m_flGlowTime > gpGlobals->curtime )
		{
			float brightness;
			float perc = ( m_flGlowTime - gpGlobals->curtime ) / MILITIAGUNNER_GLOW_TIME;

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





void CNPC_Militiagunner::Event_Killed( const CTakeDamageInfo &info )
{
    BaseClass::Event_Killed( info );

//	UTIL_Remove(this);

//	ExplosionCreate(GetAbsOrigin(), GetAbsAngles(), NULL, random->RandomInt(30, 40), 0, true);

  //  Gib();
}

void CNPC_Militiagunner::Gib(void)
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
	CGib::SpawnSpecificGibs( this, MILITIAGUNNER_GIB_COUNT, 800, 1000, "models/gibs/Antlion_gib_Large_2.mdl");


	UTIL_Remove(this);
}


void CNPC_Militiagunner::HandleAnimEvent( animevent_t *pEvent )
{
	switch ( pEvent -> event )
	{
	case MILITIAGUNNER_AE_SHAKEIMPACT:
		Shove();
		UTIL_ScreenShake( GetAbsOrigin(), 25.0, 150.0, 1.0, 750, SHAKE_START );
		break;

	case MILITIAGUNNER_AE_LEFTFOOT:
		{
			EmitSound( "NPC_CombineGuard.FootstepLeft" );
		}
		break;

	case MILITIAGUNNER_AE_RIGHTFOOT:
		{
			EmitSound( "NPC_CombineGuard.FootstepRight" );
		}
		break;
	
	case MILITIAGUNNER_AE_SHOVE:
		Shove();
		break;

	case MILITIAGUNNER_AE_FIRE:
		{
			m_flLastRangeTime = gpGlobals->curtime + 1.0f;
			//FireRangeWeapon();
			ShootMG();
			DoMuzzleFlash();
			
			EmitSound( "Hassault.Minigun" );

			EntityMessageBegin( this, true );
				WRITE_BYTE( MILITIAGUNNER_MSG_SHOT );
			MessageEnd();
		}
		break;

	case MILITIAGUNNER_AE_FIRE_START:
		{
			EmitSound( "NPC_CombineGuard.Charging" );

			EntityMessageBegin( this, true );
				WRITE_BYTE( MILITIAGUNNER_MSG_SHOT_START );
			MessageEnd();
		}
		break;

	case MILITIAGUNNER_AE_GLOW:
		m_flGlowTime = gpGlobals->curtime + MILITIAGUNNER_GLOW_TIME;
		break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
		return;
	}
}

void CNPC_Militiagunner::Shove( void ) // Doesn't work for some reason // It sure works now! -Dan
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

	if ( flDist < MILITIAGUNNER_MELEE1_RANGE && flDot >= MILITIAGUNNER_MELEE1_CONE )
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

bool CNPC_Militiagunner::IsJumpLegal(const Vector &startPos, const Vector &apex, const Vector &endPos) const
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


int CNPC_Militiagunner::SelectSchedule( void )
{

		if (m_NPCState == NPC_STATE_IDLE)
			return SCHED_MILITIAGUNNER_FOLLOW;

		if (m_NPCState == NPC_STATE_ALERT)
			return SCHED_MILITIAGUNNER_FOLLOW;


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


			if( HasCondition( COND_SEE_ENEMY ) ) /////////added this - Should pursue enemies now..
			{
				if( HasCondition( COND_TOO_FAR_TO_ATTACK ) )
				{
				return SCHED_CHASE_ENEMY;
				}
			}

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


void CNPC_Militiagunner::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_MILITIAGUNNER_SET_BALANCE:
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

	case TASK_MILITIAGUNNER_RANGE_ATTACK1:
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

void CNPC_Militiagunner::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_MILITIAGUNNER_RANGE_ATTACK1:
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

Activity CNPC_Militiagunner::NPC_TranslateActivity( Activity baseAct )
{

	if ( baseAct == ACT_IDLE && m_NPCState != NPC_STATE_IDLE )
	{
		return ( Activity ) ACT_IDLE_ANGRY;
	}

	return baseAct;
}

int CNPC_Militiagunner::TranslateSchedule( int type ) 
{
	switch( type )
	{
	case SCHED_RANGE_ATTACK1:
		return SCHED_MILITIAGUNNER_RANGE_ATTACK1;
		break;
	}

	return BaseClass::TranslateSchedule( type );
}





int CNPC_Militiagunner::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	CTakeDamageInfo newInfo = info;
	if( newInfo.GetDamageType() & DMG_BURN)
	{
		newInfo.ScaleDamage( 0 );
		DevMsg( "Fire damage; no actual damage is taken\n" );
	}	

	int nDamageTaken = BaseClass::OnTakeDamage_Alive( newInfo );


	return nDamageTaken;
}


int CNPC_Militiagunner::MeleeAttack1Conditions( float flDot, float flDist )
{
	if ( flDist > MILITIAGUNNER_MELEE1_RANGE )
		return COND_TOO_FAR_TO_ATTACK;
	
	if ( flDot < MILITIAGUNNER_MELEE1_CONE )
		return COND_NOT_FACING_ATTACK;

	return COND_CAN_MELEE_ATTACK1;
}

int CNPC_Militiagunner::RangeAttack1Conditions( float flDot, float flDist )
{
	if ( flDist > 3072 )
		return COND_TOO_FAR_TO_ATTACK;
	
	if ( flDot < 0.0f )
		return COND_NOT_FACING_ATTACK;

	if ( m_flLastRangeTime > gpGlobals->curtime )
		return COND_TOO_FAR_TO_ATTACK;

	return COND_CAN_RANGE_ATTACK1;
}



#define	DEBUG_AIMING 0



float CNPC_Militiagunner::MaxYawSpeed( void )
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

void CNPC_Militiagunner::BuildScheduleTestBits( void )
{
	SetCustomInterruptCondition( COND_MILITIAGUNNER_CLOBBERED );
}


AI_BEGIN_CUSTOM_NPC( npc_thresh_assault, CNPC_Militiagunner )

	DECLARE_TASK( TASK_MILITIAGUNNER_RANGE_ATTACK1 )
	DECLARE_TASK( TASK_MILITIAGUNNER_SET_BALANCE )
	
	DECLARE_CONDITION( COND_MILITIAGUNNER_CLOBBERED )

	DECLARE_ACTIVITY( ACT_MILITIAGUNNER_IDLE_TO_ANGRY )
	DECLARE_ACTIVITY( ACT_MILITIAGUNNER_CLOBBERED )
	DECLARE_ACTIVITY( ACT_MILITIAGUNNER_TOPPLE )
	DECLARE_ACTIVITY( ACT_MILITIAGUNNER_GETUP )
	DECLARE_ACTIVITY( ACT_MILITIAGUNNER_HELPLESS )

	DEFINE_SCHEDULE
	(
		SCHED_MILITIAGUNNER_RANGE_ATTACK1,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_FACE_ENEMY				0"
		"		TASK_ANNOUNCE_ATTACK		1"
		"		TASK_MILITIAGUNNER_RANGE_ATTACK1	0"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_NO_PRIMARY_AMMO"
	)

	DEFINE_SCHEDULE
	(
		SCHED_MILITIAGUNNER_FOLLOW,
	
		"	Tasks"
		"		TASK_GET_PATH_TO_PLAYER			0"
		"		TASK_RUN_PATH					0"
		"		TASK_RUN_PATH_WITHIN_DIST		150"
		"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_TARGET_FACE"
		"	"
		"	Interrupts"
		"			COND_NEW_ENEMY"
	//	"			COND_LIGHT_DAMAGE"
	//	"			COND_HEAVY_DAMAGE"
		"			COND_HEAR_DANGER"
		"			COND_PROVOKED"
	)

AI_END_CUSTOM_NPC()



