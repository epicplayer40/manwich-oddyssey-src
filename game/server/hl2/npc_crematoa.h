#ifndef NPC_CREMATOA_H
#define NPC_CREMATOA_H
#ifdef _WIN32
#pragma once
#endif

#include	"npcevent.h"
#include "ai_hull.h"
#include "ai_basenpc.h"
#include "ai_task.h"
#include "ai_default.h"
#include "ai_schedule.h"
#include "ai_node.h"
#include "ai_network.h"
#include "ai_hint.h"
#include "ai_link.h"
#include "ai_waypoint.h"
#include "ai_navigator.h"
#include "ai_motor.h"
#include "ai_senses.h"
#include "ai_squadslot.h"
#include "ai_squad.h"
#include "ai_memory.h"
#include "ai_tacticalservices.h"
#include "ai_moveprobe.h"
#include "ai_utils.h"
#include "datamap.h"

#include "soundent.h"
#include "activitylist.h"
#include "engine/IEngineSound.h"

#include "props.h"
#include "gib.h"

//#include "grenade_homer.h" // for test attack (disabled)
#include "particle_parse.h" // for particle attack and for oil sprays
#include "te_particlesystem.h" // for particle attack and for oil sprays
#include "te_effect_dispatch.h" // for particle attack and for oil sprays

// for the beam attack
#include "fire.h"
#include "beam_shared.h"
#include "beam_flags.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	CREMATOA_IMMOLATE_DIST	256

#define	CREMATOA_BEAM_ATTACH	1
#define CREMATOA_BURN_TIME		20 // it's the time that the C. itself burns, walking in flames with dynamic light

// for the beam attack
#define CREMATOA_BEAM_SPRITE		"sprites/bluelaser1.vmt"
#define	CREMATOA_BEAMS	1

#define	CREMATOA_MAX_RANGE		500
#define	CREMATOA_MIN_RANGE		0
#define	CREMATOA_RANGE1_CONE		0.0f

ConVar sk_crematoa_health( "sk_crematoa_health", "1802" );

//class CSprite;

enum 
{
	TASK_CREMATOA_ALERT = LAST_SHARED_TASK,
	TASK_CREMATOA_IDLE,
	TASK_CREMATOA_HEAR_TARGET,
	TASK_CREMATOA_SEEPLAYER,	
	TASK_CREMATOA_RANGE_ATTACK1,
	TASK_CREMATOA_SET_BALANCE,
};
enum
{
	SCHED_CREMATOA_CHASE = LAST_SHARED_SCHEDULE,
	SCHED_CREMATOA_RANGE_ATTACK1,
	SCHED_CREMATOA_IDLE,
};
enum
{
	COND_CREMATOA_HEAR_TARGET,
};
#define	CREMATOA_AE_IMMO_STRIKE					( 6 )
/*
class CCrematorHead : public CPhysicsProp
{
public:
	DECLARE_CLASS( CCrematorHead, CPhysicsProp );
	DECLARE_DATADESC();

	void Precache( void );
	void Spawn( void );

	virtual void VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	virtual void OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason );
	
};
LINK_ENTITY_TO_CLASS(crematorhead, CCrematorHead);
*/
// commented out til better times

class CNPC_Crematoa : public CAI_BaseNPC
{
			
	DECLARE_CLASS( CNPC_Crematoa, CAI_BaseNPC );
	DECLARE_DATADESC();

public:

	void	Precache( void );
	void	Spawn( void );
	Class_T	Classify( void );

	void	 IdleSound( void );
	void	AlertSound( void );
	void	DeathSound( const CTakeDamageInfo &info );

	void	HandleAnimEvent( animevent_t *pEvent );
	float	MaxYawSpeed ( void );	
	float	m_flNextIdleSoundTime;
	//int		GetSoundInterests ( void );
	
	virtual const char *GetHeadpropModel( void );
	
	void	Event_Killed( const CTakeDamageInfo &info );
	void	DropHead( );
	int		OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void	Ignite( float flFlameLifetime, bool bNPCOnly, float flSize, bool bCalledByLevelDesigner );
	void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );

	Activity NPC_TranslateActivity( Activity baseAct );	
	int				TranslateSchedule( int type );	
	virtual int		SelectSchedule( void );
	int				RangeAttack1Conditions( float flDot, float flDist );

	void	StartTask( const Task_t *pTask );
	void	RunTask( const Task_t *pTask );

	void	DispatchSpray( void );
	void	RemoveBeams( void );

	void	EnemyIgnited( CAI_BaseNPC *pVictim );

private:

	int	  m_iBeams;
	CBeam *m_pBeam[CREMATOA_BEAMS];
	int	m_MuzzleAttachment;
	int	m_HeadAttachment;

#endif // NPC_CREMATOA_H