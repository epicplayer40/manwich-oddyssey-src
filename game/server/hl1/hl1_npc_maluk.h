//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef NPC_MalukHL1_H
#define NPC_MalukHL1_H

#include "hl1_npc_talker.h"

#define	MALUK_MAX_BEAMS	 1

//=========================================================
//=========================================================
class CNPC_HL1Maluk : public CHL1NPCTalker
{
	DECLARE_CLASS( CNPC_HL1Maluk, CHL1NPCTalker );
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
	void    MalukFirePistol ( void );

	bool	CorpseGib( const CTakeDamageInfo &info );
	
	int		OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo );
	void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	void	Event_Killed( const CTakeDamageInfo &info );

	void    PainSound( const CTakeDamageInfo &info );
	void	DeathSound( const CTakeDamageInfo &info );

	void	HandleAnimEvent( animevent_t *pEvent );
	int		TranslateSchedule( int scheduleType );
	int		SelectSchedule( void );

	void ClearBeams( void );

	void	DeclineFollowing( void );

	bool	CanBecomeRagdoll( void );
	bool	ShouldGib( const CTakeDamageInfo &info );

	void ZapBeam( int side ); //originally int side

	int		RangeAttack1Conditions( float flDot, float flDist );

	void	SUB_StartLVFadeOut( float delay = 10.0f, bool bNotSolid = true );
	void	SUB_LVFadeOut( void  );

	NPC_STATE CNPC_HL1Maluk::SelectIdealState ( void );

	bool	m_fGunDrawn;
	float	m_flPainTime;
	float	m_flCheckAttackTime;
	bool	m_fLastAttackCheck;

	CBeam *m_pBeam[MALUK_MAX_BEAMS];
	int	  m_iBeams;
	
	int		m_iAmmoType;

	enum
	{
		SCHED_MALUK_FOLLOW = BaseClass::NEXT_SCHEDULE,
		SCHED_MALUK_ENEMY_DRAW,
		SCHED_MALUK_FACE_TARGET,
		SCHED_MALUK_IDLE_STAND,
		SCHED_MALUK_STOP_FOLLOWING,
	};

	DEFINE_CUSTOM_AI;
};

#endif	//NPC_MalukHL1_H