//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		NEMESIS
//				
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "game.h"
#include "ai_default.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "ai_route.h"
#include "ai_hint.h"
#include "ai_navigator.h"
#include "ai_senses.h"
#include "npcevent.h"
#include "animation.h"
#include "Nemator.h"
#include "gib.h"
#include "soundent.h"
#include "ndebugoverlay.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "hl1_grenade_spit.h"
#include "util.h"
#include "shake.h"
#include "movevars_shared.h"
#include "decals.h"
#include "hl2_shareddefs.h"
#include "hl2_gamerules.h"
#include "ammodef.h"
#include "te_effect_dispatch.h"
#include "te_particlesystem.h"

#include "player.h"
#include "ai_network.h"
#include "ai_navigator.h"
#include "ai_motor.h"
#include "ai_default.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "ai_node.h"
#include "ai_memory.h"
#include "ai_senses.h"
#include "bitstring.h"
#include "EntityFlame.h"
#include "hl2_shareddefs.h"
#include "npcevent.h"
#include "activitylist.h"
#include "entitylist.h"
#include "gib.h"
#include "soundenvelope.h"
#include "ndebugoverlay.h"
#include "rope.h"
#include "rope_shared.h"
#include "igamesystem.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "props.h"
#include "hl2_gamerules.h"
#include "weapon_physcannon.h"
#include "ammodef.h"
#include "vehicle_base.h"
#include	"grenade_homer.h"

#define		NEMATOR_SPRINT_DIST	256

extern ConVar sk_tank_shell_velocity;


#define ZOMBIE_BURN_TIME		10 // If ignited, burn for this many seconds
#define ZOMBIE_BURN_TIME_NOISE	2  // Give or take this many seconds.

ConVar sk_nemator_health ( "sk_nemator_health", "500" );
ConVar sk_nemator_dmg_bite ( "sk_nemator_dmg_bite", "25" );
ConVar sk_nemator_dmg_whip ( "sk_nemator_dmg_whip", "35" );
ConVar sk_nemator_dmg_spit ( "sk_nemator_dmg_spit", "15" );

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_NEMATOR_HURTHOP = LAST_SHARED_SCHEDULE,
	SCHED_NEMATOR_SEECRAB,
	SCHED_NEMATOR_EAT,
	SCHED_NEMATOR_SNIFF_AND_EAT,
	SCHED_NEMATOR_WALLOW,
	SCHED_NEMATOR_CHASE_ENEMY,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum 
{
	TASK_NEMATOR_HOPTURN = LAST_SHARED_TASK,
	TASK_NEMATOR_EAT,
};

//-----------------------------------------------------------------------------
// Nemator Conditions
//-----------------------------------------------------------------------------
enum
{
	COND_NEMATOR_SMELL_FOOD	= LAST_SHARED_CONDITION + 1,
};


//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		NEMATOR_AE_SPIT		( 1 )
#define		NEMATOR_AE_BITE		( 2 )
#define		NEMATOR_AE_BLINK		( 3 )
#define		NEMATOR_AE_TAILWHIP	( 4 )
#define		NEMATOR_AE_HOP		( 5 )
#define		NEMATOR_AE_THROW		( 6 )
#define		NEMATOR_AE_ROCKET		( 7 )

LINK_ENTITY_TO_CLASS( monster_nemator, CNPC_Nemator );

int ACT_NEMATOR_EXCITED;
int ACT_NEMATOR_EAT;
int ACT_NEMATOR_DETECT_SCENT;
int ACT_NEMATOR_INSPECT_FLOOR;

//=========================================================
// Nemator's spit projectile
//=========================================================
class CNematorSpit : public CBaseEntity
{
	DECLARE_CLASS( CNematorSpit, CBaseEntity );
public:
	void Spawn( void );
	void Precache( void );

	static void Shoot( CBaseEntity *pOwner, Vector vecStart, Vector vecVelocity );
	void Touch( CBaseEntity *pOther );
	void Animate( void );

	int m_nNematorSpitSprite;

	DECLARE_DATADESC();

	void SetSprite( CBaseEntity *pSprite )
	{
		m_hSprite = pSprite;	
	}

	CBaseEntity *GetSprite( void )
	{
		return m_hSprite.Get();
	}

private:
	EHANDLE m_hSprite;


};

LINK_ENTITY_TO_CLASS( squidspit, CNematorSpit );

BEGIN_DATADESC( CNematorSpit )
	DEFINE_FIELD( m_nNematorSpitSprite, FIELD_INTEGER ),
	DEFINE_FIELD( m_hSprite, FIELD_EHANDLE ),
END_DATADESC()


void CNematorSpit::Precache( void )
{
	m_nNematorSpitSprite = PrecacheModel("sprites/blueflare1.vmt");// client side spittle.

	PrecacheScriptSound( "NPC_BigMomma.SpitTouch1" );
	PrecacheScriptSound( "NPC_BigMomma.SpitHit1" );
	PrecacheScriptSound( "NPC_BigMomma.SpitHit2" );

	PrecacheScriptSound( "Zombie.AttackHit" );
	PrecacheScriptSound( "Zombie.AttackMiss" );
}

