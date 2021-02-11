//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Some dirty old asshole who was crusty and nasty enough for Valve to not like him so he was replaced with Kleiner.
//
//=============================================================================//

#include "cbase.h"
#include "ai_default.h"
#include "ai_task.h"
#include "ai_schedule.h"
#include "ai_node.h"
#include "ai_hull.h"
#include "ai_hint.h"
#include "ai_squad.h"
#include "ai_senses.h"
#include "ai_navigator.h"
#include "ai_motor.h"
#include "ai_behavior.h"
#include "ai_baseactor.h"
#include "ai_behavior_lead.h"
#include "ai_behavior_follow.h"
#include "ai_behavior_standoff.h"
#include "ai_behavior_assault.h"
#include "npc_playercompanion.h"
#include "soundent.h"
#include "game.h"
#include "npcevent.h"
#include "activitylist.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "sceneentity.h"
#include "ai_behavior_functank.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define COHRT_MODEL "models/leakcohrt.mdl"

#define CONCEPT_PLAYER_USE "FollowWhenUsed"

ConVar	sk_cohrt_health( "sk_cohrt_health","30");

//=========================================================
//  Activities
//=========================================================

class CNPC_Cohrt : public CNPC_PlayerCompanion
{
public:
	DECLARE_CLASS( CNPC_Cohrt, CNPC_PlayerCompanion );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	virtual void Precache()
	{
		// Prevents a warning
		SelectModel( );
		BaseClass::Precache();

		PrecacheScriptSound( "NPC_Barney.FootstepLeft" );
		PrecacheScriptSound( "NPC_Barney.FootstepRight" );
		PrecacheScriptSound( "NPC_cohrt.Die" );

		PrecacheInstancedScene( "scenes/Expressions/BarneyIdle.vcd" );
		PrecacheInstancedScene( "scenes/Expressions/BarneyAlert.vcd" );
		PrecacheInstancedScene( "scenes/Expressions/BarneyCombat.vcd" );
	}

	void	Spawn( void );
	void	SelectModel();
	Class_T Classify( void );
	void	Weapon_Equip( CBaseCombatWeapon *pWeapon );

	bool CreateBehaviors( void );

	void HandleAnimEvent( animevent_t *pEvent );

	bool ShouldLookForBetterWeapon() { return false; }

	void OnChangeRunningBehavior( CAI_BehaviorBase *pOldBehavior,  CAI_BehaviorBase *pNewBehavior );

	virtual bool OnBehaviorChangeStatus( CAI_BehaviorBase *pBehavior, bool fCanFinishSchedule );

