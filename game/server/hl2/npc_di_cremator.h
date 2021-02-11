#ifndef NPC_DI_CREMATOR_H
#define NPC_DI_CREMATOR_H
#ifdef _WIN32
#pragma once
#endif

#include	"npcevent.h"
#include "ai_hull.h"
#include "AI_BaseNPC.h"
#include "AI_Task.h"
#include "AI_Default.h"
#include "AI_Schedule.h"
#include "ai_node.h"
#include "ai_network.h"
#include "ai_hint.h"
#include "ai_link.h"
#include "ai_waypoint.h"
#include "ai_navigator.h"
#include "AI_Motor.h"
#include "ai_senses.h"
#include "ai_squadslot.h"
#include "ai_squad.h"
#include "ai_memory.h"
#include "ai_tacticalservices.h"
#include "ai_moveprobe.h"
#include "ai_utils.h"
#include "datamap.h"
#include "basecombatcharacter.h"
#include "basehlcombatweapon.h"

#include "soundent.h"
#include "activitylist.h"
#include "engine/IEngineSound.h"

#include "props.h"
#include "gib.h"

//#include "grenade_homer.h" // for test attack (disabled) // obsolete
#include "particle_parse.h" // for particle attack and for oil sprays
#include "te_particlesystem.h" // for particle attack and for oil sprays
#include "te_effect_dispatch.h" // for particle attack and for oil sprays

// for the beam attack
#include "fire.h"
#include "beam_shared.h"
#include "beam_flags.h"

// tracing ammo
#include "ammodef.h" // included for ammo-related damage table

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	DI_CREMATOR_IMMOLATE_DIST	256

#define	DI_CREMATOR_BEAM_ATTACH	1
#define DI_CREMATOR_BURN_TIME		20 // burning time for the npc

// for the beam attack
#define DI_CREMATOR_BEAM_SPRITE		"sprites/crystal_beam1.vmt"
#define	DI_CREMATOR_BEAMS	1

#define	DI_CREMATOR_MAX_RANGE		256
#define	DI_CREMATOR_MIN_RANGE		0
#define	DI_CREMATOR_RANGE1_CONE		0.0f

#define DI_CREMATOR_SKIN_ALERT			0 // yellow eyes
#define DI_CREMATOR_SKIN_CALM			1 // blue eyes
#define DI_CREMATOR_SKIN_ANGRY			2 // red eyes
#define DI_CREMATOR_SKIN_DEAD			3 // black eyes

extern ConVar di_dynamic_lighting_npconfire; // declared in ai_basenpc.cpp

extern ConVar di_hipoly_character_models; // declared in ai_basenpc.cpp for all npc's, with FCVAR_ARCHIVE

ConVar sk_di_cremator_health( "sk_di_cremator_health", "150" );
ConVar sk_di_cremator_firedamage( "sk_di_cremator_firedamage", "0" );
ConVar sk_di_cremator_radiusdamage( "sk_di_cremator_radiusdamage", "0" );

//class CSprite;

enum 
{
	TASK_DI_CREMATOR_ALERT = LAST_SHARED_TASK,
	TASK_DI_CREMATOR_IDLE,
	TASK_DI_CREMATOR_HEAR_TARGET,
	TASK_DI_CREMATOR_SEEPLAYER,	
	TASK_DI_CREMATOR_RANGE_ATTACK1,
	TASK_DI_CREMATOR_SET_BALANCE,
};
enum
{
	SCHED_DI_CREMATOR_CHASE = LAST_SHARED_SCHEDULE,
	SCHED_DI_CREMATOR_RANGE_ATTACK1,
	SCHED_DI_CREMATOR_IDLE,
};
enum
{
	COND_DI_CREMATOR_HEAR_TARGET,
};
#define	DI_CREMATOR_AE_IMMO_START					( 6 )

#define	DI_CREMATOR_AE_IMMO_PARTICLE				( 7 )
#define	DI_CREMATOR_AE_IMMO_PARTICLEOFF			( 8 )

// specific hitgroups, numbers are defined in the .qc
#define DI_CREMATOR_CANISTER						2
#define DI_CREMATOR_GUN							10
/*
class CDI_CrematorHead : public CPhysicsProp
{
public:
	DECLARE_CLASS( CDI_CrematorHead, CPhysicsProp );
	DECLARE_DATADESC();

	void Precache( void );
	void Spawn( void );

	virtual void VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	virtual void OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason );
	
};
LINK_ENTITY_TO_CLASS(crematorhead, CDI_CrematorHead);
*/
// commented out til better times

class CNPC_DI_Cremator : public CAI_BaseNPC
{
			
	DECLARE_CLASS( CNPC_DI_Cremator, CAI_BaseNPC );
	DECLARE_DATADESC();

public:

	void	Precache( void );
	void	Spawn( void );
	Class_T	Classify( void );


//	void	IdleSound( void );
	void	AlertSound( void );
	void	PainSound( const CTakeDamageInfo &info );
	void	DeathSound( const CTakeDamageInfo &info );
	float	m_flNextIdleSoundTime;

	void CallForHelp( char *szClassname, float flDist, CBaseEntity * pEnemy, Vector &vecLocation );

	float	MaxYawSpeed ( void );	
	//int		GetSoundInterests ( void );
	
	virtual const char *GetHeadpropModel( void );	
	void	Event_Killed( const CTakeDamageInfo &info );

/*	void	ShouldDropHead( const CTakeDamageInfo &HeadInfo, float flDamageThreshold );
	void	ShouldExplodeCanister( const CTakeDamageInfo &expInfo, float flDamageThreshold );*/

	void	DropHead( int iVelocity );

	int		OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	void	Ignite( float flFlameLifetime, bool bNPCOnly, float flSize, bool bCalledByLevelDesigner );
	void	EnemyIgnited( CAI_BaseNPC *pVictim );

	void	SelectSkin( void ); // manages eyes' color: yellow for alerted state, red for combat state, blue for idle state, and black for death state.
	
	NPC_STATE		SelectIdealState ( void );


	void	OnChangeActivity( Activity eNewActivity );
	void	StartTask( const Task_t *pTask );
	void	RunTask( const Task_t *pTask );
	void	HandleAnimEvent( animevent_t *pEvent );

	void	DispatchSpray( void );
	//void	DispatchThink( void );
	// all the commented out stuff below is obsolete
	//void	StartImmolating();
	//void	StopImmolating();
	//void	ImmolationDamage( const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore );
	//void	ImmolationTouch( CBaseEntity *pOther );
	//void	UpdateThink ();
	//void	Update();
	float	m_flImmoRadius;
	float	m_flTimeLastUpdatedRadius;
	
	Activity NPC_TranslateActivity( Activity baseAct );	
	int				TranslateSchedule( int type );	
	virtual int		SelectSchedule( void );
	int				RangeAttack1Conditions( float flDot, float flDist );

	int				GetSoundInterests( void );

private:

	int	  m_iBeams;
	CBeam *m_pBeam[DI_CREMATOR_BEAMS];
	int	m_MuzzleAttachment;
	int	m_HeadAttachment;
	bool m_bHeadshot;
	bool m_bCanisterShot;
	int m_iVoicePitch;

#endif // NPC_DI_CREMATOR_H