void CNematorSpit:: Spawn( void )
{
	Precache();

	SetMoveType ( MOVETYPE_FLY );
	SetClassname( "squidspit" );
	
	SetSolid( SOLID_BBOX );

	m_nRenderMode = kRenderTransAlpha;
	SetRenderColorA( 255 );
	SetModel( "" );

	SetSprite( CSprite::SpriteCreate( "sprites/blueflare1.vmt", GetAbsOrigin(), true ) );
	SetRenderColor( 150, 0, 0, 255 );
	
	UTIL_SetSize( this, Vector( 0, 0, 0), Vector(0, 0, 0) );

	SetCollisionGroup( HL2COLLISION_GROUP_SPIT );
}

void CNematorSpit::Shoot( CBaseEntity *pOwner, Vector vecStart, Vector vecVelocity )
{
	CNematorSpit *pSpit = CREATE_ENTITY( CNematorSpit, "squidspit" );
	pSpit->Spawn();
	
	UTIL_SetOrigin( pSpit, vecStart );
	pSpit->SetAbsVelocity( vecVelocity );
	pSpit->SetOwnerEntity( pOwner );

	CSprite *pSprite = (CSprite*)pSpit->GetSprite();

	if ( pSprite )
	{
		pSprite->SetAttachment( pSpit, 0 );
		pSprite->SetOwnerEntity( pSpit );

		pSprite->SetScale( 0.75 );
		pSprite->SetTransparency( pSpit->m_nRenderMode, pSpit->m_clrRender->r, pSpit->m_clrRender->g, pSpit->m_clrRender->b, pSpit->m_clrRender->a, pSpit->m_nRenderFX );
	}


	CPVSFilter filter( vecStart );

	VectorNormalize( vecVelocity );
	te->SpriteSpray( filter, 0.0, &vecStart , &vecVelocity, pSpit->m_nNematorSpitSprite, 210, 25, 15 );
}

void CNematorSpit::Touch ( CBaseEntity *pOther )
{
	trace_t tr;
	int		iPitch;

	if ( pOther->GetSolidFlags() & FSOLID_TRIGGER )
		 return;

	if ( pOther->GetCollisionGroup() == HL2COLLISION_GROUP_SPIT)
	{
		return;
	}

	// splat sound
	iPitch = random->RandomFloat( 90, 110 );

	EmitSound( "NPC_BigMomma.SpitTouch1" );

	switch ( random->RandomInt( 0, 1 ) )
	{
	case 0:
		EmitSound( "NPC_BigMomma.SpitHit1" );
		break;
	case 1:
		EmitSound( "NPC_BigMomma.SpitHit2" );
		break;
	}

	if ( !pOther->m_takedamage )
	{
		// make a splat on the wall
		UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + GetAbsVelocity() * 10, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );
		UTIL_DecalTrace(&tr, "Blood" ); //BeerSplash

		// make some flecks
		CPVSFilter filter( tr.endpos );

		te->SpriteSpray( filter, 0.0,	&tr.endpos, &tr.plane.normal, m_nNematorSpitSprite, 30, 8, 5 );

	}
	else
	{
		CTakeDamageInfo info( this, this, sk_nemator_dmg_spit.GetFloat(), DMG_BULLET );
		CalculateBulletDamageForce( &info, GetAmmoDef()->Index("9mmRound"), GetAbsVelocity(), GetAbsOrigin() );
		pOther->TakeDamage( info );
	}

	UTIL_Remove( m_hSprite );
	UTIL_Remove( this );
}


BEGIN_DATADESC( CNPC_Nemator )
	DEFINE_FIELD( m_fCanThreatDisplay, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flLastHurtTime, FIELD_TIME ),
	DEFINE_FIELD( m_flNextSpitTime, FIELD_TIME ),
	DEFINE_FIELD( m_flBurnDamage, FIELD_FLOAT ),
	DEFINE_FIELD( m_flBurnDamageResetTime, FIELD_TIME ),

	DEFINE_FIELD( m_flHungryTime, FIELD_TIME ),
END_DATADESC()

//=========================================================
// Spawn
//=========================================================
void CNPC_Nemator::Spawn()
{
	Precache( );

	SetModel( "models/nemator.mdl");
	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();
	SetModelScale( 1.07 );

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	m_bloodColor		= BLOOD_COLOR_RED;
	
	SetRenderColor( 255, 255, 255, 255 );
	
	m_iHealth			= sk_nemator_health.GetFloat();
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_NPCState			= NPC_STATE_NONE;
	
	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_INNATE_RANGE_ATTACK1 | bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_INNATE_MELEE_ATTACK2 );
	CapabilitiesAdd( bits_CAP_MOVE_JUMP | bits_CAP_MOVE_CLIMB );
	CapabilitiesAdd( bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS | bits_CAP_DOORS_GROUP );
	
	m_fCanThreatDisplay	= TRUE;
	m_flNextSpitTime = gpGlobals->curtime;

	NPCInit();

	m_flDistTooFar		= 784;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CNPC_Nemator::Precache()
{

	m_iAmmoType = GetAmmoDef()->Index("Buckshot");

	BaseClass::Precache();
	
	PrecacheModel("models/nemator.mdl");
	
	PrecacheModel("sprites/blueflare1.vmt");// spit projectile.

	PrecacheScriptSound( "Nemator.Idle" );
	PrecacheScriptSound( "Nemator.Pain" );
	PrecacheScriptSound( "Nemator.Alert" );
	PrecacheScriptSound( "Nemator.Die" );
	PrecacheScriptSound( "Nemator.Attack" );
	PrecacheScriptSound( "Nemator.Bite" );
	PrecacheScriptSound( "Nemator.Growl" );
	PrecacheScriptSound( "Nemator.Shoot" );
}


