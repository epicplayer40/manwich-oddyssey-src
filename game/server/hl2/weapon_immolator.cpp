//========= Copyright � 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "fire.h"
#include "basehlcombatweapon.h"
#include "npcevent.h"
#include "basecombatcharacter.h"
#include "player.h"
#include "soundent.h"
#include "te_particlesystem.h"
#include "ndebugoverlay.h"
#include "in_buttons.h"
#include "ai_basenpc.h"
#include "ai_memory.h"
#include "beam_shared.h"
#include "EntityFlame.h"
#include "basehlcombatweapon_shared.h"
#include "gamerules.h"
#include "game.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "IEffects.h"
#include "te_effect_dispatch.h"
#include "Sprite.h"
#include "SpriteTrail.h"
#include "rumble_shared.h"
#include "gamestats.h"
#include "decals.h"

#include "weapon_immolator.h"

extern ConVar sk_cremator_dmg_immo;


#define MAX_BURN_RADIUS		256
#define RADIUS_GROW_RATE	50.0	// units/sec 

#define IMMOLATOR_TARGET_INVALID Vector( FLT_MAX, FLT_MAX, FLT_MAX )

#define BEAM_MODEL	"models/crossbow_bolt.mdl"
#define BEAM_AIR_VELOCITY	1500


//-----------------------------------------------------------------------------
// Crossbow Bolt
//-----------------------------------------------------------------------------
class CImmolatorBeam : public CBaseCombatCharacter
{
	DECLARE_CLASS( CImmolatorBeam, CBaseCombatCharacter );

public:
	CImmolatorBeam() { };
	~CImmolatorBeam();

	Class_T Classify( void ) { return CLASS_NONE; }

public:
	void Spawn( void );
	void Precache( void );
	void BubbleThink( void );
	void BoltTouch( CBaseEntity *pOther );
	bool CreateVPhysics( void );
	void ImmolationDamage( const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore );
	unsigned int PhysicsSolidMaskForEntity() const;
	static CImmolatorBeam *BoltCreate( const Vector &vecOrigin, const QAngle &angAngles, CBaseCombatCharacter *pentOwner = NULL );
	int	m_immobeamIndex;

protected:

	bool	CreateSprites( void );

	CHandle<CSprite>		m_pGlowSprite;
	CHandle<CSpriteTrail>	m_pGlowTrail;

	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();
};
LINK_ENTITY_TO_CLASS( immolator_beam, CImmolatorBeam );

BEGIN_DATADESC( CImmolatorBeam )
	// Function Pointers
	DEFINE_FUNCTION( BubbleThink ),
	DEFINE_FUNCTION( BoltTouch ),

	// These are recreated on reload, they don't need storage
	DEFINE_FIELD( m_pGlowSprite, FIELD_EHANDLE ),
	DEFINE_FIELD( m_pGlowTrail, FIELD_EHANDLE ),
	DEFINE_FIELD(m_immobeamIndex, FIELD_INTEGER),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CImmolatorBeam, DT_ImmolatorBeam )
END_SEND_TABLE()

