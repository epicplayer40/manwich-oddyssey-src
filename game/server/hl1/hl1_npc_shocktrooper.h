//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef NPC_STROOPER_H
#define NPC_STROOPER_H

#include "ai_squad.h"
#include "hl1_ai_basenpc.h"
#include	"Sprite.h"
//#include "hl1_grenade_spit.h"
#include "hl2_shareddefs.h"


class CNPC_STrooper : public CHL1BaseNPC
{
	DECLARE_CLASS( CNPC_STrooper, CHL1BaseNPC );
public:

	void	Precache( void );
	void	Spawn( void );

	void	JustSpoke( void );
	void	SpeakSentence( void );
	void	PrescheduleThink ( void );

	bool	FOkToSpeak( void );
	
	Class_T	Classify ( void );
	int     RangeAttack1Conditions ( float flDot, float flDist );
	int		MeleeAttack1Conditions ( float flDot, float flDist );
	int     RangeAttack2Conditions ( float flDot, float flDist );

	Activity NPC_TranslateActivity( Activity eNewActivity );

	void	ClearAttackConditions( void );

	int		IRelationPriority( CBaseEntity *pTarget );

	int     GetGrenadeConditions ( float flDot, float flDist );

	void			StartFollowing( CBaseEntity *pLeader );
	void			StopFollowing( void );

	virtual void 	FollowerUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	bool	FCanCheckAttacks( void );

	int     GetSoundInterests ( void );

	void    TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	int		OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo );

	float	MaxYawSpeed( void );

	void    IdleSound( void );

	void	CheckAmmo ( void );

	CBaseEntity *Kick( void );

	Vector	Weapon_ShootPosition( void );
	void	HandleAnimEvent( animevent_t *pEvent );

	void	Shoot ( void );
	void	Shotgun( void );
	
	void	StartTask ( const Task_t *pTask );
	void	RunTask ( const Task_t *pTask );

	int		SelectSchedule( void );
	int		TranslateSchedule( int scheduleType );


	void	PainSound( const CTakeDamageInfo &info );
	void	DeathSound( const CTakeDamageInfo &info );
	void	SetAim( const Vector &aimDir );

	bool	HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt);

	void	StartNPC ( void );

	int		SquadRecruit( int searchRadius, int maxMembers );
	
	void	Event_Killed( const CTakeDamageInfo &info );

	
	static const char *pGruntSentences[];
	
	bool	m_bInBarnacleMouth;
	float m_flNextGrenadeCheck;
	float m_flNextPainTime;
	float m_flLastEnemySightTime;
	float m_flTalkWaitTime;

	Vector	m_vecTossVelocity;

	int		m_iLastGrenadeCondition;
	bool	m_fStanding;
	bool	m_fFirstEncounter;// only put on the handsign show in the squad's first encounter.
	int		m_iClipSize;

	int		m_voicePitch;

	int		m_iSentence;

	float	m_flCheckAttackTime;

	int		m_iAmmoType;
	
	int		m_iWeapons;

		int m_iBall[2];

public:
	DECLARE_DATADESC();

	enum
	{
		SCHED_STROOPER_FOLLOW = BaseClass::NEXT_SCHEDULE,
		SCHED_STROOPER_STOP_FOLLOWING,
		SCHED_STROOPER_FACE_TARGET,
	};

	DEFINE_CUSTOM_AI;

private:

	// checking the feasibility of a grenade toss is kind of costly, so we do it every couple of seconds,
	// not every server frame.
	
};



class CNPC_StrooperZapBall : public CAI_BaseNPC
{	
public:	
	DECLARE_CLASS( CNPC_StrooperZapBall, CAI_BaseNPC );

	DECLARE_DATADESC();

	void Spawn( void );
	void Precache( void );

	void EXPORT AnimateThink( void );
	void EXPORT ExplodeTouch( CBaseEntity *pOther );

	void Kill( void );

	EHANDLE m_hOwner;
	float m_flSpawnTime;

	CSprite *m_pSprite;
};



#endif