int CNPC_Nemator::TranslateSchedule( int scheduleType )
{	
	switch	( scheduleType )
	{
		case SCHED_CHASE_ENEMY:
			return SCHED_NEMATOR_CHASE_ENEMY;
			break;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}

//-----------------------------------------------------------------------------
// Purpose: Indicates this monster's place in the relationship table.
// Output : 
//-----------------------------------------------------------------------------
Class_T	CNPC_Nemator::Classify( void )
{
	return CLASS_COMBINE; 
}

bool CNPC_Nemator::IsJumpLegal(const Vector &startPos, const Vector &apex, const Vector &endPos) const
{
	const float MAX_JUMP_RISE		= 400.0f;
	const float MAX_JUMP_DISTANCE	= 800.0f;
	const float MAX_JUMP_DROP		= 2048.0f;

	if ( BaseClass::IsJumpLegal( startPos, apex, endPos, MAX_JUMP_RISE, MAX_JUMP_DROP, MAX_JUMP_DISTANCE ) )
	{
		// Hang onto the jump distance. The AI is going to want it.
		m_flJumpDist = (startPos - endPos).Length();

		return true;
	}
	return false;
}

void CNPC_Nemator::ShootRPG(void)
{
	Vector vecShootOrigin;

	//	UTIL_MakeVectors(pev->angles); Big fucking mess of vectors and angles here over trying to get the goddamn fucking rocket to shoot down or up depending on enemy's position fuck fuck fuck
	vecShootOrigin = GetAbsOrigin() + Vector(0, 0, 100);
	Vector vecShootDir = GetShootEnemyDir(vecShootOrigin);
	QAngle	angShootDir;
	Vector vecSrc, vecAim;
	vecAim = GetShootEnemyDir(vecSrc);
	QAngle	vecAngles;
	VectorAngles(vecShootDir, vecAngles);

	VectorNormalize(vecAim);


	CEffectData data;
	data.m_nAttachmentIndex = LookupAttachment("RocketMuzzle");
	data.m_nEntIndex = entindex();
	GetAttachment("RocketMuzzle", vecShootOrigin);

	CMissile *pRocket = (CMissile *)CBaseEntity::Create("rpg_missile", vecShootOrigin, vecAngles, this);
	//pRocket->SetAbsVelocity( vecAim * sk_tank_shell_velocity.GetFloat() );
	pRocket->SetAbsVelocity(vecShootDir * sk_tank_shell_velocity.GetFloat());
	pRocket->DumbFire();
	pRocket->SetNextThink(gpGlobals->curtime + 0.1f);

	SetAim(vecShootDir);
}

//=========================================================
// IdleSound 
//=========================================================
#define NEMATOR_ATTN_IDLE	(float)1.5
void CNPC_Nemator::IdleSound( void )
{
	CPASAttenuationFilter filter( this, NEMATOR_ATTN_IDLE );
	EmitSound( filter, entindex(), "Nemator.Idle" );	
}

//=========================================================
// PainSound 
//=========================================================
void CNPC_Nemator::PainSound( const CTakeDamageInfo &info )
{
	CPASAttenuationFilter filter( this );
	EmitSound( filter, entindex(), "Nemator.Pain" );	
}

//=========================================================
// AlertSound
//=========================================================
void CNPC_Nemator::AlertSound( void )
{
	CPASAttenuationFilter filter( this );
	EmitSound( filter, entindex(), "Nemator.Alert" );	
}

//=========================================================
// DeathSound
//=========================================================
void CNPC_Nemator::DeathSound( const CTakeDamageInfo &info )
{
	CPASAttenuationFilter filter( this );
	EmitSound( filter, entindex(), "Nemator.Die" );	
}

//=========================================================
// AttackSound
//=========================================================
void CNPC_Nemator::AttackSound( void )
{
	CPASAttenuationFilter filter( this );
	EmitSound( filter, entindex(), "Nemator.Attack" );	
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
float CNPC_Nemator::MaxYawSpeed( void )
{
	float flYS = 0;

	switch ( GetActivity() )
	{
	case	ACT_WALK:			flYS = 90;	break;
	case	ACT_RUN:			flYS = 90;	break;
	case	ACT_IDLE:			flYS = 90;	break;
	case	ACT_RANGE_ATTACK1:	flYS = 90;	break;
	default:
		flYS = 90;
		break;
	}

	return flYS;
}

bool CNPC_Nemator::ShouldIgnite( const CTakeDamageInfo &info )
{
 	if ( IsOnFire() )
	{
		// Already burning!
		return false;
	}

	if ( info.GetDamageType() & DMG_BURN )
	{
		//
		// If we take more than ten percent of our health in burn damage within a five
		// second interval, we should catch on fire.
		//
		m_flBurnDamage += info.GetDamage();
		m_flBurnDamageResetTime = gpGlobals->curtime + 5;

		if ( m_flBurnDamage >= m_iMaxHealth * 0.1 )
		{
			Ignite(100.0f);
			return true;
		}
	}

	return false;
}

void CNPC_Nemator::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();

	if ( ( m_flBurnDamageResetTime ) && ( gpGlobals->curtime >= m_flBurnDamageResetTime ) )
	{
		m_flBurnDamage = 0;
	}
}

void CNPC_Nemator::DoGatMuzzleFlash( void )
{
	BaseClass::DoMuzzleFlash();
	
	CEffectData datag;

	datag.m_nAttachmentIndex = LookupAttachment( "Muzzle" );
	datag.m_nEntIndex = entindex();
	DispatchEffect( "AirboatMuzzleFlash", datag );
}

void CNPC_Nemator::MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType )
{
	float flTracerDist;
	Vector vecDir;
	Vector vecEndPos;
	Vector vecShootOrigin;
	Vector angShootDir;
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



//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CNPC_Nemator::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
		case NEMATOR_AE_SPIT:
		{
			if ( GetEnemy() )
			{
	Vector vecShootOrigin;

//	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = GetAbsOrigin() + Vector( 0, 0, 55 );
	Vector vecShootDir = GetShootEnemyDir( vecShootOrigin );
	QAngle angDir;
	QAngle	angShootDir;
			EmitSound( "Hassault.Minigun" );	
	CEffectData data;
	data.m_nAttachmentIndex = LookupAttachment( "Muzzle" );
	data.m_nEntIndex = entindex();
	GetAttachment("Muzzle", vecShootOrigin, angShootDir);
	DoGatMuzzleFlash();

//	GetAttachment( LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir );
	//VectorAngles( vecShootDir, angDir );
	//SetBlending( 0, angDir.x );
	//pev->effects = EF_MUZZLEFLASH;
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_15DEGREES, 1024, m_iAmmoType  ); //originally 2 bullets at a time w/ 10 degree spread
//	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "hass/minigun_shoot.wav", 1, ATTN_NORM, 0, 100 );
			}
		}
		break;

		case NEMATOR_AE_BITE:
		{
		// SOUND HERE!
			CPASAttenuationFilter filter( this );
			//CBaseEntity *pHurt = CheckTraceHullAttack( 70, Vector(-16,-16,-16), Vector(16,16,16), sk_nemator_dmg_bite.GetFloat(), DMG_SLASH );
			Vector vecMins = GetHullMins();
			Vector vecMaxs = GetHullMaxs();
			vecMins.z = vecMins.x;
			vecMaxs.z = vecMaxs.x;

			CBaseEntity *pHurt = CheckTraceHullAttack( 70, vecMins, vecMaxs, sk_nemator_dmg_bite.GetFloat(), DMG_SLASH );
			if ( pHurt )
			{
				Vector forward, up;
				AngleVectors( GetAbsAngles(), &forward, NULL, &up );
				pHurt->SetAbsVelocity( pHurt->GetAbsVelocity() - (forward * 100) );
				pHurt->SetAbsVelocity( pHurt->GetAbsVelocity() + (up * 100) );
				pHurt->SetGroundEntity( NULL );
				EmitSound( filter, entindex(), "Nemator.Bite" );
			}

			else // Play a random attack miss sound
			EmitSound( filter, entindex(), "Zombie.AttackMiss" );
		}
		break;

		case NEMATOR_AE_TAILWHIP:
		{
			Vector vecMins = GetHullMins();
			Vector vecMaxs = GetHullMaxs();
			vecMins.z = vecMins.x;
			vecMaxs.z = vecMaxs.x;
			CPASAttenuationFilter filter( this );
			EmitSound( filter, entindex(), "Nemator.Bite" );	
			//CBaseEntity *pHurt = CheckTraceHullAttack( 70, Vector(-16,-16,-16), Vector(16,16,16), sk_nemator_dmg_whip.GetFloat(), DMG_SLASH | DMG_ALWAYSGIB );
				CBaseEntity *pHurt = CheckTraceHullAttack( 70, vecMins, vecMaxs, sk_nemator_dmg_whip.GetFloat(), DMG_SLASH | DMG_ALWAYSGIB );
			if ( pHurt ) 
			{
				Vector right, up;
				AngleVectors( GetAbsAngles(), NULL, &right, &up );

				if ( pHurt->GetFlags() & ( FL_NPC | FL_CLIENT ) )
					 pHurt->ViewPunch( QAngle( 20, 0, -20 ) );
			
				pHurt->SetAbsVelocity( pHurt->GetAbsVelocity() + (right * 500) );
				pHurt->SetAbsVelocity( pHurt->GetAbsVelocity() + (up * 600) );
			}

		}
		break;

		case NEMATOR_AE_BLINK:
		{
			// close eye. 
			m_nSkin = 1;
		}
		break;

		case NEMATOR_AE_HOP:
		{
			float flGravity = sv_gravity.GetFloat();

			// throw the squid up into the air on this frame.
			if ( GetFlags() & FL_ONGROUND )
			{
				SetGroundEntity( NULL );
			}

			// jump into air for 0.8 (24/30) seconds
			Vector vecVel = GetAbsVelocity();
			vecVel.z += ( 0.625 * flGravity ) * 0.5;
			SetAbsVelocity( vecVel );
		}
		break;

		case NEMATOR_AE_THROW:
			{
				// squid throws its prey IF the prey is a client. 
				CBaseEntity *pHurt = CheckTraceHullAttack( 70, Vector(-16,-16,-16), Vector(16,16,16), 0, 0 );


				if ( pHurt )
				{
					// croonchy bite sound
					CPASAttenuationFilter filter( this );
					EmitSound( filter, entindex(), "Nemator.Bite" );	

					// screeshake transforms the viewmodel as well as the viewangle. No problems with seeing the ends of the viewmodels.
					UTIL_ScreenShake( pHurt->GetAbsOrigin(), 25.0, 1.5, 0.7, 2, SHAKE_START );

					if ( pHurt->IsPlayer() )
					{
						Vector forward, up;
						AngleVectors( GetAbsAngles(), &forward, NULL, &up );
				
						pHurt->SetAbsVelocity( pHurt->GetAbsVelocity() + forward * 300 + up * 300 );
					}
				}
			}
		break;

		default:
			BaseClass::HandleAnimEvent( pEvent );
	}
}

