//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef NPC_AlphaGarneyHL1_H
#define NPC_AlphaGarneyHL1_H

#include "hl1_npc_talker.h"

//=========================================================
//=========================================================
class CNPC_HL1AlphaGarney : public CHL1NPCTalker
{
	DECLARE_CLASS( CNPC_HL1AlphaGarney, CHL1NPCTalker );
public:
	
	DECLARE_DATADESC();

	virtual void ModifyOrAppendCriteria( AI_CriteriaSet& set );

	void	Precache( void );
	void	Spawn( void );
	void	TalkInit( void );

	int     GetGrenadeConditions ( float flDot, float flDist );

	void	StartTask( const Task_t *pTask );
	void	RunTask( const Task_t *pTask );

	int		GetSoundInterests ( void );
	Class_T  Classify ( void );
	void	AlertSound( void );
	void    SetYawSpeed ( void );

	bool    CheckRangeAttack1 ( float flDot, float flDist );
	void    AlphaGarneyFirePistol ( void );
	void    AlphaGarneyFireShotgun ( void );
	void    AlphaGarneyFireSniper ( void );
	void    AlphaGarneyFireSMG ( void );
	int		m_iWeapons;

	bool	CorpseGib( const CTakeDamageInfo &info );
	
	int		OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo );
	void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	void	Event_Killed( const CTakeDamageInfo &info );

	void    PainSound( const CTakeDamageInfo &info );
	void	DeathSound( const CTakeDamageInfo &info );

	void	HandleAnimEvent( animevent_t *pEvent );
	int		TranslateSchedule( int scheduleType );
	int		SelectSchedule( void );

	void	DeclineFollowing( void );

	bool	CanBecomeRagdoll( void );
	bool	ShouldGib( const CTakeDamageInfo &info );

	int		RangeAttack1Conditions( float flDot, float flDist );
	int     RangeAttack2Conditions ( float flDot, float flDist );

	void	SUB_StartLVFadeOut( float delay = 10.0f, bool bNotSolid = true );
	void	SUB_LVFadeOut( void  );

	NPC_STATE SelectIdealState ( void );

	bool	m_fGunDrawn;
	float	m_flPainTime;
	float	m_flCheckAttackTime;
	bool	m_fLastAttackCheck;

	float m_flNextGrenadeCheck;
	int		m_iLastGrenadeCondition;
	Vector	m_vecTossVelocity;
	
	int		m_iAmmoType;

	enum
	{
		SCHED_ALPHAGARNEY_FOLLOW = BaseClass::NEXT_SCHEDULE,
		SCHED_ALPHAGARNEY_ENEMY_DRAW,
		SCHED_ALPHAGARNEY_FACE_TARGET,
		SCHED_ALPHAGARNEY_IDLE_STAND,
		SCHED_ALPHAGARNEY_STOP_FOLLOWING,
	};

	DEFINE_CUSTOM_AI;
};

#endif	//NPC_AlphaGarneyHL1_H
