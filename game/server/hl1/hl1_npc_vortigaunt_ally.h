//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef NPC_AllygauntHL1_H
#define NPC_AllygauntHL1_H

#define	ALLYGAUNT_MAX_BEAMS	8

#include "hl1_npc_talker.h"
//=========================================================
//=========================================================
class CNPC_HL1Allygaunt : public CHL1NPCTalker
{
	DECLARE_CLASS( CNPC_HL1Allygaunt, CHL1NPCTalker );
public:

	void Spawn( void );
	void Precache( void );
	Class_T	Classify ( void );

	void AlertSound( void );
	void IdleSound( void );
	void PainSound( const CTakeDamageInfo &info );
	void DeathSound( const CTakeDamageInfo &info );
	
	int	 GetSoundInterests ( void );

	void	DeclineFollowing( void );
	
	float MaxYawSpeed ( void );

	void Event_Killed( const CTakeDamageInfo &info );
	void CallForHelp( char *szClassname, float flDist, CBaseEntity * pEnemy, Vector &vecLocation );

	int  RangeAttack1Conditions( float flDot, float flDist );

	int  OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo );
	void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );

	NPC_STATE CNPC_HL1Allygaunt::SelectIdealState ( void );
	
	void StartTask( const Task_t *pTask );

	int  SelectSchedule( void );
	int  TranslateSchedule( int scheduleType );

	void ArmBeam( int side );
	void BeamGlow( void );
	void WackBeam( int side, CBaseEntity *pEntity );
	void ZapBeam( int side );
	void ClearBeams( void );

	void HandleAnimEvent( animevent_t *pEvent );




	enum
	{
		SCHED_ALLYGAUNT_FOLLOW = BaseClass::NEXT_SCHEDULE,
		SCHED_ALLYGAUNT_ENEMY_DRAW,
		SCHED_ALLYGAUNT_FACE_TARGET,
		SCHED_ALLYGAUNT_IDLE_STAND,
		SCHED_ALLYGAUNT_STOP_FOLLOWING,
	};


	DEFINE_CUSTOM_AI;
	DECLARE_DATADESC();

private:
	int m_iVoicePitch;
	int	  m_iBeams;

	int m_iBravery;

	CBeam *m_pBeam[ALLYGAUNT_MAX_BEAMS];

	float m_flNextAttack;

	EHANDLE m_hDead;
};


#endif //NPC_AllygauntHL1_