int CNPC_Nemator::RangeAttack1Conditions( float flDot, float flDist )
{
	if ( IsMoving() && flDist >= 512 )
	{
		// squid will far too far behind if he stops running to spit at this distance from the enemy.
		return ( COND_NONE );
	}

	if ( flDist > 85 && flDist <= 784 && flDot >= 0.5 && gpGlobals->curtime >= m_flNextSpitTime )
	{
		if ( GetEnemy() != NULL )
		{
			if ( fabs( GetAbsOrigin().z - GetEnemy()->GetAbsOrigin().z ) > 256 )
			{
				// don't try to spit at someone up really high or down really low.
				return( COND_NONE );
			}
		}

		if ( IsMoving() )
		{
			// don't spit again for a long time, resume chasing enemy.
			m_flNextSpitTime = gpGlobals->curtime + RandomInt(1, 3);
		}
		else
		{
			// not moving, so spit again pretty soon.
			m_flNextSpitTime = gpGlobals->curtime + 0.5;
		}
		return( COND_CAN_RANGE_ATTACK1 );
	}

	return( COND_NONE );
}

//=========================================================
// MeleeAttack2Conditions - nemator is a big guy, so has a longer
// melee range than most monsters. This is the tailwhip attack
//=========================================================
int CNPC_Nemator::MeleeAttack1Conditions( float flDot, float flDist )
{
	if ( GetEnemy()->m_iHealth <= sk_nemator_dmg_whip.GetFloat() && flDist <= 85 && flDot >= 0.7 )
	{
		return ( COND_CAN_MELEE_ATTACK1 );
	}
	
	return( COND_NONE );
}

