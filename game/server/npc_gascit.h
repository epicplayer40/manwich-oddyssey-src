//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:		This is the base version of the combine (not instanced only subclassed)
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef	NPC_GASCIT_H
#define	NPC_GASCIT_H
#pragma once

//#if 0
#include "npc_talker.h"

//=========================================================
//	>> CNPC_Gascit
//=========================================================
//class CNPC_Gascit : public CAI_PlayerAlly
class CNPC_Gascit : public CNPCSimpleTalker
{
//	DECLARE_CLASS( CNPC_Gascit, CAI_PlayerAlly );
	DECLARE_CLASS( CNPC_Gascit, CNPCSimpleTalker );
public:
	void			Spawn( void );
	void			Precache( void );
	float			MaxYawSpeed( void );
	int				GetSoundInterests( void );
	void			GascitFirePistol( void );
	void			AlertSound( void );
	Class_T			Classify ( void );
	void			HandleAnimEvent( animevent_t *pEvent );
	bool			HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt);

	void			RunTask( const Task_t *pTask );
	int				ObjectCaps( void ) { return UsableNPCObjectCaps( BaseClass::ObjectCaps() ); }
	int				OnTakeDamage_Alive( const CTakeDamageInfo &info );
	Vector			BodyTarget( const Vector &posSrc, bool bNoisy = true );

	Activity		GetFollowActivity( float flDistance ) { return ACT_RUN; }

	void			DeclineFollowing( void );

	Activity		NPC_TranslateActivity( Activity eNewActivity );
	WeaponProficiency_t CalcWeaponProficiency( CBaseCombatWeapon *pWeapon );

	// Override these to set behavior
	virtual int		TranslateSchedule( int scheduleType );
	virtual int		SelectSchedule( void );

	void			DeathSound( void );
	void			PainSound( void );
	
	void			TalkInit( void );

	void			TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );

	bool			m_fGunDrawn;
	float			m_painTime;
	float			m_checkAttackTime;
	float			m_nextLineFireTime;
	bool			m_lastAttackCheck;
	bool			m_bInBarnacleMouth;

	float m_flNextGrenadeCheck;
	int		m_iLastGrenadeCondition;
	Vector	m_vecTossVelocity;
	int     RangeAttack2Conditions ( float flDot, float flDist );
	int     GetGrenadeConditions ( float flDot, float flDist );


	//=========================================================
	// Gascit Tasks
	//=========================================================
	enum 
	{
		TASK_GASCIT_CROUCH = BaseClass::NEXT_TASK,
	};

	//=========================================================
	// Gascit schedules
	//=========================================================
	enum
	{
		SCHED_GASCIT_FOLLOW = BaseClass::NEXT_SCHEDULE,
		SCHED_GASCIT_DRAW,
		SCHED_GASCIT_FACE_TARGET,
		SCHED_GASCIT_STAND,
		SCHED_GASCIT_AIM,
	//	SCHED_GASCIT_MELEE_CHARGE,
		SCHED_GASCIT_BARNACLE_HIT,
		SCHED_GASCIT_BARNACLE_PULL,
		SCHED_GASCIT_BARNACLE_CHOMP,
		SCHED_GASCIT_BARNACLE_CHEW,
		SCHED_GASCIT_ROLL1,
		SCHED_GASCIT_ROLL2,
	};


public:
	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;
};

//#endif

#endif	//NPC_GASCIT_H