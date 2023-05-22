//========= Totally Not Copyright © 2023, Valve Cut Content, All rights reserved. ============//
//Who are combine janitor
//They are Well known as Cremators
//Their Job was Cleaning Human dead body and XEN creatures in streets with his flame thrower
//His head was made by Child labor without their willing
//Cremator are the Cut enemy from Half - life 2
//but, they still exist in half - life universe
//Cremator become famous among THE GAMERS
//So, Valve Decided to Bring back Cremator in Half - life alyx
//But, again Cremators was Cut From Game
//
//=============================================================================//
#include "cbase.h"
#include "ai_default.h"
#include "ai_task.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "ai_squad.h"
#include "soundent.h"
#include "game.h"
#include "npcevent.h"
#include "entitylist.h"
#include "activitylist.h"
#include "ai_basenpc.h"
#include "engine/IEngineSound.h"
#include "particle_parse.h"
#include "te_particlesystem.h"
#include "weapon_immolator.h"

#define CREMATOR_MODEL "models/cremator.mdl"
#define CREMATOR_MODEL_BODY "models/cremator_body.mdl" //The Cremator's head is gonna get blown off if the killing blow is a headshot - epicplayer
#define CREMATOR_MODEL_HEAD "models/cremator_head.mdl"

ConVar	sk_cremator_health( "sk_cremator_health","200" );
ConVar	sk_cremator_limp_health( "sk_cremator_limp_health", "100" );

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
// Private animevents
//=========================================================
int AE_CREMATOR_TURNONHOSE;
int AE_CREMATOR_TURNOFFHOSE;

//=========================================================
// Private activities
//=========================================================
Activity ACT_CREMATOR_GUN_JAM;

//=========================================================
//=========================================================
class CNPC_CrematorManod : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_CrematorManod, CAI_BaseNPC );
	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;

public:
	void	Precache( void );
	void	Spawn( void );
	Class_T Classify( void );

	void HandleAnimEvent( animevent_t *pEvent );
	int  OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo );
	void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );
	void PainSound(const CTakeDamageInfo &info);
	void DeathSound( const CTakeDamageInfo &info );
	Activity NPC_TranslateActivity( Activity eNewActivity );
	void PostNPCInit();
private:
	enum
	{
		SCHED_NEWNPC_SCHEDULE = BaseClass::NEXT_SCHEDULE,
		SCHED_NEWNPC_SCHEDULE2,
		NEXT_SCHEDULE
	};

	enum 
	{
		TASK_NEWNPC_TASK = BaseClass::NEXT_TASK,
		TASK_NEWNPC_TASK2,
		NEXT_TASK
	};

	enum 
	{
		COND_NEWNPC_CONDITION = BaseClass::NEXT_CONDITION,
		COND_NEWNPC_CONDITION2,
		NEXT_CONDITION
	};
};

LINK_ENTITY_TO_CLASS( npc_cremator, CNPC_CrematorManod );

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_CrematorManod )

END_DATADESC()

AI_BEGIN_CUSTOM_NPC(npc_cremator, CNPC_CrematorManod)