//=========================================================
// MeleeAttack2Conditions - nemator is a big guy, so has a longer
// melee range than most monsters. This is the bite attack.
// this attack will not be performed if the tailwhip attack
// is valid.
//=========================================================
int CNPC_Nemator::MeleeAttack2Conditions( float flDot, float flDist )
{
	if ( flDist <= 85 && flDot >= 0.7 && !HasCondition( COND_CAN_MELEE_ATTACK1 ) )		// The player & gonome can be as much as their bboxes 
		 return ( COND_CAN_MELEE_ATTACK2 );
	
	return( COND_NONE );
}


bool CNPC_Nemator::FValidateHintType ( CAI_Hint *pHint )
{
	if ( pHint->HintType() == HINT_HL1_WORLD_HUMAN_BLOOD )
		 return true;

	Msg ( "Couldn't validate hint type" );

	return false;
}

void CNPC_Nemator::RemoveIgnoredConditions( void )
{
	if ( m_flHungryTime > gpGlobals->curtime )
		 ClearCondition( COND_NEMATOR_SMELL_FOOD );

	if ( gpGlobals->curtime - m_flLastHurtTime <= 20 )
	{
		// haven't been hurt in 20 seconds, so let the squid care about stink. 
		ClearCondition( COND_SMELL );
	}

	if ( GetEnemy() != NULL )
	{
		// ( Unless after a tasty headcrab, yumm ^_^ )
		if ( FClassnameIs( GetEnemy(), "npc_citizen" ) )
			 ClearCondition( COND_SMELL );
	}

	if ( GetActivity() == ACT_MELEE_ATTACK1 )
	{
		ClearCondition( COND_LIGHT_DAMAGE );
		ClearCondition( COND_HEAVY_DAMAGE );
	}

	if ( GetActivity() == ACT_MELEE_ATTACK2 )
	{
		ClearCondition( COND_LIGHT_DAMAGE );
		ClearCondition( COND_HEAVY_DAMAGE );
	}
}

Disposition_t CNPC_Nemator::IRelationType( CBaseEntity *pTarget )
{
	if ( gpGlobals->curtime - m_flLastHurtTime < 5 && FClassnameIs ( pTarget, "npc_citizen" ) )
	{
		// if squid has been hurt in the last 5 seconds, and is getting relationship for a headcrab, 
		// tell squid to disregard crab. 
		return D_NU;
	}

	return BaseClass::IRelationType ( pTarget );
}



static float DamageForce( const Vector &size, float damage )
{ 
	float force = damage * ((32 * 32 * 72.0) / (size.x * size.y * size.z)) * 5;
	
	if ( force > 1000.0) 
	{
		force = 1000.0;
	}

	return force;
}

