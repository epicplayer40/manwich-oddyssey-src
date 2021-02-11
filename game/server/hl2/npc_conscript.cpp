//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Formerly low-ranking expendable forces under the combine, the conscripts have rebelled against their oppressors.
// Uses npc_barney as a base. I know, shame on me.
// Todo: Find a way to make them throw grenades with a 1 in 6 chance of throwing them
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
#include "soundenvelope.h"
#include "game.h"
#include "npcevent.h"
#include "activitylist.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "sceneentity.h"
#include "ai_behavior_functank.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define CONSCRIPT_MODEL "models/conscript.mdl"

#define CONCEPT_PLAYER_USE "FollowWhenUsed"

ConVar	sk_conscript_health( "sk_conscript_health","50");

//=========================================================
//  Activities
//=========================================================

class CNPC_Conscript : public CNPC_PlayerCompanion
{
public:
	DECLARE_CLASS( CNPC_Conscript, CNPC_PlayerCompanion );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	virtual void Precache()
	{
		// Prevents a warning
		SelectModel( );
		BaseClass::Precache();

		PrecacheScriptSound( "NPC_Barney.FootstepLeft" );
		PrecacheScriptSound( "NPC_Barney.FootstepRight" );
		PrecacheScriptSound( "NPC_Conscript.Death" );
		PrecacheScriptSound( "NPC_Conscript.Pain" );
		PrecacheScriptSound( "NPC_Conscript.Alert" );

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

	void PainSound( const CTakeDamageInfo &info );
	void DeathSound( const CTakeDamageInfo &info );
	void AlertSound( void );

	void GatherConditions();
	void UseFunc( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	void FollowUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	CAI_FuncTankBehavior		m_FuncTankBehavior;
	COutputEvent				m_OnPlayerUse;

	DEFINE_CUSTOM_AI;
};


LINK_ENTITY_TO_CLASS( npc_conscript, CNPC_Conscript );

//---------------------------------------------------------
// 
//---------------------------------------------------------
//IMPLEMENT_CUSTOM_AI( npc_conscript,CNPC_Conscript );
IMPLEMENT_SERVERCLASS_ST(CNPC_Conscript, DT_NPC_Conscript)
END_SEND_TABLE()


//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_Conscript )
//						m_FuncTankBehavior
	DEFINE_OUTPUT( m_OnPlayerUse, "OnPlayerUse" ),


END_DATADESC()

//--------------------------------------------------------------------------------------------------
// Purpose: Sets the model, I plan to give the cons random models like with the citizens eventually.
//--------------------------------------------------------------------------------------------------
void CNPC_Conscript::SelectModel()
{
	SetModelName( AllocPooledString( CONSCRIPT_MODEL ) );
}




//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Conscript::Spawn( void )
{
	Precache();

	m_iHealth = sk_conscript_health.GetFloat();;

	m_iszIdleExpression = MAKE_STRING("scenes/Expressions/CitizenIdle.vcd");
	m_iszAlertExpression = MAKE_STRING("scenes/Expressions/citizenalert_loop.vcd");
	m_iszCombatExpression = MAKE_STRING("scenes/Expressions/BarneyCombat.vcd");

	BaseClass::Spawn();

	AddEFlags( EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL | EFL_NO_PHYSCANNON_INTERACTION );

	NPCInit();

	SetUse( &CNPC_Conscript::FollowUse );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
Class_T	CNPC_Conscript::Classify( void )
{
	return	CLASS_CONSCRIPT;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Conscript::Weapon_Equip( CBaseCombatWeapon *pWeapon )
{
	BaseClass::Weapon_Equip( pWeapon );

	if( hl2_episodic.GetBool() && FClassnameIs( pWeapon, "weapon_oicw" ) )
	{
		// Allow These guys to defend themselves at point-blank range.
		pWeapon->m_fMinRange1 = 0.0f;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Conscript::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
	case NPC_EVENT_LEFTFOOT:
		{
			EmitSound( "NPC_Barney.FootstepLeft", pEvent->eventtime );
		}
		break;
	case NPC_EVENT_RIGHTFOOT:
		{
			EmitSound( "NPC_Barney.FootstepRight", pEvent->eventtime );
		}
		break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CNPC_Conscript::PainSound( const CTakeDamageInfo &info )
{
	//Play the unique sound when on fire. It would take a real hard-ass to just go "ah. ugh. oof!" while they are being burned alive.
	if ( IsOnFire() )
	{
		EmitSound( "npc_conscript.DeathFire" );
		return;
	}

//	EmitSound( "npc_conscript.Pain" );
}

void CNPC_Conscript::DeathSound( const CTakeDamageInfo &info )
{
	// Sentences don't play on dead NPCs
	SentenceStop();

	EmitSound( "npc_conscript.Death" );

}

void CNPC_Conscript::AlertSound( void )
{
	EmitSound( "npc_conscript.Alert" );
}



bool CNPC_Conscript::CreateBehaviors( void )
{
	BaseClass::CreateBehaviors();
	AddBehavior( &m_FuncTankBehavior );
	AddBehavior( &m_FollowBehavior );

	return true;
}

bool CNPC_Conscript::OnBehaviorChangeStatus( CAI_BehaviorBase *pBehavior, bool fCanFinishSchedule )
{
	bool interrupt = BaseClass::OnBehaviorChangeStatus( pBehavior, fCanFinishSchedule );
	if ( !interrupt )
	{
		interrupt = ( pBehavior == &m_FollowBehavior && ( m_NPCState == NPC_STATE_IDLE || m_NPCState == NPC_STATE_ALERT ) );
	}
	return interrupt;
}

void CNPC_Conscript::OnChangeRunningBehavior( CAI_BehaviorBase *pOldBehavior,  CAI_BehaviorBase *pNewBehavior )
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
void CNPC_Conscript::GatherConditions()
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
void CNPC_Conscript::UseFunc( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	m_bDontUseSemaphore = true;
	SpeakIfAllowed( TLK_USE );
	m_bDontUseSemaphore = false;

	m_OnPlayerUse.FireOutput( pActivator, pCaller );
}

void CNPC_Conscript::FollowUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	Speak( CONCEPT_PLAYER_USE );

	CBaseEntity *pLeader = ( !m_FollowBehavior.GetFollowTarget() ) ? pCaller : NULL;
	m_FollowBehavior.SetFollowTarget( pLeader );

	if ( m_pSquad && m_pSquad->NumMembers() > 1 )
	{
		AISquadIter_t iter;
		for (CAI_BaseNPC *pSquadMember = m_pSquad->GetFirstMember( &iter ); pSquadMember; pSquadMember = m_pSquad->GetNextMember( &iter ) )
		{
			CNPC_Conscript *pBarney = dynamic_cast<CNPC_Conscript *>(pSquadMember);
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

AI_BEGIN_CUSTOM_NPC( npc_conscript, CNPC_Conscript )

AI_END_CUSTOM_NPC()
