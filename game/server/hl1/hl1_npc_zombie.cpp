//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: A slow-moving, once-human headcrab victim with only melee attacks.
//
// UNDONE: Make head take 100% damage, body take 30% damage.
// UNDONE: Don't flinch every time you get hit.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "game.h"
#include "AI_Default.h"
#include "AI_Schedule.h"
#include "AI_Hull.h"
#include "AI_Route.h"
#include "NPCEvent.h"
#include "hl1_npc_zombie.h"
#include "gib.h"
#include "props.h"
#include "particle_parse.h"
//#include "AI_Interactions.h"
#include "ndebugoverlay.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"

ConVar	sk_zombiehl1_health( "sk_zombiehl1_health","95");
ConVar  sk_zombiehl1_dmg_one_slash( "sk_zombiehl1_dmg_one_slash", "20" );
ConVar  sk_zombiehl1_dmg_both_slash( "sk_zombiehl1_dmg_both_slash", "40" );



LINK_ENTITY_TO_CLASS( monster_zombie, CNPC_Zombie );


//=========================================================
// Spawn
//=========================================================
void CNPC_Zombie::Spawn()
{
	Precache( );

	SetModel( "models/zombie.mdl" );
	
	SetRenderColor( 255, 255, 255, 255 );
	
	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	m_bloodColor		= BLOOD_COLOR_GREEN;
    m_iHealth			= sk_zombiehl1_health.GetFloat();
	//pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;
	m_NPCState			= NPC_STATE_NONE;
	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_MOVE_CLIMB | bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_DOORS_GROUP );

	NPCInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CNPC_Zombie::Precache()
{
	PrecacheModel( "models/zombie.mdl" );

	PrecacheScriptSound( "Zombie.AttackHit" );
	PrecacheScriptSound( "Zombie.AttackMiss" );
	PrecacheScriptSound( "Zombie.Pain" );
	PrecacheScriptSound( "Zombie.Idle" );
	PrecacheScriptSound( "Zombie.Alert" );
	PrecacheScriptSound( "Zombie.Attack" );
	PrecacheScriptSound( "BaseCombatCharacter.FleshGib" );

//	PrecacheModel("models/zombie/classic_torso.mdl");
	PrecacheModel("models/gibs/zombie_legs.mdl");
	PrecacheModel("models/gibs/zombie_lleg.mdl");
	PrecacheModel("models/gibs/zombie_rleg.mdl");
	PrecacheModel("models/gibs/zombie_lleg_static.mdl");
	PrecacheModel("models/gibs/zombie_rleg_static.mdl");

	PrecacheModel("models/gibs/zombie_lhand.mdl");
	PrecacheModel("models/gibs/zombie_larm.mdl");
	PrecacheModel("models/gibs/zombie_rarm.mdl");

	PrecacheModel("models/gibs/zombie_head.mdl");
	PrecacheModel("models/gibs/zombie_torso.mdl");
	PrecacheModel("models/gibs/zombie_organ.mdl");

	PrecacheParticleSystem( "blood_zombie_split" );
//	PrecacheParticleSystem( "blood_zombie_split_spray" ); these particles don't parent properly, disabling - epicplayer
//	PrecacheParticleSystem( "blood_zombie_split_spray_tiny2" );

	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: Returns this monster's place in the relationship table.
//-----------------------------------------------------------------------------
Class_T	CNPC_Zombie::Classify( void )
{
	return CLASS_ZOMBIE; 
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CNPC_Zombie::HandleAnimEvent( animevent_t *pEvent )
{
	Vector v_forward, v_right;
	switch( pEvent->event )
	{
		case ZOMBIE_AE_ATTACK_RIGHT:
		{
			// do stuff for this event.
	//		ALERT( at_console, "Slash right!\n" );

			Vector vecMins = GetHullMins();
			Vector vecMaxs = GetHullMaxs();
			vecMins.z = vecMins.x;
			vecMaxs.z = vecMaxs.x;

			CBaseEntity *pHurt = CheckTraceHullAttack( 70, vecMins, vecMaxs, sk_zombiehl1_dmg_one_slash.GetFloat(), DMG_SLASH );
			CPASAttenuationFilter filter( this );
			if ( pHurt )
			{
				if ( pHurt->GetFlags() & ( FL_NPC | FL_CLIENT ) )
				{
					pHurt->ViewPunch( QAngle( 5, 0, 18 ) );
					
					GetVectors( &v_forward, &v_right, NULL );

					pHurt->SetAbsVelocity( pHurt->GetAbsVelocity() - v_right * 100 );
				}
				// Play a random attack hit sound
				EmitSound( filter, entindex(), "Zombie.AttackHit" );
			}
			else // Play a random attack miss sound
				EmitSound( filter, entindex(), "Zombie.AttackMiss" );

			if ( random->RandomInt( 0, 1 ) )
				 AttackSound();
		}
		break;

		case ZOMBIE_AE_ATTACK_LEFT:
		{
			// do stuff for this event.
	//		ALERT( at_console, "Slash left!\n" );
			Vector vecMins = GetHullMins();
			Vector vecMaxs = GetHullMaxs();
			vecMins.z = vecMins.x;
			vecMaxs.z = vecMaxs.x;

			CBaseEntity *pHurt = CheckTraceHullAttack( 70, vecMins, vecMaxs, sk_zombiehl1_dmg_one_slash.GetFloat(), DMG_SLASH );
			
			CPASAttenuationFilter filter2( this );
			if ( pHurt )
			{
				if ( pHurt->GetFlags() & ( FL_NPC | FL_CLIENT ) )
				{
					pHurt->ViewPunch( QAngle ( 5, 0, -18 ) );
					
					GetVectors( &v_forward, &v_right, NULL );

					pHurt->SetAbsVelocity( pHurt->GetAbsVelocity() - v_right * 100 );
				}
				EmitSound( filter2, entindex(), "Zombie.AttackHit" );
			}
			else
			{
				EmitSound( filter2, entindex(), "Zombie.AttackMiss" );
			}

			if ( random->RandomInt( 0,1 ) ) 
				 AttackSound();
		}
		break;

		case ZOMBIE_AE_ATTACK_BOTH:
		{
			// do stuff for this event.
			Vector vecMins = GetHullMins();
			Vector vecMaxs = GetHullMaxs();
			vecMins.z = vecMins.x;
			vecMaxs.z = vecMaxs.x;

			CBaseEntity *pHurt = CheckTraceHullAttack( 70, vecMins, vecMaxs, sk_zombiehl1_dmg_both_slash.GetFloat(), DMG_SLASH );
			

			CPASAttenuationFilter filter3( this );
			if ( pHurt )
			{
				if ( pHurt->GetFlags() & ( FL_NPC | FL_CLIENT ) )
				{
					pHurt->ViewPunch( QAngle ( 5, 0, 0 ) );
					
					GetVectors( &v_forward, &v_right, NULL );
					pHurt->SetAbsVelocity( pHurt->GetAbsVelocity() - v_right * 100 );
				}
				EmitSound( filter3, entindex(), "Zombie.AttackHit" );
			}
			else
				EmitSound( filter3, entindex(),"Zombie.AttackMiss" );

			if ( random->RandomInt( 0,1 ) )
				 AttackSound();
		}
		break;

		default:
			BaseClass::HandleAnimEvent( pEvent );
			break;
	}
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


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pInflictor - 
//			pAttacker - 
//			flDamage - 
//			bitsDamageType - 
// Output : int
//-----------------------------------------------------------------------------
int CNPC_Zombie::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
{
	CTakeDamageInfo info = inputInfo;

	// Take 30% damage from bullets
	if ( info.GetDamageType() == DMG_BULLET )
	{
		Vector vecDir = GetAbsOrigin() - info.GetInflictor()->WorldSpaceCenter();
		VectorNormalize( vecDir );
		float flForce = DamageForce( WorldAlignSize(), info.GetDamage() );
		SetAbsVelocity( GetAbsVelocity() + vecDir * flForce );
		info.ScaleDamage( 0.3f );
	}

	// HACK HACK -- until we fix this.
	if ( IsAlive() )
		 PainSound( info );
	
	return BaseClass::OnTakeDamage_Alive( info );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Zombie::ShouldGib(const CTakeDamageInfo &info)
{
	// If we're being hoisted, we only want to gib when the barnacle hurts us with his bite!
	if ( IsEFlagSet( EFL_IS_BEING_LIFTED_BY_BARNACLE ) )
	{
		if ( info.GetAttacker() && info.GetAttacker()->Classify() != CLASS_BARNACLE )
			return false;

		return true;
	}

	if ( info.GetDamageType() & (DMG_NEVERGIB|DMG_DISSOLVE) )
		return false;

	if ( info.GetDamageType() & (DMG_ALWAYSGIB|DMG_BLAST) )
		return true;

	if ( m_iHealth < -20 )
		return true;
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Zombie::CorpseGib(const CTakeDamageInfo &info)
{

	// Use the bone position to handle being moved by an animation (like a dynamic scripted sequence)
	static int s_nBodyBone = -1;
	if ( s_nBodyBone == -1 )
	{
		s_nBodyBone = LookupBone( "Bip01 Pelvis" );
	}

	Vector vecOrigin;
	QAngle angBone;
	GetBonePosition( s_nBodyBone, vecOrigin, angBone );

	DispatchParticleEffect( "blood_zombie_split", vecOrigin, QAngle( 0, 0, 0 ) );
//	CPASAttenuationFilter filter( this );
//	EmitSound( filter, entindex(), "BaseCombatCharacter.FleshGib" );
	EmitSound( "BaseCombatCharacter.FleshGib" );

	Vector velocity = vec3_origin;
	AngularImpulse	angVelocity = RandomAngularImpulse( -150, 150 );
	breakablepropparams_t params( EyePosition(), GetAbsAngles(), velocity, angVelocity );
	params.impactEnergyScale = 1.0f;
	params.defBurstScale = 150.0f;
	params.defCollisionGroup = COLLISION_GROUP_DEBRIS;
	PropBreakableCreateAll( GetModelIndex(), NULL, params, this, -1, true, true );

//	gm_nTopGunAttachment = LookupAttachment( "top_eye" );
	/*
	CBaseEntity *pGib = CreateRagGib( "models/zombie/classic_torso.mdl", GetAbsOrigin(), GetAbsAngles(), velocity, 10.0f );
	DispatchParticleEffect( "blood_zombie_split_spray", PATTACH_ABSORIGIN_FOLLOW, pGib );
	DispatchParticleEffect( "blood_zombie_split_spray", PATTACH_POINT_FOLLOW, pGib, 1 );
	DispatchParticleEffect("blood_zombie_split_spray", PATTACH_ROOTBONE_FOLLOW, pGib);

	// don't collide with this thing ever
	if ( pGib )
	{
		pGib->SetOwnerEntity( this );
	}	
	*/
	return true;
}

void CNPC_Zombie::PainSound( const CTakeDamageInfo &info )
{
	if ( random->RandomInt(0,5) < 2)
	{
		CPASAttenuationFilter filter( this );
		EmitSound( filter, entindex(), "Zombie.Pain" );
	}
}

void CNPC_Zombie::AlertSound( void )
{
	CPASAttenuationFilter filter( this );
	EmitSound( filter, entindex(), "Zombie.Alert" );
}

void CNPC_Zombie::IdleSound( void )
{
	// Play a random idle sound
	CPASAttenuationFilter filter( this );
	EmitSound( filter, entindex(), "Zombie.Idle" );
}

void CNPC_Zombie::AttackSound( void )
{
	// Play a random attack sound
	CPASAttenuationFilter filter( this );
	EmitSound( filter, entindex(), "Zombie.Attack" );
}

int CNPC_Zombie::MeleeAttack1Conditions ( float flDot, float flDist )
{
	if ( flDist > 64)
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	else if (flDot < 0.7)
	{
		return 0;
	}
	else if (GetEnemy() == NULL)
	{
		return 0;
	}

	return COND_CAN_MELEE_ATTACK1;
}

void CNPC_Zombie::RemoveIgnoredConditions ( void )
{
	if ( GetActivity() == ACT_MELEE_ATTACK1 )
	{
		// Nothing stops an attacking zombie.
		ClearCondition( COND_LIGHT_DAMAGE );
		ClearCondition( COND_HEAVY_DAMAGE );
	}

	if (( GetActivity() == ACT_SMALL_FLINCH ) || ( GetActivity() == ACT_BIG_FLINCH ))
	{
		if (m_flNextFlinch < gpGlobals->curtime)
			m_flNextFlinch = gpGlobals->curtime + ZOMBIE_FLINCH_DELAY;
	}

	BaseClass::RemoveIgnoredConditions();
}