//=========================================================
// TakeDamage - overridden for nemator so we can keep track
// of how much time has passed since it was last injured
//=========================================================
int CNPC_Nemator::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
{
	CTakeDamageInfo info = inputInfo;

	if( inputInfo.GetDamageType() & DMG_BURN )
	{
		// If a zombie is on fire it only takes damage from the fire that's attached to it. (DMG_DIRECT)
		// This is to stop zombies from burning to death 10x faster when they're standing around
		// 10 fire entities.
		if( IsOnFire() && !(inputInfo.GetDamageType() & DMG_DIRECT) )
		{
			return 0;
		}
		
		Scorch( 8, 50 );
		Ignite( 100.0f ); //THE FUCK ISN'T THIS WORKING FOR
	}

	if ( ShouldIgnite( info ) )
	{
		Ignite( 100.0f );
	}

	if ( !FClassnameIs ( inputInfo.GetAttacker(), "npc_citizen" ) )
	{
		// don't forget about headcrabs if it was a headcrab that hurt the squid.
		m_flLastHurtTime = gpGlobals->curtime;
	}

	if ( info.GetDamageType() == DMG_BULLET )
	{
		Vector vecDir = GetAbsOrigin() - info.GetInflictor()->WorldSpaceCenter();
		VectorNormalize( vecDir );
		float flForce = DamageForce( WorldAlignSize(), info.GetDamage() );
		SetAbsVelocity( GetAbsVelocity() + vecDir * flForce );
		info.ScaleDamage( 0.25f );
	}

	return BaseClass::OnTakeDamage_Alive ( inputInfo );
}


//=========================================================
// GetSoundInterests - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CNPC_Nemator::GetSoundInterests ( void )
{
	return	SOUND_WORLD	|
			SOUND_COMBAT	|
		    SOUND_CARCASS	|
			SOUND_MEAT		|
			SOUND_GARBAGE	|
			SOUND_PLAYER;
}

//=========================================================
// OnListened - monsters dig through the active sound list for
// any sounds that may interest them. (smells, too!)
//=========================================================
void CNPC_Nemator::OnListened( void )
{
	AISoundIter_t iter;
	
	CSound *pCurrentSound;

	static int conditionsToClear[] = 
	{
		COND_NEMATOR_SMELL_FOOD,
	};

	ClearConditions( conditionsToClear, ARRAYSIZE( conditionsToClear ) );
	
	pCurrentSound = GetSenses()->GetFirstHeardSound( &iter );
	
	while ( pCurrentSound )
	{
		// the npc cares about this sound, and it's close enough to hear.
		int condition = COND_NONE;
		
		if ( !pCurrentSound->FIsSound() )
		{
			// if not a sound, must be a smell - determine if it's just a scent, or if it's a food scent
			if ( pCurrentSound->IsSoundType( SOUND_MEAT | SOUND_CARCASS ) )
			{
				// the detected scent is a food item
				condition = COND_NEMATOR_SMELL_FOOD;
			}
		}
		
		if ( condition != COND_NONE )
			SetCondition( condition );

		pCurrentSound = GetSenses()->GetNextHeardSound( &iter );
	}

	BaseClass::OnListened();
}

//========================================================
// RunAI - overridden for nemator because there are things
// that need to be checked every think.
//========================================================
void CNPC_Nemator::RunAI ( void )
{
	// first, do base class stuff
	BaseClass::RunAI();

	if ( m_nSkin != 0 )
	{
		// close eye if it was open.
		m_nSkin = 0; 
	}

	if ( random->RandomInt( 0,39 ) == 0 )
	{
		m_nSkin = 1;
	}

	if ( GetEnemy() != NULL && GetActivity() == ACT_RUN )
	{
		// chasing enemy. Sprint for last bit
		if ( (GetAbsOrigin() - GetEnemy()->GetAbsOrigin()).Length2D() < NEMATOR_SPRINT_DIST )
		{
			m_flPlaybackRate = 1.25;
		}
	}

}


void CNPC_Nemator::Ignite( float flFlameLifetime, bool bNPCOnly, float flSize, bool bCalledByLevelDesigner )
{
	BaseClass::Ignite( flFlameLifetime, bNPCOnly, flSize, bCalledByLevelDesigner );

	// Set the zombie up to burn to death in about ten seconds.
	SetHealth( MIN( m_iHealth, FLAME_DIRECT_DAMAGE_PER_SEC * (ZOMBIE_BURN_TIME + random->RandomFloat( -ZOMBIE_BURN_TIME_NOISE, ZOMBIE_BURN_TIME_NOISE)) ) );

	// FIXME: use overlays when they come online
	//AddOverlay( ACT_ZOM_WALK_ON_FIRE, false );
		Activity activity = GetActivity();
		Activity burningActivity = activity;

		if ( activity == ACT_WALK )
		{
			burningActivity = ACT_WALK_ON_FIRE;
		}
		else if ( activity == ACT_RUN )
		{
			burningActivity = ACT_RUN_ON_FIRE;
		}
		else if ( activity == ACT_IDLE )
		{
			burningActivity = ACT_IDLE_ON_FIRE;
		}

		if( HaveSequenceForActivity(burningActivity) )
		{
			// Make sure we have a sequence for this activity (torsos don't have any, for instance) 
			// to prevent the baseNPC & baseAnimating code from throwing red level errors.
			SetActivity( burningActivity );
		}
}

