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
#include "movevars_shared.h"

#include "weapon_immolator.h"

ConVar	sk_plr_dmg_immolator("sk_plr_dmg_immolator", "0");
ConVar	sk_npc_dmg_immolator("sk_npc_dmg_immolator", "0");
ConVar	sk_plr_burn_duration_immolator("sk_plr_burn_duration_immolator", "0");
ConVar	sk_npc_burn_duration_immolator("sk_npc_burn_duration_immolator", "0");
ConVar	sk_immolator_gravity("sk_immolator_gravity", "0");
ConVar	sk_immolator_speed("sk_immolator_speed", "0");

extern ConVar sk_cremator_maxrange;
extern ConVar sk_cremator_minrange;

ConVar immolator_debug("immolator_debug", "0");

#define MAX_BURN_RADIUS		256
#define RADIUS_GROW_RATE	50.0	// units/sec 

#define IMMOLATOR_TARGET_INVALID Vector( FLT_MAX, FLT_MAX, FLT_MAX )

#define BEAM_MODEL	"models/crossbow_bolt.mdl"

//#define CREMATOR_AIM_HACK

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

	Vector startpos, endpos;

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
	pBeam->startpos = vecOrigin;
//	pBeam->SetGravity(sk_gravity_immolator.GetFloat());
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
	SetSolidFlags(FSOLID_NOT_SOLID | FSOLID_TRIGGER);
	SetGravity(sk_immolator_gravity.GetFloat());
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
		if ( pBCC && pBCC->IsAlive() && !pBCC->IsOnFire() && pBCC->IsImmolatable() )
		{
			// UNDONE: this should check a damage mask, not an ignore
			if ( iClassIgnore != CLASS_NONE && pEntity->Classify() == iClassIgnore )
			{
				continue;
			}
		
			float lifetime;
			if (pBCC->IsPlayer())
			{
				lifetime = sk_npc_burn_duration_immolator.GetFloat(); //How long the player should burn, ask epicplayer why the convar is named this
			}
			else
			{
				lifetime = sk_plr_burn_duration_immolator.GetFloat();
			}

			CEntityFlame* pFlame = CEntityFlame::Create(pBCC, true, true);
			if (pFlame)
			{
				pFlame->SetLifetime( lifetime );
				pBCC->AddFlag( FL_ONFIRE );
				pBCC->SetEffectEntity(pFlame);
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

	if (GetOwnerEntity() == pOther)
		return;
		
	if ( pOther->IsSolidFlagSet(FSOLID_VOLUME_CONTENTS | FSOLID_TRIGGER) )
	{
		// Some NPCs are triggers that can take damage (like antlion grubs). We should hit them.
		if ( ( pOther->m_takedamage == DAMAGE_NO ) || ( pOther->m_takedamage == DAMAGE_EVENTS_ONLY ) )
			return;
	}
	float damage;
	if (pOther->IsPlayer())
	{
		damage = sk_plr_dmg_immolator.GetFloat();
	}
	else
	{
		damage = sk_npc_dmg_immolator.GetFloat();
	}
	//Lychy: Decided to remove DMG_BURN since it created a red fade that didnt work well with the blu one
	RadiusDamage( CTakeDamageInfo( this, GetOwnerEntity(), damage, DMG_DISSOLVE | DMG_PLASMA /* | DMG_BURN*/), GetAbsOrigin(), 100, CLASS_PLAYER_ALLY_VITAL, NULL); //changed from 256 to 128 to correspond with noisebeams
	ImmolationDamage(CTakeDamageInfo(this, GetOwnerEntity(), 1, DMG_PLASMA), GetAbsOrigin(), 100, CLASS_PLAYER_ALLY_VITAL);
	if (immolator_debug.GetBool())
	{
		Vector pos = GetAbsOrigin();
		pos.z = 0;
		Vector delta = pos - startpos;
		delta.z = 0;
		Msg("End point: %f %f\n", pos.x, pos.y);
		Msg("Delta: %f %f\n", delta.x, delta.y);
		Msg("Length: %f\n", delta.Length());

	}

	UTIL_Remove(this);
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
	DEFINE_FIELD (m_bUsingAnimEvents, FIELD_BOOLEAN ),

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


void CWeaponImmolator::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	int event = pEvent->event;
	switch (event)
	{
	case EVENT_WEAPON_SMG1:
		{
			m_iPrimaryAmmoType = -1;
			m_bUsingAnimEvents = true;
			m_flStartFireTime = gpGlobals->curtime;
			StartImmolating();
		}
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
	pBeamer->SetAbsVelocity(muzzlePoint * GetBeamVelocity());

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
	CBaseCombatCharacter *pOwner = ToBaseCombatCharacter( GetOwner() );
	if ( pOwner == NULL )
		return;

	if ( pOwner->GetAmmoCount( m_iPrimaryAmmoType ) >= count )
		pOwner->RemoveAmmo( count, m_iPrimaryAmmoType );
	else
		pOwner->RemoveAmmo( pOwner->GetAmmoCount( m_iPrimaryAmmoType ), m_iPrimaryAmmoType );
}

void CWeaponImmolator::StartImmolating()
{
	if (IsImmolating())
	{
		return;
	}
	EmitSound("Weapon_Immolator.Run");
	m_flBurnRadius = 0.1;
	m_flTimeLastUpdatedRadius = gpGlobals->curtime;
	SetThink( &CWeaponImmolator::UpdateThink );
	SetNextThink( gpGlobals->curtime );
	m_fireState = FIRE_CHARGE;
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

void CWeaponImmolator::Spawn(void)
{
	BaseClass::Spawn();

	if (m_nMuzzleFlashAttachment == 0)
	{
		m_nMuzzleFlashAttachment = LookupAttachment("muzzle");
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponImmolator::PrimaryAttack( void )
{
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
			if ( gpGlobals->curtime >= ( m_flStartFireTime + 0.3 ) )
			{
				StartImmolating();
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

	// Ignore vertical distance when doing our RPG distance calculations
	CAI_BaseNPC *pNPC = GetOwner()->MyNPCPointer();
	if (pNPC)
	{
		CBaseEntity *pEnemy = pNPC->GetEnemy();
		Vector vecToTarget = (pEnemy->GetAbsOrigin() - pNPC->GetAbsOrigin());
		vecToTarget.z = 0;
		flDist = vecToTarget.Length();
	}
	
	if (flDist < m_fMinRange1)
		return COND_TOO_CLOSE_TO_ATTACK;

	//if (m_flNextPrimaryAttack > gpGlobals->curtime)
	//	return 0;

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

	if (m_bUsingAnimEvents && gpGlobals->curtime > m_flStartFireTime + 1.0f)
	{
		StopImmolating();
		return;
	}

	FireBeam();
	SetNextThink( gpGlobals->curtime + 0.05 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponImmolator::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();
}

void CWeaponImmolator::ImmolationDamage( const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore )
{
	CBaseEntity *pEntity = NULL;
	trace_t		tr;
	Vector		vecSpot;

	Vector vecSrc = vecSrcIn;

	// iterate on all entities in the vicinity.
	for ( CEntitySphereQuery sphere( vecSrc, flRadius ); (pEntity = sphere.GetCurrentEntity()) != 0; sphere.NextEntity() )
	{
		CBaseCombatCharacter *pBCC = pEntity->MyCombatCharacterPointer();
		if ( pBCC && pBCC->IsAlive() && !pBCC->IsOnFire() && pBCC->IsImmolatable() )
		{
			// UNDONE: this should check a damage mask, not an ignore
			if ( iClassIgnore != CLASS_NONE && pEntity->Classify() == iClassIgnore )
			{
				continue;
			}
		
			float lifetime;
			if (pBCC->IsPlayer())
			{
				lifetime = sk_plr_burn_duration_immolator.GetFloat();
			}
			else
			{
				lifetime = sk_npc_burn_duration_immolator.GetFloat();
			}

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
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponImmolator::FireBeam( void )
{
	CBaseCombatCharacter* pOwner = GetOwner();
	CBasePlayer* pPlayer = NULL;
	if (pOwner->IsPlayer())
	{
		pPlayer = static_cast<CBasePlayer*>(pOwner);
	}
	
	if (pOwner == NULL)
		return;

	if(pPlayer)
		pPlayer->RumbleEffect(RUMBLE_357, 0, RUMBLE_FLAG_RESTART);

	int iMuzzleAttachment = LookupAttachment("muzzle");
	//Vector vecAiming = pPlayer->GetAutoaimVector(0);
	QAngle angEyes;
	Vector vecSrc;
	if (pPlayer)
	{
		vecSrc = pPlayer->Weapon_ShootPosition();
		angEyes = pPlayer->EyeAngles();
	}
	else
	{
		GetAttachment(iMuzzleAttachment, vecSrc, angEyes);
	}

	Vector	vForward, vRight, vUp;
	AngleVectors(angEyes, &vForward, &vRight, &vUp);

	Vector muzzlePoint = vecSrc;
	if (pPlayer)
	{
		muzzlePoint += vForward * 27.0f + vRight * 7.5f + vUp * -5.0f;
	}
	CImmolatorBeam* pBeam = CImmolatorBeam::BoltCreate(muzzlePoint, angEyes, pOwner);

	Vector velocity = vForward * GetBeamVelocity();
	pBeam->SetAbsVelocity(velocity);

	UseAmmo(1);

	if (pPlayer && !HasAmmo())
	{
		StopImmolating();
	}
	CSoundEnt::InsertSound(SOUND_COMBAT, GetAbsOrigin(), 200, 0.2);

	if (pPlayer && !m_iClip1 && pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
	}

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + 0.75;
}

float CWeaponImmolator::GetBeamVelocity() const
{
	return sk_immolator_speed.GetFloat();
}

void CWeaponImmolator::CalculateMaxDistance()
{
	m_fMinRange1 = sk_cremator_minrange.GetFloat();
	m_fMinRange2 = sk_cremator_minrange.GetFloat();
	m_fMaxRange1 = sk_cremator_maxrange.GetFloat();
	m_fMaxRange2 = sk_cremator_maxrange.GetFloat();
}

void CWeaponImmolator::StopLoopingSounds()
{
	StopSound("Weapon_Immolator.Run");
	BaseClass::StopLoopingSounds();
}