AI_END_CUSTOM_NPC()

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CNPC_CrematorManod::Precache( void )
{
	PrecacheModel( CREMATOR_MODEL );
	PrecacheModel( CREMATOR_MODEL_BODY );
	PrecacheModel( CREMATOR_MODEL_HEAD );
	PrecacheScriptSound( "NPC_Cremator.FootstepLeft" ); //footsteps
	PrecacheScriptSound( "NPC_Cremator.FootstepRight" );
	PrecacheScriptSound( "NPC_Cremator.CloakSwish" ); //triggered by anim events
	PrecacheScriptSound( "NPC_Cremator.Pain" );
	PrecacheScriptSound( "NPC_Cremator.Die" );
	PrecacheScriptSound( "NPC_Cremator.SeeEnemy" ); //see's hostile enemy
	PrecacheScriptSound( "NPC_Cremator.SeeObject" ); //target is a bullseye for scripted areas
	PrecacheScriptSound( "NPC_Cremator.Breathe" ); //constant breathing sound
	PrecacheScriptSound( "NPC_Cremator.Breathe_Mad" ); //breathing when limping
	PrecacheScriptSound( "Weapon_Immolator.Start" );
	PrecacheScriptSound( "Weapon_Immolator.Off" );
	PrecacheScriptSound( "Weapon_Immolator.Run" );
	PrecacheParticleSystem( "blood_impact_synth_01" );

	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CNPC_CrematorManod::Spawn( void )
{
	Precache();

	SetModel( CREMATOR_MODEL );
	SetHullType(HULL_MEDIUM);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	SetBloodColor( BLOOD_COLOR_RED );
	m_bloodColor = DONT_BLEED; //Emit Hunter blood - epicplayer
	m_iHealth	= sk_cremator_health.GetFloat();
	m_flFieldOfView	= 0.5; // indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState	= NPC_STATE_NONE;

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_TURN_HEAD | bits_CAP_MOVE_GROUND | bits_CAP_USE_WEAPONS | bits_CAP_AIM_GUN | bits_CAP_WEAPON_RANGE_ATTACK1 | bits_CAP_MOVE_SHOOT );

	NPCInit();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CrematorManod::PostNPCInit()
{
	CBaseCombatWeapon *pWeapon = GetActiveWeapon();
		// Give a warning if a Cremator is equipped with anything other than
		// an immolator. 
		if( !GetActiveWeapon() /*|| !FClassnameIs( GetActiveWeapon(), "weapon_immolator" )*/ )
		{
			DevWarning("**Cremator spawned without weapon, giving Immolator\n");
			pWeapon = (CBaseCombatWeapon *)CREATE_UNSAVED_ENTITY(CWeaponImmolator, "weapon_immolator");
			DispatchSpawn(pWeapon);
			pWeapon->Activate();
			Weapon_Equip(pWeapon);

		}

	BaseClass::PostNPCInit();
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
// Output : 
//-----------------------------------------------------------------------------
Class_T	CNPC_CrematorManod::Classify( void )
{
	return	CLASS_COMBINE;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_CrematorManod::HandleAnimEvent( animevent_t *pEvent )
{
	if (pEvent->event == AE_CREMATOR_TURNONHOSE) //Forcefully turn on the piss hose, have some kinda emergency fallback if the turnoff never triggers
	{
		EmitSound( "Weapon_Immolator.Start", pEvent->eventtime );
		return;
	}

	if (pEvent->event == AE_CREMATOR_TURNONHOSE) //Forcefully turn off immolator
	{
		EmitSound( "Weapon_Immolator.Off", pEvent->eventtime );
		return;
	}

	switch( pEvent->event )
	{
	case NPC_EVENT_LEFTFOOT:
		{
			EmitSound( "NPC_Cremator.FootstepLeft", pEvent->eventtime );
		}
		break;

	case NPC_EVENT_RIGHTFOOT:
		{
			EmitSound( "NPC_Cremator.FootstepRight", pEvent->eventtime );
		}
		break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// TakeDamage - get provoked when injured
//=========================================================

int CNPC_CrematorManod::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{

	int nDamageTaken = BaseClass::OnTakeDamage_Alive( info );


	return nDamageTaken;


	CTakeDamageInfo newInfo = info;
	if( newInfo.GetDamageType() & DMG_BURN)
	{
		newInfo.ScaleDamage( 0.0 );
		DevMsg( "Cremator is immune to fire damage!\n" ); //doesn't currently work, cremator shouldn't be ignitable but could be killed by direct plasma damage from other immolators - epicplayer
	}	

	if ( info.GetInflictor() && info.GetInflictor()->GetOwnerEntity() == this )
		return 0;
}


void CNPC_CrematorManod::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{

	if ( info.GetDamageType() & DMG_SHOCK )
	 return;

	// Even though the damage might not hurt us, we want to react to it
	// if it's from the player.
	if ( info.GetAttacker()->IsPlayer() )
	{
		if ( !HasMemory( bits_MEMORY_PROVOKED ) )
		{
			GetEnemies()->ClearMemory( info.GetAttacker() );
			Remember( bits_MEMORY_PROVOKED );
			PainSound(info);
			SetCondition( COND_HEAVY_DAMAGE );
		}
	}

	// Makes the Cremator emit hunter damage particles - epicplayer
	if ( ( info.GetDamageType() & DMG_BULLET ) ||
		 ( info.GetDamageType() & DMG_BUCKSHOT ) ||
		 ( info.GetDamageType() & DMG_CLUB ) ||
		 ( info.GetDamageType() & DMG_NEVERGIB ) )
	{
		QAngle vecAngles;
		VectorAngles( ptr->plane.normal, vecAngles );
		DispatchParticleEffect( "blood_impact_synth_01", ptr->endpos, vecAngles );
	}

	BaseClass::TraceAttack( info, vecDir, ptr, pAccumulator );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CrematorManod::PainSound( const CTakeDamageInfo &info )
{
	EmitSound( "NPC_Cremator.Pain" );
}

//=========================================================
// DieSound
//=========================================================

void CNPC_CrematorManod::DeathSound( const CTakeDamageInfo &info )
{
	EmitSound( "NPC_Cremator.Die" );
}

//=========================================================
// SetActivity 
//=========================================================
Activity CNPC_CrematorManod::NPC_TranslateActivity( Activity eNewActivity )
{
	switch (eNewActivity)
	{
	case ACT_RUN:
		if (m_iHealth <= sk_cremator_limp_health.GetFloat())
		{
			// limp!
			return ACT_RUN_HURT;
		}
		else
		{
			return eNewActivity;
		}
		break;
	case ACT_WALK:
		if (m_iHealth <= sk_cremator_limp_health.GetFloat())
		{
			// limp!
			return ACT_RUN_HURT;
		}
		else
		{
			return eNewActivity;
		}
		break;
	case ACT_IDLE:
		if (m_NPCState == NPC_STATE_COMBAT)
		{
			eNewActivity = ACT_IDLE_ANGRY;
		}

		break;
	}

	return BaseClass::NPC_TranslateActivity(eNewActivity);
}