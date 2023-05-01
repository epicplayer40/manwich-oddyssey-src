//========= Copyright © LOLOLOL, All rights reserved. ============//
//
// Purpose: General Schremb, the evil man who oppresses the people of Hongo
//
//
//=============================================================================//


#include "cbase.h"
#include "ai_task.h"
#include "ai_default.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "ai_motor.h"
#include "ai_memory.h"
#include "npc_combine.h"
#include "physics.h"
#include "bitstring.h"
#include "activitylist.h"
#include "game.h"
#include "npcevent.h"
#include "player.h"
#include "entitylist.h"
#include "ai_interactions.h"
#include "soundent.h"
#include "gib.h"
#include "shake.h"
#include "ammodef.h"
#include "Sprite.h"
#include "explode.h"
#include "grenade_homer.h"
#include "ai_basenpc.h"
#include "soundenvelope.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "te_effect_dispatch.h"
#include "te_particlesystem.h"

#define FBOSS_GIB_COUNT			5   //Assuming we get a new modeller who can make these for us.

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar sk_finalboss_health( "sk_finalboss_health", "9500" );

class CSprite;

extern void CreateConcussiveBlast( const Vector &origin, const Vector &surfaceNormal, CBaseEntity *pOwner, float magnitude );

#define	FINALBOSS_MODEL	"models/final_boss.mdl"

#define FBOSS_MSG_SHOT	1
#define FBOSS_MSG_SHOT_START 2



class CNPC_FinalBoss : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_FinalBoss, CAI_BaseNPC );
public:

//	DECLARE_SERVERCLASS();

	CNPC_FinalBoss( void );

//	int	OnTakeDamage( const CTakeDamageInfo &info );
	int	OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void Event_Killed( const CTakeDamageInfo &info );
	int	TranslateSchedule( int type );
	int	MeleeAttack1Conditions( float flDot, float flDist );
	int	RangeAttack1Conditions( float flDot, float flDist );
	void Gib(void);
	void ImmoBeam( int side );
	int	  m_iBeams;
	int	  m_iNoise;

	CBeam *m_pBeam[1];
	CBeam *m_hNoise[5];
	int		m_iAmmoType;

	void Precache( void );
	void Spawn( void );
			void ShootMG( void );
				void	MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType );
	void PrescheduleThink( void );
	void TraceAttack( CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, trace_t *ptr, int bitsDamageType );
	void HandleAnimEvent( animevent_t *pEvent );
	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );

	void			DoMuzzleFlash( void );

	bool AllArmorDestroyed( void );
	bool IsArmorPiece( int iArmorPiece );

	float MaxYawSpeed( void );
	void ClearBeams( void );

	Class_T Classify( void ) { return CLASS_COMBINE; }

	Activity NPC_TranslateActivity( Activity baseAct );

	virtual int	SelectSchedule( void );

	void BuildScheduleTestBits( void );

	DECLARE_DATADESC();

private:

	bool m_fOffBalance;
	float m_flNextClobberTime;

	int	GetReferencePointForBodyGroup( int bodyGroup );

	void DamageArmorPiece( int pieceID, float damage, const Vector &vecOrigin, const Vector &vecDir );
	void DestroyArmorPiece( int pieceID );
	void Shove( void );
	void FireRangeWeapon( void );


	bool AimGunAt( CBaseEntity *pEntity, float flInterval );


	int	m_nGibModel;

	float m_flGlowTime;
	float m_flLastRangeTime;

	float m_aimYaw;
	float m_aimPitch;

	int	m_YawControl;
	int	m_PitchControl;
	int	m_MuzzleAttachment;

	DEFINE_CUSTOM_AI;
};

#define	FBOSS_DEFAULT_ARMOR_HEALTH	50

#define	FINALBOSS_MELEE1_RANGE	92
#define	FINALBOSS_MELEE1_CONE	0.5f

#define	FINALBOSS_RANGE1_RANGE	1024
#define	FINALBOSS_RANGE1_CONE	0.0f

#define	FBOSS_GLOW_TIME			0.5f

//IMPLEMENT_SERVERCLASS_ST(CNPC_FinalBoss, DT_NPC_FinalBoss)
//END_SEND_TABLE()

BEGIN_DATADESC( CNPC_FinalBoss )

	DEFINE_FIELD( m_fOffBalance, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flNextClobberTime, FIELD_TIME ),
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