	void DeathSound( const CTakeDamageInfo &info );
	void GatherConditions();
	void UseFunc( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	void FollowUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	CAI_FuncTankBehavior		m_FuncTankBehavior;
	COutputEvent				m_OnPlayerUse;

	DEFINE_CUSTOM_AI;
};


LINK_ENTITY_TO_CLASS( npc_cohrt, CNPC_Cohrt );

//---------------------------------------------------------
// 
//---------------------------------------------------------
//IMPLEMENT_CUSTOM_AI( npc_cohrt,CNPC_Cohrt );
IMPLEMENT_SERVERCLASS_ST(CNPC_Cohrt, DT_NPC_Cohrt)
END_SEND_TABLE()


//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_Cohrt )
//						m_FuncTankBehavior
	DEFINE_OUTPUT( m_OnPlayerUse, "OnPlayerUse" ),


END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Cohrt::SelectModel()
{
	SetModelName( AllocPooledString( COHRT_MODEL ) );
}




//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Cohrt::Spawn( void )
{
	Precache();

	m_iHealth = sk_cohrt_health.GetFloat();;

	m_iszIdleExpression = MAKE_STRING("scenes/Expressions/CitizenIdle.vcd");
	m_iszAlertExpression = MAKE_STRING("scenes/Expressions/citizenalert_loop.vcd");
	m_iszCombatExpression = MAKE_STRING("scenes/Expressions/BarneyCombat.vcd");

	BaseClass::Spawn();

	AddEFlags( EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL | EFL_NO_PHYSCANNON_INTERACTION );

	NPCInit();

	SetUse( &CNPC_Cohrt::FollowUse );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
Class_T	CNPC_Cohrt::Classify( void )
{
	return	CLASS_CONSCRIPT;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Cohrt::Weapon_Equip( CBaseCombatWeapon *pWeapon )
{
	BaseClass::Weapon_Equip( pWeapon );

	if( hl2_episodic.GetBool() && FClassnameIs( pWeapon, "weapon_*" ) )
	{
		// Allow These guys to defend themselves at point-blank range.
		pWeapon->m_fMinRange1 = 0.0f;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Cohrt::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
	case NPC_EVENT_LEFTFOOT:
		{
			EmitSound( "NPC_Citizen.FootstepLeft", pEvent->eventtime );
		}
		break;
	case NPC_EVENT_RIGHTFOOT:
		{
			EmitSound( "NPC_Citizen.FootstepRight", pEvent->eventtime );
		}
		break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_Cohrt::DeathSound( const CTakeDamageInfo &info )
{
	// Sentences don't play on dead NPCs
	SentenceStop();

	EmitSound( "npc_cohrt.die" );

}

bool CNPC_Cohrt::CreateBehaviors( void )
{
	BaseClass::CreateBehaviors();
	AddBehavior( &m_FuncTankBehavior );
	AddBehavior( &m_FollowBehavior );

	return true;
}

bool CNPC_Cohrt::OnBehaviorChangeStatus( CAI_BehaviorBase *pBehavior, bool fCanFinishSchedule )
{
	bool interrupt = BaseClass::OnBehaviorChangeStatus( pBehavior, fCanFinishSchedule );
	if ( !interrupt )
	{
		interrupt = ( pBehavior == &m_FollowBehavior && ( m_NPCState == NPC_STATE_IDLE || m_NPCState == NPC_STATE_ALERT ) );
	}
	return interrupt;
}

void CNPC_Cohrt::OnChangeRunningBehavior( CAI_BehaviorBase *pOldBehavior,  CAI_BehaviorBase *pNewBehavior )
{
	if ( pNewBehavior == &m_FuncTankBehavior )
	{
		m_bReadinessCapable = false;
	}
	else if ( pOldBehavior == &m_FuncTankBehavior )
	{
		m_bReadinessCapable = IsReadinessCapable();
	}

	BaseClass::OnChangeRunningBehavior( pOldBehavior, pNewBehavior );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Cohrt::GatherConditions()
{
	BaseClass::GatherConditions();

	// Handle speech AI. Don't do AI speech if we're in scripts unless permitted by the EnableSpeakWhileScripting input.
	if ( m_NPCState == NPC_STATE_IDLE || m_NPCState == NPC_STATE_ALERT || m_NPCState == NPC_STATE_COMBAT ||
		( ( m_NPCState == NPC_STATE_SCRIPT ) && CanSpeakWhileScripting() ) )
	{
		DoCustomSpeechAI();
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Cohrt::UseFunc( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	m_bDontUseSemaphore = true;
	SpeakIfAllowed( TLK_USE );
	m_bDontUseSemaphore = false;

	m_OnPlayerUse.FireOutput( pActivator, pCaller );
}

void CNPC_Cohrt::FollowUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	Speak( CONCEPT_PLAYER_USE );

	CBaseEntity *pLeader = ( !m_FollowBehavior.GetFollowTarget() ) ? pCaller : NULL;
	m_FollowBehavior.SetFollowTarget( pLeader );

	if ( m_pSquad && m_pSquad->NumMembers() > 1 )
	{
		AISquadIter_t iter;
		for (CAI_BaseNPC *pSquadMember = m_pSquad->GetFirstMember( &iter ); pSquadMember; pSquadMember = m_pSquad->GetNextMember( &iter ) )
		{
			CNPC_Cohrt *pBarney = dynamic_cast<CNPC_Cohrt *>(pSquadMember);
			if ( pBarney && pBarney != this)
			{
				pBarney->m_FollowBehavior.SetFollowTarget( pLeader );
			}
		}

	}
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_cohrt, CNPC_Cohrt )

AI_END_CUSTOM_NPC()
