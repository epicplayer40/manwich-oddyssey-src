//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:		Base combat character with no AI
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef NPC_HL2BULLSQUID_H
#define NPC_HL2BULLSQUID_H

#include "ai_basenpc.h"
#include "ai_blended_movement.h"


class CNPC_HL2Bullsquid : public CAI_BlendedNPC
{
	DECLARE_CLASS( CNPC_HL2Bullsquid, CAI_BlendedNPC );
	DECLARE_DATADESC();

public:
	void Spawn( void );
	void Precache( void );
	Class_T	Classify( void );
	
	void IdleSound( void );
	void AlertSound( void );
	void PainSound( const CTakeDamageInfo &info );
	void DeathSound( const CTakeDamageInfo &info );
	void AttackSound( void );
	void GrowlSound( void );

	float MaxYawSpeed ( void );

	void HandleAnimEvent( animevent_t *pEvent );

	int RangeAttack1Conditions( float flDot, float flDist );
	int MeleeAttack1Conditions( float flDot, float flDist );
	int MeleeAttack2Conditions( float flDot, float flDist );

	bool FValidateHintType ( CAI_Hint *pHint );
	void RemoveIgnoredConditions( void );
	Disposition_t IRelationType( CBaseEntity *pTarget );
	int OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo );

	int GetSoundInterests ( void );
	void RunAI ( void );
	virtual void OnListened ( void );

	int SelectSchedule( void );
	bool FInViewCone ( Vector pOrigin );
	bool FVisible ( Vector vecOrigin );

	void StartTask ( const Task_t *pTask );
	void RunTask ( const Task_t *pTask );

	bool OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval );
	int TranslateSchedule( int type );

	NPC_STATE SelectIdealState ( void );

	DEFINE_CUSTOM_AI;

private:
	
	bool  m_fCanThreatDisplay;// this is so the squid only does the "I see a headcrab!" dance one time. 
	float m_flLastHurtTime;// we keep track of this, because if something hurts a squid, it will forget about its love of headcrabs for a while.
	float m_flNextSpitTime;// last time the bullsquid used the spit attack.
	int   m_nSquidSpitSprite;
	float m_flHungryTime;// set this is a future time to stop the monster from eating for a while. 

	float m_fNextPainSoundingTime;

	float m_nextSquidSoundTime;

	//virtual bool GetSpitVector( const Vector &vecStartPos, const Vector &vecTarget, Vector *vecOut );
	Vector	m_vecSaveBullsquidSpitVelocity;	// Saved when we start to attack and used if we failed to get a clear shot once we release
};
#endif // NPC_HL2BULLSQUID_H