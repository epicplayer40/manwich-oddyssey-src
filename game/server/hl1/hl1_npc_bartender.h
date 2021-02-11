//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef NPC_BartenderHL1_H
#define NPC_BartenderHL1_H

#include "hl1_npc_talker.h"

//=========================================================
//=========================================================
class CNPC_HL1Bartender : public CHL1NPCTalker
{
	DECLARE_CLASS( CNPC_HL1Bartender, CHL1NPCTalker );
public:
	
	DECLARE_DATADESC();

	virtual void ModifyOrAppendCriteria( AI_CriteriaSet& set );

	void	Precache( void );
	void	Spawn( void );
	void	TalkInit( void );

	void	StartTask( const Task_t *pTask );
	void	RunTask( const Task_t *pTask );

	int		GetSoundInterests ( void );
	Class_T  Classify ( void );
	void	AlertSound( void );
	void    SetYawSpeed ( void );

	bool    CheckRangeAttack1 ( float flDot, float flDist );
	void    BartenderFirePistol ( void );

	bool	CorpseGib( const CTakeDamageInfo &info );


	mutable float	m_flJumpDist;
	
	int		OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo );
	void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	void	Event_Killed( const CTakeDamageInfo &info );

	bool IsJumpLegal(const Vector &startPos, const Vector &apex, const Vector &endPos) const;

	void    PainSound( const CTakeDamageInfo &info );
	void	DeathSound( const CTakeDamageInfo &info );

	void	HandleAnimEvent( animevent_t *pEvent );
	int		TranslateSchedule( int scheduleType );
	int		SelectSchedule( void );

	void	DeclineFollowing( void );

	bool	CanBecomeRagdoll( void );
	bool	ShouldGib( const CTakeDamageInfo &info );

	int		RangeAttack1Conditions( float flDot, float flDist );

	void	SUB_StartLVFadeOut( float delay = 10.0f, bool bNotSolid = true );
	void	SUB_LVFadeOut( void  );

	NPC_STATE CNPC_HL1Bartender::SelectIdealState ( void );

	bool	m_fGunDrawn;
	float	m_flPainTime;
	float	m_flCheckAttackTime;
	bool	m_fLastAttackCheck;
	
	int		m_iAmmoType;

	enum
	{
		SCHED_BARTENDER_FOLLOW = BaseClass::NEXT_SCHEDULE,
		SCHED_BARTENDER_ENEMY_DRAW,
		SCHED_BARTENDER_FACE_TARGET,
		SCHED_BARTENDER_IDLE_STAND,
		SCHED_BARTENDER_STOP_FOLLOWING,
	};

	DEFINE_CUSTOM_AI;
};

#endif	//NPC_BartenderHL1_H