enum FinalBossSchedules 
{	
	SCHED_FBOSS_RANGE_ATTACK1 = LAST_SHARED_SCHEDULE,

};

enum FinalBossTasks 
{	
	TASK_FBOSS_RANGE_ATTACK1 = LAST_SHARED_TASK,
	TASK_FINALBOSS_SET_BALANCE,
};

enum FinalBossConditions
{	
	COND_FINALBOSS_CLOBBERED = LAST_SHARED_CONDITION,
};

int	ACT_FBOSS_IDLE_TO_ANGRY;
int ACT_FINALBOSS_CLOBBERED;
int ACT_FINALBOSS_TOPPLE;
int ACT_FINALBOSS_GETUP;
int ACT_FINALBOSS_HELPLESS;

#define	FBOSS_AE_IMMO_SHOOT	4
#define	FBOSS_AE_IMMO_DONE	5
#define	FBOSS_AE_SHOVE	11
#define	FBOSS_AE_FIRE 12
#define	FBOSS_AE_FIREMG 3
#define	FBOSS_AE_FIRE_START 13
#define	FBOSS_AE_GLOW 14
#define FBOSS_AE_LEFTFOOT 20
#define FBOSS_AE_RIGHTFOOT	21
#define FBOSS_AE_SHAKEIMPACT 22

CNPC_FinalBoss::CNPC_FinalBoss( void )
{
}

LINK_ENTITY_TO_CLASS( npc_finalboss, CNPC_FinalBoss );

void CNPC_FinalBoss::Precache( void )
{
	PrecacheModel( FINALBOSS_MODEL );

	m_iAmmoType = GetAmmoDef()->Index("AR2");

	PrecacheModel("models/gibs/Antlion_gib_Large_2.mdl"); //Antlion gibs for now

	PrecacheModel( "sprites/blueflare1.vmt" ); //For some reason this appears between his feet.

	PrecacheScriptSound( "NPC_FinalBoss.FootstepLeft" );
	PrecacheScriptSound( "NPC_FinalBoss.FootstepRight" );
	PrecacheScriptSound( "NPC_CombineGuard.Fire" );
	PrecacheScriptSound( "NPC_Vortigaunt.Minigun" );
	PrecacheScriptSound( "HL1Cremator.ImmoShoot" );
	PrecacheScriptSound( "NPC_CombineGuard.Charging" );

	BaseClass::Precache();
}



void CNPC_FinalBoss::Spawn( void )
{
	Precache();

	SetModel( FINALBOSS_MODEL );
	SetModelScale(1.3);
	SetHullType( HULL_WIDE_HUMAN );
	
	SetNavType( NAV_GROUND );
	m_NPCState = NPC_STATE_NONE;
	SetBloodColor( BLOOD_COLOR_YELLOW );

	m_iHealth = m_iMaxHealth = sk_finalboss_health.GetFloat();
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
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_INNATE_RANGE_ATTACK1 );


	NPCInit();
	
	
	SetBodygroup( 1, true );

	m_YawControl = LookupPoseParameter( "aim_yaw" );
	m_PitchControl = LookupPoseParameter( "aim_pitch" );
	m_MuzzleAttachment = LookupAttachment( "muzzle" );

	m_fOffBalance = false;

	BaseClass::Spawn();
}

void CNPC_FinalBoss::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();
	
	if( HasCondition( COND_FINALBOSS_CLOBBERED ) )
	{
		Msg( "CLOBBERED!\n" );
	}

	if ( GetEnemy() != NULL )
	{
		AimGunAt( GetEnemy(), 0.1f );
	}

	Vector vecDamagePoint;
	QAngle vecDamageAngles;


}

void CNPC_FinalBoss::Event_Killed( const CTakeDamageInfo &info )
{
    BaseClass::Event_Killed( info );

	ClearBeams( );

}



