//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef NPC_AlphaBlarneyHL1_H
#define NPC_AlphaBlarneyHL1_H

#include "hl1_npc_talker.h"

//=========================================================
//=========================================================
class CNPC_HL1AlphaBlarney : public CHL1NPCTalker
{
	DECLARE_CLASS( CNPC_HL1AlphaBlarney, CHL1NPCTalker );
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
	void    AlphaBlarneyFirePistol ( void );
	void    AlphaBlarneyFireShotgun ( void );
	void    AlphaBlarneyFireSniper ( void );
	void    AlphaBlarneyFireSMG ( void );

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

	int		m_iWeapons;

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
		SCHED_ALPHABLARNEY_FOLLOW = BaseClass::NEXT_SCHEDULE,
		SCHED_ALPHABLARNEY_ENEMY_DRAW,
		SCHED_ALPHABLARNEY_FACE_TARGET,
		SCHED_ALPHABLARNEY_IDLE_STAND,
		SCHED_ALPHABLARNEY_STOP_FOLLOWING,
	};

	DEFINE_CUSTOM_AI;
};

#endif	//NPC_AlphaBlarneyHL1_H