//=========================================================
// GetSchedule 
//=========================================================
int CNPC_Nemator::SelectSchedule( void )
{
	switch	( m_NPCState )
	{
	case NPC_STATE_ALERT:
		{
			if ( HasCondition( COND_LIGHT_DAMAGE ) || HasCondition( COND_HEAVY_DAMAGE ) )
			{
				return SCHED_NEMATOR_HURTHOP;
			}

			if ( HasCondition( COND_NEMATOR_SMELL_FOOD ) )
			{
				CSound		*pSound;

				pSound = GetBestScent();
				
				if ( pSound && (!FInViewCone ( pSound->GetSoundOrigin() ) || !FVisible ( pSound->GetSoundOrigin() )) )
				{
					// scent is behind or occluded
					return SCHED_NEMATOR_SNIFF_AND_EAT;
				}

				// food is right out in the open. Just go get it.
				return SCHED_NEMATOR_EAT;
			}

			if ( HasCondition( COND_SMELL ) )
			{
				// there's something stinky. 
				CSound		*pSound;

				pSound = GetBestScent();
				if ( pSound )
					return SCHED_NEMATOR_WALLOW;
			}

			break;
		}
	case NPC_STATE_COMBAT:
		{
// dead enemy
			if ( HasCondition( COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return BaseClass::SelectSchedule();
			}

			if ( HasCondition( COND_NEW_ENEMY ) )
			{
				if ( m_fCanThreatDisplay && IRelationType( GetEnemy() ) == D_HT && FClassnameIs( GetEnemy(), "npc_citizen" ) )
				{
					// this means squid sees a headcrab!
					m_fCanThreatDisplay = FALSE;// only do the headcrab dance once per lifetime.
					return SCHED_NEMATOR_SEECRAB;
				}
				else
				{
					return SCHED_WAKE_ANGRY;
				}
			}

			if ( HasCondition( COND_NEMATOR_SMELL_FOOD ) )
			{
				CSound		*pSound;

				pSound = GetBestScent();
				
				if ( pSound && (!FInViewCone ( pSound->GetSoundOrigin() ) || !FVisible ( pSound->GetSoundOrigin() )) )
				{
					// scent is behind or occluded
					return SCHED_NEMATOR_SNIFF_AND_EAT;
				}

				// food is right out in the open. Just go get it.
				return SCHED_NEMATOR_EAT;
			}

			if ( HasCondition( COND_CAN_RANGE_ATTACK1 ) )
			{
				return SCHED_RANGE_ATTACK1;
			}

			if ( HasCondition( COND_CAN_MELEE_ATTACK1 ) )
			{
				return SCHED_MELEE_ATTACK1;
			}

			if ( HasCondition( COND_CAN_MELEE_ATTACK2 ) )
			{
				return SCHED_MELEE_ATTACK2;
			}
			
			return SCHED_CHASE_ENEMY;

			break;
		}
	}

	return BaseClass::SelectSchedule();
}



//=========================================================
// FInViewCone - returns true is the passed vector is in
// the caller's forward view cone. The dot product is performed
// in 2d, making the view cone infinitely tall. 
//=========================================================
bool CNPC_Nemator::FInViewCone ( Vector pOrigin )
{
	Vector los = ( pOrigin - GetAbsOrigin() );

	// do this in 2D
	los.z = 0;
	VectorNormalize( los );

	Vector facingDir = EyeDirection2D( );

	float flDot = DotProduct( los, facingDir );

	if ( flDot > m_flFieldOfView )
		return true;

	return false;
}

//=========================================================
// FVisible - returns true if a line can be traced from
// the caller's eyes to the target vector
//=========================================================
bool CNPC_Nemator::FVisible ( Vector vecOrigin )
{
	trace_t tr;
	Vector		vecLookerOrigin;
	
	vecLookerOrigin = EyePosition();//look through the caller's 'eyes'
	UTIL_TraceLine(vecLookerOrigin, vecOrigin, MASK_BLOCKLOS, this/*pentIgnore*/, COLLISION_GROUP_NONE, &tr);
	
	if ( tr.fraction != 1.0 )
		 return false; // Line of sight is not established
	else
		 return true;// line of sight is valid.
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.  OVERRIDDEN for nemator because it needs to
// know explicitly when the last attempt to chase the enemy
// failed, since that impacts its attack choices.
//=========================================================
void CNPC_Nemator::StartTask ( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_MELEE_ATTACK2:
		{
			CPASAttenuationFilter filter( this );
			EmitSound( filter, entindex(), "Nemator.Growl" );		
			BaseClass::StartTask ( pTask );
			break;
		}
	case TASK_MELEE_ATTACK1:
		{
			CPASAttenuationFilter filter( this );
			EmitSound( filter, entindex(), "Nemator.Growl" );		
			BaseClass::StartTask ( pTask );
			break;
		}
	case TASK_RANGE_ATTACK1:
		{
			CPASAttenuationFilter filter( this );
			EmitSound( filter, entindex(), "Nemator.Shoot" );		
			BaseClass::StartTask ( pTask );
			break;
		}
	case TASK_NEMATOR_HOPTURN:
		{
			SetActivity ( ACT_HOP );
			
			if ( GetEnemy() )
			{
				Vector	vecFacing = ( GetEnemy()->GetAbsOrigin() - GetAbsOrigin() );
				VectorNormalize( vecFacing );

				GetMotor()->SetIdealYaw( vecFacing );
			}

			break;
		}
	case TASK_NEMATOR_EAT:
		{
			m_flHungryTime = gpGlobals->curtime + pTask->flTaskData;
			break;
		}

	default:
		{
			BaseClass::StartTask ( pTask );
			break;
		}
	}
}

//=========================================================
// RunTask
//=========================================================
void CNPC_Nemator::RunTask ( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_NEMATOR_HOPTURN:
		{
			if ( GetEnemy() )
			{
				Vector	vecFacing = ( GetEnemy()->GetAbsOrigin() - GetAbsOrigin() );
				VectorNormalize( vecFacing );
				GetMotor()->SetIdealYaw( vecFacing );
			}

			if ( IsSequenceFinished() )
			{
				TaskComplete(); 
			}
			break;
		}
	default:
		{
			BaseClass::RunTask( pTask );
			break;
		}
	}
}