void CNPC_FinalBoss::HandleAnimEvent( animevent_t *pEvent )
{
	switch ( pEvent -> event )
	{
	case FBOSS_AE_SHAKEIMPACT:
		Shove();
		UTIL_ScreenShake( GetAbsOrigin(), 25.0, 150.0, 1.0, 750, SHAKE_START );
		break;

	case FBOSS_AE_LEFTFOOT:
		{
			EmitSound( "NPC_FinalBoss.FootstepLeft" );
		}
		break;

	case FBOSS_AE_RIGHTFOOT:
		{
			EmitSound( "NPC_FinalBoss.FootstepRight" );
		}
		break;


		case FBOSS_AE_IMMO_SHOOT:
		{
			ClearBeams( );
			EHANDLE m_hDead;

			if ( m_hDead != NULL )
			{
				Vector vecDest = m_hDead->GetAbsOrigin() + Vector( 0, 0, 38 );
				trace_t trace;
				UTIL_TraceHull( vecDest, vecDest, GetHullMins(), GetHullMaxs(),MASK_SOLID, m_hDead, COLLISION_GROUP_NONE, &trace );
			}
			
			ClearMultiDamage();

			ImmoBeam( -1 );
			ImmoBeam( 1 );

			CPASAttenuationFilter filter4( this );
			EmitSound( filter4, entindex(), "HL1Cremator.ImmoShoot" );
			ApplyMultiDamage();

			CBroadcastRecipientFilter filter;
//			te->DynamicLight( filter, 0.0, &GetAbsOrigin(), 0, 255, 0, 2, 256, 0.2 / m_flPlaybackRate, 0 );
			te->DynamicLight( filter, 0.0, &GetAbsOrigin(), 255, 255, 0, 2, 256, 0.2 / m_flPlaybackRate, 0 );

			m_flNextAttack = gpGlobals->curtime + random->RandomFloat( 0.5, 1.0 );
		}
		break;
		
		case FBOSS_AE_IMMO_DONE:
		{
			ClearBeams();
		}
		break;
	
	case FBOSS_AE_SHOVE:
		Shove();
		break;

	case FBOSS_AE_FIREMG:
		{

			m_flLastRangeTime = gpGlobals->curtime + 0.5f;
			//FireRangeWeapon();
			ShootMG();
			DoMuzzleFlash();
			
			EmitSound( "NPC_Vortigaunt.Minigun" );

			EntityMessageBegin( this, true );
				WRITE_BYTE( FBOSS_MSG_SHOT );
			MessageEnd();
		}
		break;

	case FBOSS_AE_FIRE:
		{
			m_flLastRangeTime = gpGlobals->curtime + 0.5f;
			FireRangeWeapon();
			
			EmitSound( "NPC_CombineGuard.Fire" );

			EntityMessageBegin( this, true );
				WRITE_BYTE( FBOSS_MSG_SHOT );
			MessageEnd();
		}
		break;

	case FBOSS_AE_FIRE_START:
		{
			EmitSound( "NPC_CombineGuard.Charging" );

			EntityMessageBegin( this, true );
				WRITE_BYTE( FBOSS_MSG_SHOT_START );
			MessageEnd();
		}
		break;

	case FBOSS_AE_GLOW:
		m_flGlowTime = gpGlobals->curtime + FBOSS_GLOW_TIME;
		break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
		return;
	}
}

void CNPC_FinalBoss::ShootMG( void )
{
	Vector vecShootOrigin;

//	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = GetAbsOrigin() + Vector( 0, 0, 55 );
	Vector vecShootDir = GetShootEnemyDir( vecShootOrigin );
	QAngle angDir;
	
	QAngle	angShootDir;
	//GetAttachment( LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir ); //Horrid Accuracy
	//VectorAngles( vecShootDir, angDir );
	//SetBlending( 0, angDir.x );
	//pev->effects = EF_MUZZLEFLASH;
	FireBullets(2, vecShootOrigin, vecShootDir, VECTOR_CONE_9DEGREES, 2048, m_iAmmoType  ); //originally 2 bullets at a time w/ 10 degree spread
//	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "hass/minigun_shoot.wav", 1, ATTN_NORM, 0, 100 );
}

void CNPC_FinalBoss::MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType )
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