CImmolatorBeam *CImmolatorBeam::BoltCreate( const Vector &vecOrigin, const QAngle &angAngles, CBaseCombatCharacter *pentOwner )
{
	// Create a new entity with CImmolatorBeam private data
	CImmolatorBeam *pBeam = (CImmolatorBeam *)CreateEntityByName( "immolator_beam" );
	UTIL_SetOrigin( pBeam, vecOrigin );
	pBeam->SetAbsAngles( angAngles );
	pBeam->Spawn();
	pBeam->SetOwnerEntity( pentOwner );

	return pBeam;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CImmolatorBeam::~CImmolatorBeam( void )
{
	if ( m_pGlowSprite )
	{
		UTIL_Remove( m_pGlowSprite );
	}
	if ( m_pGlowTrail )
	{
		UTIL_Remove( m_pGlowTrail );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CImmolatorBeam::CreateVPhysics( void )
{
	// Create the object in the physics system
	VPhysicsInitNormal( SOLID_BBOX, FSOLID_NOT_STANDABLE, false );

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
unsigned int CImmolatorBeam::PhysicsSolidMaskForEntity() const
{
//	return ( BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_HITBOX ) & ~CONTENTS_GRATE;
	return MASK_NPCSOLID;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CImmolatorBeam::CreateSprites( void )
{
	// Start up the eye glow
	m_pGlowSprite = CSprite::SpriteCreate( "sprites/light_glow02_noz.vmt", GetLocalOrigin(), false );

	if ( m_pGlowSprite != NULL )
	{
		m_pGlowSprite->FollowEntity( this );
		m_pGlowSprite->SetTransparency( kRenderGlow, 255, 255, 255, 128, kRenderFxNoDissipation );
		m_pGlowSprite->SetScale( 0.0f );
		m_pGlowSprite->TurnOff();
	}

	// Start up the eye trail
	m_pGlowTrail	= CSpriteTrail::SpriteTrailCreate( "sprites/bluelaser1.vmt", GetLocalOrigin(), false );

	if ( m_pGlowTrail != NULL )
	{
		m_pGlowTrail->FollowEntity( this );
//		m_pGlowTrail->SetAttachment( this, nAttachment );
		m_pGlowTrail->SetTransparency( kRenderTransAdd, 0, 255, 0, 255, kRenderFxNone );
		m_pGlowTrail->SetStartWidth( 5.2f );
		m_pGlowTrail->SetEndWidth( 1.0f );
		m_pGlowTrail->SetLifeTime( 0.9f );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CImmolatorBeam::Spawn( void )
{
	Precache( );

	SetModel( "models/crossbow_bolt.mdl" );
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
	UTIL_SetSize( this, -Vector(0.3f,0.3f,0.3f), Vector(0.3f,0.3f,0.3f) );
	SetSolid( SOLID_BBOX );
	SetGravity( 0.7f );
	AddEffects( EF_NODRAW );
	//epicplayer
	SetCollisionGroup( COLLISION_GROUP_PROJECTILE );

	m_immobeamIndex = engine->PrecacheModel("sprites/bluelaser1.vmt");

	// Make sure we're updated if we're underwater
	UpdateWaterState();

	SetTouch( &CImmolatorBeam::BoltTouch );

	SetThink( &CImmolatorBeam::BubbleThink );
	SetNextThink( gpGlobals->curtime + 0.1f );
	
	CreateSprites();
}


void CImmolatorBeam::Precache( void )
{
	PrecacheModel( BEAM_MODEL );

	// This is used by C_TEStickyBolt, despte being different from above!!!
	PrecacheModel( "models/crossbow_bolt.mdl" );

	PrecacheModel( "sprites/light_glow02_noz.vmt" );
}


void CImmolatorBeam::ImmolationDamage( const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore )
{
	CBaseEntity *pEntity = NULL;
	trace_t		tr;
	Vector		vecSpot;

	Vector vecSrc = vecSrcIn;

	//epicplayer - cursed immolator fadskjajs effect here
	int beams;

	for (beams = 0; beams < 5; beams++)
	{
		Vector vecDest;

		// Random unit vector
		vecDest.x = random->RandomFloat(-1, 1);
		vecDest.y = random->RandomFloat(-1, 1);
		vecDest.z = random->RandomFloat(0, 1);

		// Push out to radius dist.
		vecDest = vecSrc + vecDest * 80.1f;

		UTIL_Beam(vecSrc,
			vecDest,
			m_immobeamIndex,
			0,		//halo index
			0,		//frame start
			2.0f,	//framerate
			0.15f,	//life
			20,		// width
			1.75,	// endwidth
			0.75,	// fadelength,
			15,		// noise

			0,		// red
			255,	// green
			0,	// blue,

			128, // bright
			100  // speed
			);
	}

//	CEntitySphereQuery sphere(vecSrc, flRadius);
//	CBaseEntity *pEntity = sphere.GetCurrentEntity();

	// iterate on all entities in the vicinity.
	for ( CEntitySphereQuery sphere( vecSrc, flRadius ); (pEntity = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity() )
	{
		CBaseCombatCharacter *pBCC = pEntity->MyCombatCharacterPointer();
		if ( pBCC && pBCC->IsAlive() && !pBCC->IsOnFire() )
		{
			// UNDONE: this should check a damage mask, not an ignore
			if ( iClassIgnore != CLASS_NONE && pEntity->Classify() == iClassIgnore )
			{
				continue;
			}
		
			float lifetime = random->RandomFloat( 3, 5 );

			CEntityFlame *pFlame = CEntityFlame::Create( pBCC, true );
			if (pFlame)
			{
				pFlame->SetLifetime( lifetime );
				pBCC->AddFlag( FL_ONFIRE );
			}
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CImmolatorBeam::BoltTouch( CBaseEntity *pOther )
{
	if (!pOther->IsSolid())
		return;
		
	if ( pOther->IsSolidFlagSet(FSOLID_VOLUME_CONTENTS | FSOLID_TRIGGER) )
	{
		// Some NPCs are triggers that can take damage (like antlion grubs). We should hit them.
		if ( ( pOther->m_takedamage == DAMAGE_NO ) || ( pOther->m_takedamage == DAMAGE_EVENTS_ONLY ) )
			return;
	}

		RadiusDamage( CTakeDamageInfo( this, GetOwnerEntity(), sk_cremator_dmg_immo.GetFloat(), DMG_DISSOLVE | DMG_PLASMA | DMG_BURN ), GetAbsOrigin(), 100, CLASS_PLAYER_ALLY_VITAL, NULL ); //changed from 256 to 128 to correspond with noisebeams
		ImmolationDamage(CTakeDamageInfo(this, GetOwnerEntity(), 1, DMG_PLASMA), GetAbsOrigin(), 100, CLASS_PLAYER_ALLY_VITAL);
	/*
	if ( pOther->m_takedamage != DAMAGE_NO )
	{
		trace_t	tr, tr2;
		tr = BaseClass::GetTouchTrace();
		Vector	vecNormalizedVel = GetAbsVelocity();

		ClearMultiDamage();
		VectorNormalize( vecNormalizedVel );

		if( GetOwnerEntity() && GetOwnerEntity()->IsPlayer() && pOther->IsNPC() )
		{
			CTakeDamageInfo	dmgInfo( this, GetOwnerEntity(), sk_cremator_dmg_immo.GetFloat(), DMG_DISSOLVE | DMG_PLASMA | DMG_BURN );
			dmgInfo.AdjustPlayerDamageInflictedForSkillLevel();
			CalculateMeleeDamageForce( &dmgInfo, vecNormalizedVel, tr.endpos, 0.7f );
			dmgInfo.SetDamagePosition( tr.endpos );
			pOther->DispatchTraceAttack( dmgInfo, vecNormalizedVel, &tr );

			CBasePlayer *pPlayer = ToBasePlayer( GetOwnerEntity() );
			if ( pPlayer )
			{
				gamestats->Event_WeaponHit( pPlayer, true, "weapon_immolator", dmgInfo );
			}

		}
		else
		{
			CTakeDamageInfo	dmgInfo( this, GetOwnerEntity(), sk_cremator_dmg_immo.GetFloat(), DMG_DISSOLVE | DMG_PLASMA | DMG_BURN  );
			CalculateMeleeDamageForce( &dmgInfo, vecNormalizedVel, tr.endpos, 0.7f );
			dmgInfo.SetDamagePosition( tr.endpos );
			pOther->DispatchTraceAttack( dmgInfo, vecNormalizedVel, &tr );
		}

		ApplyMultiDamage();

		//Adrian: keep going through the glass.
		if ( pOther->GetCollisionGroup() == COLLISION_GROUP_BREAKABLE_GLASS )
			 return;

		if ( !pOther->IsAlive() )
		{
			// We killed it! 
			const surfacedata_t *pdata = physprops->GetSurfaceData( tr.surface.surfaceProps );
			if ( pdata->game.material == CHAR_TEX_GLASS )
			{
				return;
			}
		}

		SetAbsVelocity( Vector( 0, 0, 0 ) );

		// play body "thwack" sound
//		EmitSound( "Weapon_Crossbow.BoltHitBody" );

		Vector vForward;

		AngleVectors( GetAbsAngles(), &vForward );
		VectorNormalize ( vForward );

		UTIL_TraceLine( GetAbsOrigin(),	GetAbsOrigin() + vForward * 128, MASK_BLOCKLOS, pOther, COLLISION_GROUP_NONE, &tr2 );

		if ( tr2.fraction != 1.0f )
		{
//			NDebugOverlay::Box( tr2.endpos, Vector( -16, -16, -16 ), Vector( 16, 16, 16 ), 0, 255, 0, 0, 10 );
//			NDebugOverlay::Box( GetAbsOrigin(), Vector( -16, -16, -16 ), Vector( 16, 16, 16 ), 0, 0, 255, 0, 10 );

			if ( tr2.m_pEnt == NULL || ( tr2.m_pEnt && tr2.m_pEnt->GetMoveType() == MOVETYPE_NONE ) )
			{
				CEffectData	data;

				data.m_vOrigin = tr2.endpos;
				data.m_vNormal = vForward;
				data.m_nEntIndex = tr2.fraction != 1.0f;
				
						CBaseEntity *pEntity;
						pEntity = tr2.m_pEnt;

						if ( pEntity != NULL && m_takedamage )
						{
							RadiusDamage( CTakeDamageInfo( this, pEntity, sk_cremator_dmg_immo.GetFloat(), DMG_BURN ), tr.endpos, 256,  CLASS_NONE, NULL ); //changed from 256 to 128 to correspond with noisebeams
						}
						else
						{
							// The attack beam struck some kind of entity directly.
						}

				DispatchEffect( "BoltImpact", data );
				UTIL_Remove(this);
			}
			UTIL_Remove(this);
		}
		
		SetTouch( NULL );
		SetThink( NULL );


		UTIL_Remove( this );

	}
	else
	{
		trace_t	tr;
		tr = BaseClass::GetTouchTrace();

		// See if we struck the world
		if ( pOther->GetMoveType() == MOVETYPE_NONE && !( tr.surface.flags & SURF_SKY ) )
		{
//			EmitSound( "Weapon_Crossbow.BoltHitWorld" );

			// if what we hit is static architecture, can stay around for a while.
			Vector vecDir = GetAbsVelocity();
			float speed = VectorNormalize( vecDir );

			// See if we should reflect off this surface
			float hitDot = DotProduct( tr.plane.normal, -vecDir );
			
			if ( ( hitDot < 0.5f ) && ( speed > 100 ) )
			{
				Vector vReflection = 2.0f * tr.plane.normal * hitDot + vecDir;
				
				QAngle reflectAngles;

				VectorAngles( vReflection, reflectAngles );

				SetLocalAngles( reflectAngles );

				SetAbsVelocity( vReflection * speed * 0.75f );

				// Start to sink faster
				SetGravity( 1.0f );

				UTIL_Remove(this);
			}
			else
			{
				SetThink( &CImmolatorBeam::SUB_Remove );
				SetNextThink( gpGlobals->curtime + 2.0f );
				
				//FIXME: We actually want to stick (with hierarchy) to what we've hit
				SetMoveType( MOVETYPE_NONE );
			
				Vector vForward;

				AngleVectors( GetAbsAngles(), &vForward );
				VectorNormalize ( vForward );

				CEffectData	data;

				data.m_vOrigin = tr.endpos;
				data.m_vNormal = vForward;
				data.m_nEntIndex = 0;
			
				//DispatchEffect( "BoltImpact", data );
				
				UTIL_ImpactTrace( &tr, DMG_BULLET );

				

				AddEffects( EF_NODRAW );
				SetTouch( NULL );
				SetThink( &CImmolatorBeam::SUB_Remove );
				SetNextThink( gpGlobals->curtime + 2.0f );

				if ( m_pGlowSprite != NULL )
				{
					m_pGlowSprite->TurnOn();
					m_pGlowSprite->FadeAndDie( 3.0f );
				}

				UTIL_Remove(this);
			}
			
			// Shoot some sparks
			if ( UTIL_PointContents( GetAbsOrigin() ) != CONTENTS_WATER)
			{
				g_pEffects->Sparks( GetAbsOrigin() );
			}

			UTIL_Remove(this);
		}
		else
		{
			// Put a mark unless we've hit the sky
			if ( ( tr.surface.flags & SURF_SKY ) == false )
			{
				UTIL_ImpactTrace( &tr, DMG_BULLET );
			}
			*/
			UTIL_Remove( this );
//		}
//	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CImmolatorBeam::BubbleThink( void )
{
	QAngle angNewAngles;

	VectorAngles( GetAbsVelocity(), angNewAngles );
	SetAbsAngles( angNewAngles );

	SetNextThink( gpGlobals->curtime + 0.1f );

	// Make danger sounds out in front of me, to scare snipers back into their hole
	CSoundEnt::InsertSound( SOUND_DANGER_SNIPERONLY, GetAbsOrigin() + GetAbsVelocity() * 0.2, 120.0f, 0.5f, this, SOUNDENT_CHANNEL_REPEATED_DANGER );

	if ( GetWaterLevel()  == 0 )
		return;

	UTIL_BubbleTrail( GetAbsOrigin() - GetAbsVelocity() * 0.1f, GetAbsOrigin(), 5 );
}

IMPLEMENT_SERVERCLASS_ST(CWeaponImmolator, DT_WeaponImmolator)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( info_target_immolator, CPointEntity );
LINK_ENTITY_TO_CLASS( weapon_immolator, CWeaponImmolator );
PRECACHE_WEAPON_REGISTER( weapon_immolator );

BEGIN_DATADESC( CWeaponImmolator )

	DEFINE_FIELD( m_beamIndex, FIELD_INTEGER ),
	DEFINE_FIELD( m_flBurnRadius, FIELD_FLOAT ),
	DEFINE_FIELD( m_flTimeLastUpdatedRadius, FIELD_TIME ),
	DEFINE_FIELD( m_vecImmolatorTarget, FIELD_VECTOR ),

	DEFINE_FIELD( m_fireState, FIELD_INTEGER ),
	DEFINE_FIELD( m_flAmmoUseTime, FIELD_TIME ),
	DEFINE_FIELD( m_flStartFireTime, FIELD_TIME ),

	DEFINE_ENTITYFUNC( UpdateThink ),
END_DATADESC()


//-----------------------------------------------------------------------------
// Maps base activities to weapons-specific ones so our characters do the right things.
//-----------------------------------------------------------------------------
acttable_t CWeaponImmolator::m_acttable[] = 
{
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_SMG1,			true },
	{ ACT_RELOAD,					ACT_RELOAD_SMG1,				true },
	{ ACT_IDLE,						ACT_IDLE_SMG1,					true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SMG1,			true },

	{ ACT_WALK,						ACT_WALK_RIFLE,					true },
	{ ACT_WALK_AIM,					ACT_WALK_AIM_RIFLE,				true  },
	
// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED,				ACT_IDLE_SMG1_RELAXED,			false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_SMG1_STIMULATED,		false },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_SMG1,			false },//always aims

	{ ACT_WALK_RELAXED,				ACT_WALK_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_RIFLE_STIMULATED,		false },
	{ ACT_WALK_AGITATED,			ACT_WALK_AIM_RIFLE,				false },//always aims

	{ ACT_RUN_RELAXED,				ACT_RUN_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_RIFLE_STIMULATED,		false },
	{ ACT_RUN_AGITATED,				ACT_RUN_AIM_RIFLE,				false },//always aims

// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_SMG1_RELAXED,			false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_AIM_RIFLE_STIMULATED,	false },
	{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_SMG1,			false },//always aims

	{ ACT_WALK_AIM_RELAXED,			ACT_WALK_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_RIFLE_STIMULATED,	false },
	{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_RIFLE,				false },//always aims

	{ ACT_RUN_AIM_RELAXED,			ACT_RUN_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_RIFLE_STIMULATED,	false },
	{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_RIFLE,				false },//always aims
//End readiness activities

	{ ACT_WALK_AIM,					ACT_WALK_AIM_RIFLE,				true },
	{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,			true },
	{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,		true },
	{ ACT_RUN,						ACT_RUN_RIFLE,					true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_RIFLE,				true },
	{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,			true },
	{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,		true },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_SMG1,	true },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_SMG1_LOW,		true },
	{ ACT_COVER_LOW,				ACT_COVER_SMG1_LOW,				false },
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_SMG1_LOW,			false },
	{ ACT_RELOAD_LOW,				ACT_RELOAD_SMG1_LOW,			false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_SMG1,		true },
};

IMPLEMENT_ACTTABLE( CWeaponImmolator );

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponImmolator::CWeaponImmolator( void )
{
	m_fMaxRange1 = 1200;
	m_fMinRange1 = 200;

//	StopImmolating();
}

void CWeaponImmolator::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
		case EVENT_WEAPON_SMG1:
		{
//		Operator_ForceNPCFire( pOperator, 0 );
		Vector	muzzlePoint;
		QAngle	vecAngles;

		muzzlePoint = GetOwner()->Weapon_ShootPosition();

		CAI_BaseNPC *npc = pOperator->MyNPCPointer();


		Vector vecShootDir = npc->GetActualShootTrajectory(muzzlePoint);

		// Fire!
		Vector vecSrc;
		Vector vecAiming;

		QAngle	angShootDir;
		GetAttachment(LookupAttachment("muzzle"), vecSrc, angShootDir);
		AngleVectors(angShootDir, &vecAiming);

		// look for a better launch location
		Vector altLaunchPoint;
		if (GetAttachment("muzzle", altLaunchPoint))
		{
			// check to see if it's relativly free
			trace_t tr;
			AI_TraceHull(altLaunchPoint, altLaunchPoint + vecShootDir * (10.0f*12.0f), Vector(-24, -24, -24), Vector(24, 24, 24), MASK_NPCSOLID, NULL, &tr);

			if (tr.fraction == 1.0)
			{
				muzzlePoint = altLaunchPoint;
			}
		}

		vecShootDir = npc->GetActualShootTrajectory(muzzlePoint);
		VectorAngles(vecShootDir, vecAngles);

		//	pViewModel->GetAttachment( pViewModel->LookupAttachment( "muzzle" ), beamSrc );
		CImmolatorBeam *pBeamer = CImmolatorBeam::BoltCreate(vecSrc, angShootDir, pOperator);
		pBeamer->SetAbsVelocity(vecAiming * BEAM_AIR_VELOCITY);
		}
		break;
		default:
			BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	}
}

void CWeaponImmolator::Operator_ForceNPCFire( CBaseCombatCharacter *pOperator, bool bSecondary )
{
	WeaponSound(SINGLE);

	Vector	muzzlePoint;
	QAngle	vecAngles;

	muzzlePoint = GetOwner()->Weapon_ShootPosition();

	CAI_BaseNPC *npc = pOperator->MyNPCPointer();
	ASSERT( npc != NULL );

	Vector vecShootDir = npc->GetActualShootTrajectory( muzzlePoint );
	VectorAngles(vecShootDir, vecAngles);

	//	pViewModel->GetAttachment( pViewModel->LookupAttachment( "muzzle" ), beamSrc );
	CImmolatorBeam *pBeamer = CImmolatorBeam::BoltCreate(muzzlePoint, vecAngles, pOperator);
	pBeamer->SetAbsVelocity(muzzlePoint * BEAM_AIR_VELOCITY);
	/*
	Vector	impactPoint	= muzzlePoint + ( vecShootDir * MAX_TRACE_LENGTH );

	trace_t	tr;
	AI_TraceHull( muzzlePoint, impactPoint, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), MASK_SHOT, pOperator, COLLISION_GROUP_NONE, &tr );
	AI_TraceLine( muzzlePoint, impactPoint, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
//	CreateConcussiveBlast( tr.endpos, tr.plane.normal, this, 1.0 );
		CBeam *pBeam = CBeam::BeamCreate( "sprites/bluelaser1.vmt", 20 );
	pBeam->PointEntInit( tr.endpos, this );
	pBeam->SetAbsStartPos( tr.endpos );
	pBeam->SetEndAttachment( 1 );
	pBeam->SetScrollRate( 2.0f );		//framerate
	pBeam->LiveForTime( 0.1f );			//life
	pBeam->SetEndWidth( 1 );			// endwidth
	pBeam->SetFadeLength( 0 );			// fadelength,
	pBeam->SetNoise( 1 );				// noise
	pBeam->SetColor( 0, 255, 0 );		// red, green, blue,
//	pBeam->SetColor( 0, 165, 255 );		// red, green, blue,
	pBeam->SetBrightness( 255 );	// bright

	if( true || tr.DidHitWorld() )
	{
		int beams;

		for( beams = 0 ; beams < 5 ; beams++ )
		{
			Vector vecDest;

			// Random unit vector
			vecDest.x = random->RandomFloat( -1, 1 );
			vecDest.y = random->RandomFloat( -1, 1 );
			vecDest.z = random->RandomFloat( 0, 1 );

			// Push out to radius dist.
			vecDest = tr.endpos + vecDest * m_flBurnRadius;

			UTIL_Beam(  tr.endpos,
						vecDest,
						m_beamIndex,
						0,		//halo index
						0,		//frame start
						2.0f,	//framerate
						0.15f,	//life
						20,		// width
						1.75,	// endwidth
						0.75,	// fadelength,
						15,		// noise

					//	0,		// red
					//	255,	// green
					//	0,		// blue,

						0,		// red
						255,	// green
						0,	// blue,

						128, // bright
						100  // speed
						);
		}

	
		CBaseEntity *pEntity;
		pEntity = tr.m_pEnt;

	if ( pEntity != NULL && m_takedamage )
	{
		RadiusDamage( CTakeDamageInfo( this, pEntity, sk_cremator_dmg_immo.GetFloat(), DMG_BURN ), tr.endpos, 256,  CLASS_NONE, NULL ); //changed from 256 to 128 to correspond with noisebeams
	}
	else
	{
		// The attack beam struck some kind of entity directly.
	}
	}*/
}

bool CWeaponImmolator::Deploy( void )
{
	m_fireState = FIRE_OFF;

	return BaseClass::Deploy();
}

bool CWeaponImmolator::Holster( CBaseCombatWeapon *pSwitchingTo )
{
    StopImmolating();

	return BaseClass::Holster( pSwitchingTo );
}

/*
bool CWeaponImmolator::HasAmmo( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner == NULL )
		return false;
	if ( pOwner->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
		return false;
	return true;
}
*/

void CWeaponImmolator::UseAmmo( int count )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner == NULL )
		return;

	if ( pOwner->GetAmmoCount( m_iPrimaryAmmoType ) >= count )
		pOwner->RemoveAmmo( count, m_iPrimaryAmmoType );
	else
		pOwner->RemoveAmmo( pOwner->GetAmmoCount( m_iPrimaryAmmoType ), m_iPrimaryAmmoType );
}

void CWeaponImmolator::StartImmolating()
{
	// Start the radius really tiny because we use radius == 0.0 to 
	// determine whether the immolator is operating or not.
	m_flBurnRadius = 0.1;
	m_flTimeLastUpdatedRadius = gpGlobals->curtime;
	SetThink( &CWeaponImmolator::UpdateThink );
	SetNextThink( gpGlobals->curtime );
}

void CWeaponImmolator::StopImmolating()
{
	m_flBurnRadius = 0.0;
	SetThink( NULL );
	m_vecImmolatorTarget= IMMOLATOR_TARGET_INVALID;
	m_flNextPrimaryAttack = gpGlobals->curtime + 2.0;

	StopSound( "Weapon_Immolator.Run" );
	
	if ( m_fireState != FIRE_OFF )
	{
		 EmitSound( "Weapon_Immolator.Off" );
	}

	SetWeaponIdleTime( gpGlobals->curtime + 2.0 );
	m_flNextPrimaryAttack = gpGlobals->curtime + 0.5;

	m_fireState = FIRE_OFF;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponImmolator::Precache( void )
{
	m_beamIndex = engine->PrecacheModel( "sprites/bluelaser1.vmt" );

	BaseClass::Precache();

	PrecacheScriptSound( "Weapon_Immolator.Start" );
	PrecacheScriptSound( "Weapon_Immolator.Run" );
	PrecacheScriptSound( "Weapon_Immolator.Off" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponImmolator::PrimaryAttack( void )
{
/*
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner == NULL )
		return;
	WeaponSound( SINGLE );
	if( !IsImmolating() )
	{
		StartImmolating();
	} 
	// VXP
	pOwner->m_fEffects |= EF_MUZZLEFLASH;
	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
*/

	switch( m_fireState )
	{
		case FIRE_OFF:
		{
			if ( !HasAmmo() )
			{
				Msg( "No ammo!\n" );
				m_flNextPrimaryAttack = gpGlobals->curtime + 0.25;
				WeaponSound( EMPTY );
				return;
			}

			m_flAmmoUseTime = gpGlobals->curtime;// start using ammo ASAP.
			
			EmitSound( "Weapon_Immolator.Start" ); // VXP: Play startup sound here
			
			SendWeaponAnim( ACT_VM_PRIMARYATTACK );
			
			m_flStartFireTime = gpGlobals->curtime;

			SetWeaponIdleTime( gpGlobals->curtime + 0.1 );
			
			m_fireState = FIRE_STARTUP;
		}
		break;

		case FIRE_STARTUP:
		{
		//	StartImmolating();

			if ( gpGlobals->curtime >= ( m_flStartFireTime + 0.3 ) )
			{
				EmitSound( "Weapon_Immolator.Run" );

				m_fireState = FIRE_CHARGE;
			}

			if ( !HasAmmo() )
			{
				StopImmolating();
				m_flNextPrimaryAttack = gpGlobals->curtime + 1.0;
			}
		}
		break;

		case FIRE_CHARGE:
		{
			if( !IsImmolating() )
			{
				StartImmolating();
			}

			if ( !HasAmmo() )
			{
				StopImmolating();
				m_flNextPrimaryAttack = gpGlobals->curtime + 1.0;
			}
		}
		break;
	}
}

//-----------------------------------------------------------------------------
// This weapon is said to have Line of Sight when it CAN'T see the target, but
// can see a place near the target than can.
//-----------------------------------------------------------------------------
bool CWeaponImmolator::WeaponLOSCondition( const Vector &ownerPos, const Vector &targetPos, bool bSetConditions )
{
	CAI_BaseNPC* npcOwner = GetOwner()->MyNPCPointer();
	
	if( !npcOwner )
	{
		return false;
	}
	/*
	if( IsImmolating() )
	{
		// Don't update while Immolating. This is a committed attack.
		return false;
	}

	// Assume we won't find a target.
//	m_vecImmolatorTarget = IMMOLATOR_TARGET_INVALID;
	m_vecImmolatorTarget = targetPos;

	if( npcOwner->FInViewCone( targetPos ) && npcOwner->FVisible( targetPos ) )
	{
		// We can see the target (usually the enemy). Don't fire the immolator directly at them.
		return false;
	}

	// Find a nearby immolator target that CAN see targetPOS
	float flNearest = FLT_MAX;
	CBaseEntity *pNearest = NULL;

	CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, "info_target_immolator" );

	while( pEnt )
	{
		float flDist = ( pEnt->GetAbsOrigin() - targetPos ).Length();

		if( flDist < flNearest )
		{
			trace_t tr;

			UTIL_TraceLine( targetPos, pEnt->GetAbsOrigin(), MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr );

			if( tr.fraction == 1.0 )
			{
				pNearest = pEnt;
				flNearest = flDist;
			}
		}

		pEnt = gEntList.FindEntityByClassname( pEnt, "info_target_immolator" );
	}

	if( pNearest )
	{
		m_vecImmolatorTarget = pNearest->GetAbsOrigin();
		return true;
	}

	return false;
	*/
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Weapon firing conditions
//-----------------------------------------------------------------------------
int CWeaponImmolator::WeaponRangeAttack1Condition( float flDot, float flDist )
{
	/*
	if( m_flNextPrimaryAttack > gpGlobals->curtime )
	{
		// Too soon to attack!
		return COND_NONE;
	}

	if( IsImmolating() )
	{
		// Once is enough!
		return COND_NONE;
	}
	
	if(	m_vecImmolatorTarget == IMMOLATOR_TARGET_INVALID )
	{
		// No target!
		return COND_NONE;
	}
	
	if ( flDist > m_fMaxRange1 )
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	else if ( flDot < 0.5f )	// UNDONE: Why check this here? Isn't the AI checking this already?
	{
		return COND_NOT_FACING_ATTACK;
	}
	*/
	// Ignore vertical distance when doing our RPG distance calculations
	CAI_BaseNPC *pNPC = GetOwner()->MyNPCPointer();
	if (pNPC)
	{
		CBaseEntity *pEnemy = pNPC->GetEnemy();
		Vector vecToTarget = (pEnemy->GetAbsOrigin() - pNPC->GetAbsOrigin());
		vecToTarget.z = 0;
		flDist = vecToTarget.Length();
	}
	
	if (flDist < MIN(m_fMinRange1, m_fMinRange2))
		return COND_TOO_CLOSE_TO_ATTACK;

	if (m_flNextPrimaryAttack > gpGlobals->curtime)
		return 0;
	/*
	// See if there's anyone in the way!
	CAI_BaseNPC *pOwner = GetOwner()->MyNPCPointer();
	ASSERT(pOwner != NULL);

	if (pOwner)
	{
		// Make sure I don't shoot the world!
		trace_t tr;

		Vector vecMuzzle = pOwner->Weapon_ShootPosition();
		Vector vecShootDir = pOwner->GetActualShootTrajectory(vecMuzzle);

		// Make sure I have a good 10 feet of wide clearance in front, or I'll blow my teeth out.
		AI_TraceHull(vecMuzzle, vecMuzzle + vecShootDir * (10.0f*12.0f), Vector(-24, -24, -24), Vector(24, 24, 24), MASK_NPCSOLID, NULL, &tr);

		if (tr.fraction != 1.0)
		{
			return COND_WEAPON_SIGHT_OCCLUDED;
		}
	}
	*/
	return COND_CAN_RANGE_ATTACK1;

	// !!!HACKHACK this makes the strider work! 
	// It's the Immolator's RangeAttack1, but it's the Strider's Range Attack 2.
//	return COND_CAN_RANGE_ATTACK2;
}

void CWeaponImmolator::UpdateThink( void )
{
	CBaseCombatCharacter *pOwner = GetOwner();

	if( pOwner && !pOwner->IsAlive() )
	{
		StopImmolating();
		return;
	}

	FireBeam();
	//Update();
	SetNextThink( gpGlobals->curtime + 0.05 );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CWeaponImmolator::Update()
{
	float flDuration = gpGlobals->curtime - m_flTimeLastUpdatedRadius;
	if( flDuration != 0.0 )
	{
		m_flBurnRadius += RADIUS_GROW_RATE * flDuration;
	}

	// Clamp
	m_flBurnRadius = min( m_flBurnRadius, MAX_BURN_RADIUS );

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	Vector vecSrc;
	Vector vecStart;
	Vector vecAiming;
	QAngle dummy;

	if( pOwner )
	{
		vecSrc	 = pOwner->Weapon_ShootPosition( );

		CBaseViewModel *pViewModel = pOwner->GetViewModel();
		pViewModel->GetAttachment( pViewModel->LookupAttachment( "muzzle" ), vecStart, dummy );

		vecAiming = pOwner->GetAutoaimVector(AUTOAIM_2DEGREES);
	}
	else
	{
		CBaseCombatCharacter *pOwner = GetOwner();

		vecSrc = pOwner->Weapon_ShootPosition( );
		vecAiming = m_vecImmolatorTarget - vecSrc;
		VectorNormalize( vecAiming );

		vecStart = vecSrc;
	}

	trace_t	tr;
	UTIL_TraceLine( vecSrc, vecSrc + vecAiming * MAX_TRACE_LENGTH, MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &tr );

	CSoundEnt::InsertSound( SOUND_DANGER, m_vecImmolatorTarget, 256, 5.0, GetOwner() );

	if ( gpGlobals->curtime >= m_flAmmoUseTime )
	{
		UseAmmo( 1 );
		ImmolationDamage( CTakeDamageInfo( this, this, 1, DMG_PLASMA ), tr.endpos, m_flBurnRadius, CLASS_NONE );
		
		if ( !HasAmmo() )
		{
			StopImmolating();
		}
		
		m_flAmmoUseTime = gpGlobals->curtime + 0.1;
	}

	int brightness;
	brightness = 255 * (m_flBurnRadius/MAX_BURN_RADIUS);
/*
	UTIL_Beam(  vecSrc,
				tr.endpos,
				m_beamIndex,
				0,		//halo index
				0,		//frame start
				2.0f,	//framerate
				0.1f,	//life
				20,		// width
				1,		// endwidth
				0,		// fadelength,
				1,		// noise
				0,		// red
				255,	// green
				0,		// blue,
				brightness, // bright
				100  // speed
				);
*/
	CBeam *pBeam = CBeam::BeamCreate( "sprites/bluelaser1.vmt", 20 );
	pBeam->PointEntInit( vecStart, this );
	pBeam->SetAbsStartPos( tr.endpos );
	pBeam->SetEndAttachment( 1 );
	pBeam->SetScrollRate( 2.0f );		//framerate
	pBeam->LiveForTime( 0.1f );			//life
	pBeam->SetEndWidth( 1 );			// endwidth
	pBeam->SetFadeLength( 0 );			// fadelength,
	pBeam->SetNoise( 1 );				// noise
	pBeam->SetColor( 0, 255, 0 );		// red, green, blue,
//	pBeam->SetColor( 0, 165, 255 );		// red, green, blue,
	pBeam->SetBrightness( brightness );	// bright

	if( true || tr.DidHitWorld() )
	{
		int beams;

		for( beams = 0 ; beams < 5 ; beams++ )
		{
			Vector vecDest;

			// Random unit vector
			vecDest.x = random->RandomFloat( -1, 1 );
			vecDest.y = random->RandomFloat( -1, 1 );
			vecDest.z = random->RandomFloat( 0, 1 );

			// Push out to radius dist.
			vecDest = tr.endpos + vecDest * m_flBurnRadius;

			UTIL_Beam(  tr.endpos,
						vecDest,
						m_beamIndex,
						0,		//halo index
						0,		//frame start
						2.0f,	//framerate
						0.15f,	//life
						20,		// width
						1.75,	// endwidth
						0.75,	// fadelength,
						15,		// noise

					//	0,		// red
					//	255,	// green
					//	0,		// blue,

						0,		// red
						255,	// green
						0,	// blue,

						128, // bright
						100  // speed
						);
		}

	//	ImmolationDamage( CTakeDamageInfo( this, this, 1, DMG_BURN ), tr.endpos, m_flBurnRadius, CLASS_NONE );
	//	ImmolationDamage( CTakeDamageInfo( this, this, 1, DMG_PLASMA ), tr.endpos, m_flBurnRadius, CLASS_NONE );
	}
	else
	{
		// The attack beam struck some kind of entity directly.
	}

	m_flTimeLastUpdatedRadius = gpGlobals->curtime;

/*	if( m_flBurnRadius >= MAX_BURN_RADIUS )
	{
		StopImmolating();
	}*/
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponImmolator::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();
}

// AARIKFIXME : Too tired right now fix this later.
#pragma warning(disable : 4706)

void CWeaponImmolator::ImmolationDamage( const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore )
{
	CBaseEntity *pEntity = NULL;
	trace_t		tr;
	Vector		vecSpot;

	Vector vecSrc = vecSrcIn;

	// iterate on all entities in the vicinity.
	for ( CEntitySphereQuery sphere( vecSrc, flRadius ); pEntity = sphere.GetCurrentEntity(); sphere.NextEntity() )
	{
		CBaseCombatCharacter *pBCC = pEntity->MyCombatCharacterPointer();
		if ( pBCC && pBCC->IsAlive() && !pBCC->IsOnFire() )
		{
			// UNDONE: this should check a damage mask, not an ignore
			if ( iClassIgnore != CLASS_NONE && pEntity->Classify() == iClassIgnore )
			{
				continue;
			}
		
			float lifetime = random->RandomFloat( 3, 5 );

			CEntityFlame *pFlame = CEntityFlame::Create( pBCC, true );
			if (pFlame)
			{
				pFlame->SetLifetime( lifetime );
				pBCC->AddFlag( FL_ONFIRE );
			}
		}
	}
}

void CWeaponImmolator::WeaponIdle( void )
{
	if ( !HasWeaponIdleTimeElapsed() )
		return;

	if ( m_fireState != FIRE_OFF )
		 StopImmolating();

	SendWeaponAnim( ACT_VM_IDLE );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponImmolator::SecondaryAttack( void )
{
//	FireBeam();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponImmolator::FireBeam( void )
{

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	pOwner->RumbleEffect( RUMBLE_357, 0, RUMBLE_FLAG_RESTART );

	Vector vecAiming	= pOwner->GetAutoaimVector( 0 );
	Vector vecSrc		= pOwner->Weapon_ShootPosition();

	QAngle angAiming;
	VectorAngles( vecAiming, angAiming );

//	Vector beamSrc;
//	CBaseViewModel *pViewModel = pOwner->GetViewModel();
//	pViewModel->GetAttachment( pViewModel->LookupAttachment( "muzzle" ), beamSrc );

	Vector	vForward, vRight, vUp;

	pOwner->EyeVectors( &vForward, &vRight, &vUp );

	Vector muzzlePoint = pOwner->Weapon_ShootPosition() + vForward * 27.0f + vRight * 7.5f + vUp * -5.0f;

	CImmolatorBeam *pBeam = CImmolatorBeam::BoltCreate(muzzlePoint, angAiming, pOwner);

//	if ( pOwner->GetWaterLevel() == 3 )
//	{
//		pBeam->SetAbsVelocity( vecAiming * BEAM_AIR_VELOCITY );
//	}
//	else
//	{
		pBeam->SetAbsVelocity( vecAiming * BEAM_AIR_VELOCITY );
//	}

	//epicplayer - cursed stuff incoming

	//m_iClip1--;

//	if (gpGlobals->curtime >= m_flAmmoUseTime)
	//	{
			UseAmmo(1);

			if (!HasAmmo())
			{
				StopImmolating();
			}

//			m_flAmmoUseTime = gpGlobals->curtime + 0.1;
//	}

//	pOwner->ViewPunch( QAngle( -2, 0, 0 ) );

//	WeaponSound( SINGLE );
//	WeaponSound( SPECIAL2 );

	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 200, 0.2 );

//	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	if ( !m_iClip1 && pOwner->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		// HEV suit - indicate out of ammo condition
		pOwner->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
	}

	m_flNextPrimaryAttack = m_flNextSecondaryAttack	= gpGlobals->curtime + 0.75;
}