//=========================================================
// GetIdealState - Overridden for Nemator to deal with
// the feature that makes it lose interest in headcrabs for 
// a while if something injures it. 
//=========================================================
NPC_STATE CNPC_Nemator::SelectIdealState ( void )
{
	// If no schedule conditions, the new ideal state is probably the reason we're in here.
	switch ( m_NPCState )
	{
	case NPC_STATE_COMBAT:
		/*
		COMBAT goes to ALERT upon death of enemy
		*/
		{
			if ( GetEnemy() != NULL && ( HasCondition( COND_LIGHT_DAMAGE ) || HasCondition ( COND_HEAVY_DAMAGE ) ) && FClassnameIs( GetEnemy(), "npc_citizen" ) )
			{
				// if the squid has a headcrab enemy and something hurts it, it's going to forget about the crab for a while.
				SetEnemy( NULL );
				return NPC_STATE_ALERT;
			}
			break;
		}
	}

	return BaseClass::SelectIdealState();
}


//------------------------------------------------------------------------------
//
// Schedules
//
//------------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( monster_nemator, CNPC_Nemator )

	DECLARE_TASK ( TASK_NEMATOR_HOPTURN )
	DECLARE_TASK ( TASK_NEMATOR_EAT )

	DECLARE_CONDITION( COND_NEMATOR_SMELL_FOOD )

	DECLARE_ACTIVITY( ACT_NEMATOR_EXCITED )
	DECLARE_ACTIVITY( ACT_NEMATOR_EAT )
	DECLARE_ACTIVITY( ACT_NEMATOR_DETECT_SCENT )
	DECLARE_ACTIVITY( ACT_NEMATOR_INSPECT_FLOOR )

	//=========================================================
	// > SCHED_NEMATOR_HURTHOP
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_NEMATOR_HURTHOP,
	
		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_SOUND_WAKE				0"
		"		TASK_NEMATOR_HOPTURN			0"
		"		TASK_FACE_ENEMY				0"
		"	"
		"	Interrupts"
	)
	
	//=========================================================
	// > SCHED_NEMATOR_SEECRAB
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_NEMATOR_SEECRAB,
	
		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_SOUND_WAKE				0"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_NEMATOR_EXCITED"
		"		TASK_FACE_ENEMY				0"
		"	"
		"	Interrupts"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
	)
	
	//=========================================================
	// > SCHED_NEMATOR_EAT
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_NEMATOR_EAT,
	
		"	Tasks"
		"		TASK_STOP_MOVING					0"
		"		TASK_NEMATOR_EAT						10"
		"		TASK_STORE_LASTPOSITION				0"
		"		TASK_GET_PATH_TO_BESTSCENT			0"
		"		TASK_WALK_PATH						0"
		"		TASK_WAIT_FOR_MOVEMENT				0"
		"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_NEMATOR_EAT"
		"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_NEMATOR_EAT"
		"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_NEMATOR_EAT"
		"		TASK_NEMATOR_EAT						50"
		"		TASK_GET_PATH_TO_LASTPOSITION		0"
		"		TASK_WALK_PATH						0"
		"		TASK_WAIT_FOR_MOVEMENT				0"
		"		TASK_CLEAR_LASTPOSITION				0"
		"	"
		"	Interrupts"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_NEW_ENEMY"
		"		COND_SMELL"
	)
	
	//=========================================================
	// > SCHED_NEMATOR_SNIFF_AND_EAT
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_NEMATOR_SNIFF_AND_EAT,
	
		"	Tasks"
		"		TASK_STOP_MOVING					0"
		"		TASK_NEMATOR_EAT						10"
		"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_NEMATOR_DETECT_SCENT"
		"		TASK_STORE_LASTPOSITION				0"
		"		TASK_GET_PATH_TO_BESTSCENT			0"
		"		TASK_WALK_PATH						0"
		"		TASK_WAIT_FOR_MOVEMENT				0"
		"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_NEMATOR_EAT"
		"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_NEMATOR_EAT"
		"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_NEMATOR_EAT"
		"		TASK_NEMATOR_EAT						50"
		"		TASK_GET_PATH_TO_LASTPOSITION		0"
		"		TASK_WALK_PATH						0"
		"		TASK_WAIT_FOR_MOVEMENT				0"
		"		TASK_CLEAR_LASTPOSITION				0"
		"	"
		"	Interrupts"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_NEW_ENEMY"
		"		COND_SMELL"
	)
	
	//=========================================================
	// > SCHED_NEMATOR_WALLOW
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_NEMATOR_WALLOW,
	
		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_NEMATOR_EAT					10"
		"		TASK_STORE_LASTPOSITION			0"
		"		TASK_GET_PATH_TO_BESTSCENT		0"
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_NEMATOR_INSPECT_FLOOR"
		"		TASK_NEMATOR_EAT					50"
		"		TASK_GET_PATH_TO_LASTPOSITION	0"
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_CLEAR_LASTPOSITION			0"
		"	"
		"	Interrupts"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_NEW_ENEMY"
	)
	
	//=========================================================
	// > SCHED_NEMATOR_CHASE_ENEMY
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_NEMATOR_CHASE_ENEMY,
	
		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_RANGE_ATTACK1"
		"		TASK_GET_PATH_TO_ENEMY			0"
		"		TASK_RUN_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"	"
		"	Interrupts"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_SMELL"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_TASK_FAILED"
	)

AI_END_CUSTOM_NPC()