void CNPC_FinalBoss::ImmoBeam( int side )
{
	Vector vecSrc, vecAim;
	trace_t tr;
	CBaseEntity *pEntity;

	if ( m_iBeams >= 1 )
		 return;
	if (m_iNoise >= 5 )
		return;

	Vector forward, right, up;
	AngleVectors( GetAbsAngles(), &forward, &right, &up );

	vecSrc = GetAbsOrigin() + up * 36;
	vecAim = GetShootEnemyDir( vecSrc );
	float deflection = 0;
	vecAim = vecAim + side * right * random->RandomFloat( 0, deflection ) + up * random->RandomFloat( -deflection, deflection );
	UTIL_TraceLine ( vecSrc, vecSrc + vecAim * 2048, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr); //range is 1024 by default
	
	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/physbeam.vmt", 5.0f ); //physbeam default
	if ( m_pBeam[m_iBeams] == NULL )
		 return;


	m_pBeam[m_iBeams]->PointEntInit( tr.endpos, this );
	m_pBeam[m_iBeams]->SetEndAttachment( side < 0 ? 2 : 1 );
	m_pBeam[m_iBeams]->SetColor( 255, 255, 0 ); // 0 255 0 by default
	m_pBeam[m_iBeams]->SetBrightness( 255 );
	m_pBeam[m_iBeams]->SetNoise( 0.8f ); //1.2f by default
	m_pBeam[m_iBeams]->AddSpawnFlags( SF_BEAM_TEMPORARY );
	m_iBeams++;





	for( m_iNoise = 0 ; m_iNoise < 5 ; m_iNoise++ ) 
	{
		Vector vecDest;

			// Random unit vector
		vecDest.x = random->RandomFloat( -1, 1 );
		vecDest.y = random->RandomFloat( -1, 1 );
		vecDest.z = random->RandomFloat( -1, 1 );

		// Range for noise beams is 128 world units
		vecDest = tr.endpos + vecDest * 256;

		//spawn immolator-like radius beam doo-dads
		m_hNoise[m_iNoise] = CBeam::BeamCreate(  "sprites/physbeam.vmt", 3.00f ); //physbeam default
		m_hNoise[m_iNoise]->PointsInit( tr.endpos, vecDest);
		m_hNoise[m_iNoise]->AddSpawnFlags( SF_BEAM_TEMPORARY );
		m_hNoise[m_iNoise]->SetScrollRate( 60 );
		m_hNoise[m_iNoise]->SetBrightness( 255 );
		m_hNoise[m_iNoise]->SetColor( 255, 255, 0 ); //0 255 0
		m_hNoise[m_iNoise]->SetNoise( 15 );
		m_hNoise[m_iNoise]->SetFadeLength( 0.75 );
		UTIL_DecalTrace( &tr, "FadingScorch" );
	}

	//m_iNoise++;


	trace_t noisetr;




	pEntity = tr.m_pEnt;

	if ( pEntity != NULL && m_takedamage )
	{
		CTakeDamageInfo info( this, this, 5, DMG_DISSOLVE ); //dmg_dissolve and dmg_burn by default
		Ignite(100);
		CalculateMeleeDamageForce( &info, vecAim, tr.endpos );
		RadiusDamage( CTakeDamageInfo( this, this, 5, DMG_BURN ), tr.endpos, 256,  CLASS_NONE, NULL ); //changed from 256 to 128 to correspond with noisebeams
		pEntity->DispatchTraceAttack( info, vecAim, &tr );
	}

}

void CNPC_FinalBoss::ClearBeams( )
{
	for (int i = 0; i < 1; i++)
	{
		if (m_pBeam[i])
		{
			UTIL_Remove( m_pBeam[i] );
			m_pBeam[i] = NULL;
		}
	}

	for (int i =0; i < 5; i++)
	{
		if (m_hNoise[i])
		{
			UTIL_Remove( m_hNoise[i] );
			m_hNoise[i] = NULL;
		}
		if (m_iNoise > 0)
		{
			UTIL_Remove( m_hNoise[i] );
			m_iNoise = NULL;
		}
	}

	m_iBeams = 0;
	m_iNoise = 0;
	m_nSkin = 0;
}

void CNPC_FinalBoss::Shove( void ) // Doesn't work for some reason // It sure works now! -Dan
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

	if ( flDist < FINALBOSS_MELEE1_RANGE && flDot >= FINALBOSS_MELEE1_CONE )
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

int CNPC_FinalBoss::SelectSchedule( void )
{
	ClearBeams();

	return BaseClass::SelectSchedule();
}

void CNPC_FinalBoss::StartTask( const Task_t *pTask )
{
	ClearBeams();
	switch ( pTask->iTask )
	{
	
		TaskComplete();

		break;

	case TASK_FBOSS_RANGE_ATTACK1:
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

void CNPC_FinalBoss::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_FBOSS_RANGE_ATTACK1:
		{
			Vector flEnemyLKP = GetEnemyLKP();
			GetMotor()->SetIdealYawToTargetAndUpdate( flEnemyLKP );

			if ( IsActivityFinished() )
			{
				TaskComplete();
				return;
			}

			if (HasCondition( COND_ENEMY_OCCLUDED )) //Enemy hiding from us? Don't be retarded, get a line of sight
			{
				ClearBeams();
				TaskComplete();
				break;
			}
			if (HasCondition( COND_ENEMY_DEAD )) //Killed our enemy? No reason to keep immolating
			{
				ClearBeams();
				TaskComplete();
				break;
			}

		}
		return;
	}

	BaseClass::RunTask( pTask );
}


#define TRANSLATE_SCHEDULE( type, in, out ) { if ( type == in ) return out; }

Activity CNPC_FinalBoss::NPC_TranslateActivity( Activity baseAct )
{
	if( baseAct == ACT_RUN )
	{
		return ( Activity )ACT_WALK;
	}
	
	if ( baseAct == ACT_IDLE && m_NPCState != NPC_STATE_IDLE )
	{
		return ( Activity ) ACT_IDLE_ANGRY;
	}

	return baseAct;
}

int CNPC_FinalBoss::TranslateSchedule( int type ) 
{
	switch( type )
	{
	case SCHED_RANGE_ATTACK1:
		return SCHED_FBOSS_RANGE_ATTACK1;
		break;
	}

	return BaseClass::TranslateSchedule( type );
}


int CNPC_FinalBoss::OnTakeDamage_Alive( const CTakeDamageInfo &info )
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


int CNPC_FinalBoss::MeleeAttack1Conditions( float flDot, float flDist )
{
	if ( flDist > FINALBOSS_MELEE1_RANGE )
		return COND_TOO_FAR_TO_ATTACK;
	
	if ( flDot < FINALBOSS_MELEE1_CONE )
		return COND_NOT_FACING_ATTACK;

	return COND_CAN_MELEE_ATTACK1;
}

int CNPC_FinalBoss::RangeAttack1Conditions( float flDot, float flDist )
{
	if ( flDist > FINALBOSS_RANGE1_RANGE )
		return COND_TOO_FAR_TO_ATTACK;
	
	if ( flDot < FINALBOSS_RANGE1_CONE )
		return COND_NOT_FACING_ATTACK;

	if ( m_flLastRangeTime > gpGlobals->curtime )
		return COND_TOO_FAR_TO_ATTACK;

	return COND_CAN_RANGE_ATTACK1;
}

void CNPC_FinalBoss::DoMuzzleFlash( void )
{
	BaseClass::DoMuzzleFlash();
	
	CEffectData data;

	data.m_nAttachmentIndex = LookupAttachment( "Muzzle" );
	data.m_nEntIndex = entindex();
	DispatchEffect( "StriderMuzzleFlash", data );
}


void CNPC_FinalBoss::FireRangeWeapon( void )
{
	if ( ( GetEnemy() != NULL ) && ( GetEnemy()->Classify() == CLASS_BULLSEYE ) )
	{
		GetEnemy()->TakeDamage( CTakeDamageInfo( this, this, 1.0f, DMG_GENERIC ) );
	}

	Vector vecSrc, vecAiming;
	Vector vecShootOrigin;
	Vector vecShootDir = GetShootEnemyDir( vecShootOrigin );

	GetVectors( &vecAiming, NULL, NULL );
	vecSrc = WorldSpaceCenter() + vecAiming * 64;
	
	Vector	impactPoint	= vecSrc + ( vecShootDir * MAX_TRACE_LENGTH ); //vecsrc + VecAiming by default

	vecShootOrigin = GetAbsOrigin() + Vector( 0, 0, 55 );
	QAngle angDir;
	
	QAngle	angShootDir;
	DoMuzzleFlash();
	GetAttachment( LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir );

	trace_t	tr;
	AI_TraceLine( vecSrc, impactPoint, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
// Just using the gunship tracers for a placeholder unless a better effect can be found. Maybe use the strider cannon's tracer or something.
	UTIL_Tracer( tr.startpos, tr.endpos, 0, TRACER_DONT_USE_ATTACHMENT, 6000 + random->RandomFloat( 0, 2000 ), true, "GunshipTracer" );
	UTIL_Tracer( tr.startpos, tr.endpos, 0, TRACER_DONT_USE_ATTACHMENT, 6000 + random->RandomFloat( 0, 3000 ), true, "GunshipTracer" );
	UTIL_Tracer( tr.startpos, tr.endpos, 0, TRACER_DONT_USE_ATTACHMENT, 6000 + random->RandomFloat( 0, 4000 ), true, "GunshipTracer" );

	CreateConcussiveBlast( tr.endpos, tr.plane.normal, this, 5.0 ); //5 times stronger
}

#define	DEBUG_AIMING 0

bool CNPC_FinalBoss::AimGunAt( CBaseEntity *pEntity, float flInterval )
{
	if ( !pEntity )
		return true;

	matrix3x4_t gunMatrix;
	GetAttachment( m_MuzzleAttachment, gunMatrix );

	Vector localEnemyPosition;
	VectorITransform( pEntity->GetAbsOrigin(), gunMatrix, localEnemyPosition );
	
	QAngle localEnemyAngles;
	VectorAngles( localEnemyPosition, localEnemyAngles );
	
	localEnemyAngles.x = UTIL_AngleDiff( localEnemyAngles.x, 0 );	
	localEnemyAngles.y = UTIL_AngleDiff( localEnemyAngles.y, 0 );

	float targetYaw = m_aimYaw + localEnemyAngles.y;
	float targetPitch = m_aimPitch + localEnemyAngles.x;
	
	QAngle unitAngles = localEnemyAngles;
	float angleDiff = sqrt( localEnemyAngles.y * localEnemyAngles.y + localEnemyAngles.x * localEnemyAngles.x );
	const float aimSpeed = 1;

	float yawSpeed = fabsf(aimSpeed*flInterval*localEnemyAngles.y);
	float pitchSpeed = fabsf(aimSpeed*flInterval*localEnemyAngles.x);

	yawSpeed = max(yawSpeed,15);
	pitchSpeed = max(pitchSpeed,15);

	m_aimYaw = UTIL_Approach( targetYaw, m_aimYaw, yawSpeed );
	m_aimPitch = UTIL_Approach( targetPitch, m_aimPitch, pitchSpeed );

	SetPoseParameter( m_YawControl, m_aimYaw );
	SetPoseParameter( m_PitchControl, m_aimPitch );

	m_aimPitch = GetPoseParameter( m_PitchControl );
	m_aimYaw = GetPoseParameter( m_YawControl );

	if ( angleDiff < 1 )
		return true;

	return false;
}



float CNPC_FinalBoss::MaxYawSpeed( void ) 
{ 	
	if ( GetActivity() == ACT_RANGE_ATTACK1 )
	{
		return 1.0f;
	}

	return 60.0f;
}

void CNPC_FinalBoss::BuildScheduleTestBits( void )
{
	SetCustomInterruptCondition( COND_FINALBOSS_CLOBBERED );
}

AI_BEGIN_CUSTOM_NPC( npc_finalboss, CNPC_FinalBoss )

	DECLARE_TASK( TASK_FBOSS_RANGE_ATTACK1 )
	DECLARE_TASK( TASK_FINALBOSS_SET_BALANCE )
	
	DECLARE_CONDITION( COND_FINALBOSS_CLOBBERED )

	DECLARE_ACTIVITY( ACT_FBOSS_IDLE_TO_ANGRY )
	DECLARE_ACTIVITY( ACT_FINALBOSS_CLOBBERED )
	DECLARE_ACTIVITY( ACT_FINALBOSS_TOPPLE )
	DECLARE_ACTIVITY( ACT_FINALBOSS_GETUP )
	DECLARE_ACTIVITY( ACT_FINALBOSS_HELPLESS )

	DEFINE_SCHEDULE
	(
		SCHED_FBOSS_RANGE_ATTACK1,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_FACE_ENEMY				0"
		"		TASK_ANNOUNCE_ATTACK		1"
		"		TASK_FBOSS_RANGE_ATTACK1	0"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_NO_PRIMARY_AMMO"
	)

AI_END_CUSTOM_NPC()



