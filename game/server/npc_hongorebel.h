//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

//TERO: NOTE NOTE NOTE! WHEN YOU START ADDING THE COMMANDABLE STUFF GO TO AI_ALLYMANAGER.CPP AND 
//      void CAI_AllyManager::CountAllies( int *pTotal, int *pMedics ) AND CHANGE ALL THE ALLY
//		STUFF TO POINT TO METROCOPS



#ifndef NPC_HONGOREBEL_H
#define NPC_HONGOREBEL_H
#ifdef _WIN32
#pragma once
#endif

#define ELOISE_KICK_BALLS

#include "rope.h"
#include "rope_shared.h"
#include "ai_baseactor.h"
#include "ai_basenpc.h"
#include "ai_goal_police.h"
#include "ai_behavior.h"
#include "ai_behavior_standoff.h"
#include "ai_behavior_assault.h"
#include "ai_behavior_functank.h"
#include "ai_behavior_actbusy.h"
#include "ai_behavior_rappel.h"
#include "ai_behavior_police.h"
#include "ai_behavior_follow.h"
#include "ai_behavior_recharge.h"
#include "ai_sentence.h"
#include "props.h"

#include "npc_playercompanion.h"

struct SquadCandidate_t;


//#define SF_HONGOREBEL_					0x00010000
#define SF_HONGOREBEL_SIMPLE_VERSION		0x00020000
#define SF_HONGOREBEL_ALWAYS_STITCH		0x00080000
#define SF_HONGOREBEL_NOCHATTER			0x00100000
#define SF_HONGOREBEL_ARREST_ENEMY			0x00200000
#define SF_HONGOREBEL_NO_FAR_STITCH		0x00400000
#define SF_HONGOREBEL_NO_MANHACK_DEPLOY	0x00800000
#define SF_HONGOREBEL_ALLOWED_TO_RESPOND	0x01000000
#define SF_HONGOREBEL_MID_RANGE_ATTACK		0x02000000

#define SF_HONGOREBEL_FOLLOW				0x04000000
#define	SF_HONGOREBEL_MEDIC				0x08000000
#define SF_HONGOREBEL_AMMORESUPPLIER		0x10000000	
#define SF_HONGOREBEL_NOT_COMMANDABLE		0x20000000
#define SF_HONGOREBEL_USE_RENDER_BOUNDS	0x40000000

/*static const char *TLK_CP_MEDIC			= "TLK_CP_MEDIC";
static const char *TLK_CP_BEES			= "TLK_CP_BEES";
static const char *TLK_CP_RECHARGING	= "TLK_CP_RECHARGING";*/

class CNPC_HongoRebel;

class CNPC_HongoRebel : public CNPC_PlayerCompanion //CAI_BaseActor
{
	DECLARE_CLASS( CNPC_HongoRebel, CNPC_PlayerCompanion ); //CAI_BaseActor );
	DECLARE_DATADESC();

public:
	CNPC_HongoRebel();

	virtual bool		CreateComponents();
	bool				CreateBehaviors();
	void				Spawn( void );
	void				Precache( void );

#ifdef ELOISE_KICK_BALLS

	int MeleeAttack1Conditions ( float flDot, float flDist );

#endif


	virtual int			GetMaxHealth() { return m_iMaxHealth; }

	Class_T				Classify( void );
	float				MaxYawSpeed( void );
	void				HandleAnimEvent( animevent_t *pEvent );
	Activity			NPC_TranslateActivity( Activity newActivity );

	Vector				EyeDirection3D( void )	{ return CAI_BaseHumanoid::EyeDirection3D(); } // cops don't have eyes

	virtual void		Event_Killed( const CTakeDamageInfo &info );

	virtual void		OnScheduleChange();

	float				GetIdealAccel( void ) const;
	int					ObjectCaps( void ) { return UsableNPCObjectCaps(BaseClass::ObjectCaps()); }
	void				SimpleUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	// These are overridden so that the cop can shove and move a non-criminal player safely
	//TERO:	we don't need this shit
	//CBaseEntity			*CheckTraceHullAttack( float flDist, const Vector &mins, const Vector &maxs, int iDamage, int iDmgType, float forceScale, bool bDamageAnyNPC );
	//CBaseEntity			*CheckTraceHullAttack( const Vector &vStart, const Vector &vEnd, const Vector &mins, const Vector &maxs, int iDamage, int iDmgType, float flForceScale, bool bDamageAnyNPC );

	virtual int			SelectSchedule( void );
	virtual int			SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );
	virtual int			TranslateSchedule( int scheduleType );
	void				StartTask( const Task_t *pTask );
	void				RunTask( const Task_t *pTask );
//	virtual Vector		GetActualShootTrajectory( const Vector &shootOrigin );
//	virtual void		FireBullets( const FireBulletsInfo_t &info );
	virtual bool		HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt);
	virtual void		Weapon_Equip( CBaseCombatWeapon *pWeapon );

	//virtual bool		OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval );
	bool				OnObstructionPreSteer( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult );
	bool				ShouldBruteForceFailedNav()	{ return false; }

	virtual void		GatherConditions( void );

	virtual bool		OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval );

	virtual bool		ShouldMoveAndShoot();

	// TraceAttack
	virtual void		TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );

	// Speaking
	virtual void		SpeakSentence( int nSentenceType );

	// Set up the shot regulator based on the equipped weapon
	virtual void		OnUpdateShotRegulator( );

	bool				ShouldKnockOutTarget( CBaseEntity *pTarget );
	void				KnockOutTarget( CBaseEntity *pTarget );
	void				StunnedTarget( CBaseEntity *pTarget );
	//void				AdministerJustice( void );

	bool				QueryHearSound( CSound *pSound );

	void				SetBatonState( bool state );
	bool				BatonActive( void );

	CAI_Sentence< CNPC_HongoRebel > *GetSentences() { return &m_Sentences; }

	virtual	bool		AllowedToIgnite( void ) { return true; }

	void				PlayFlinchGesture( void );

	void				AnswerQuestion( CAI_PlayerAlly *pQuestioner, int iQARandomNum, bool bAnsweringHello );

protected:
	// Determines the best type of flinch anim to play.
	virtual Activity	GetFlinchActivity( bool bHeavyDamage, bool bGesture );

	// Only move and shoot when attacking
	virtual bool		OnBeginMoveAndShoot();
	virtual void		OnEndMoveAndShoot();

private:
	void		ReleaseManhack( void );

	// Speech-related methods
	void				AnnounceTakeCoverFromDanger( CSound *pSound );
	void				AnnounceEnemyType( CBaseEntity *pEnemy );
	void				AnnounceEnemyKill( CBaseEntity *pEnemy );
	void				AnnounceHarrassment( );
	void				AnnounceOutOfAmmo( );

	// Behavior-related sentences
	void				SpeakFuncTankSentence( int nSentenceType );
	void				SpeakAssaultSentence( int nSentenceType );
	void				SpeakStandoffSentence( int nSentenceType );

	virtual void		LostEnemySound( void );
	virtual void		FoundEnemySound( void );
	virtual void		AlertSound( void );
	virtual void		PainSound( const CTakeDamageInfo &info );
	virtual void		DeathSound( const CTakeDamageInfo &info );
	virtual void		IdleSound( void );
	virtual bool		ShouldPlayIdleSound( void );

	bool				ShoutForMedic( void );

	int					OnTakeDamage_Alive( const CTakeDamageInfo &info );

	int					GetSoundInterests( void );

	virtual bool		IsAllowedToSpeak( AIConcept_t concept, bool bRespondingToPlayer = false );

	void				BuildScheduleTestBits( void );

	bool				CanDeployManhack( void );

	void				PrescheduleThink( void );

	virtual bool		UseAttackSquadSlots()	{ return true; }

	WeaponProficiency_t CalcWeaponProficiency( CBaseCombatWeapon *pWeapon );

	// Inputs
	void InputEnableManhackToss( inputdata_t &inputdata );
	void InputSetPoliceGoal( inputdata_t &inputdata );
	void InputActivateBaton( inputdata_t &inputdata );
	void InputSpeakRecharge( inputdata_t &inputdata );
	void InputSpeakGeneratorOffline( inputdata_t &inputdata );

	//TERO: from player ally
	//void InputAnswerQuestion( inputdata_t &inputdata );

	void NotifyDeadFriend ( CBaseEntity* pFriend );

	// Anim event handlers
	void OnAnimEventDeployManhack( animevent_t *pEvent );
	void OnAnimEventShove( void );
	void OnAnimEventBatonOn( void );
	void OnAnimEventBatonOff( void );
	void OnAnimEventStartDeployManhack( void );
	void OnAnimEventPreDeployManhack( void );

	bool HasBaton( void );

	// Normal schedule selection 
	int SelectCombatSchedule();
	int SelectScheduleNewEnemy();
//	int SelectScheduleArrestEnemy();
	int SelectRangeAttackSchedule();
	int SelectScheduleNoDirectEnemy();
	//int SelectScheduleInvestigateSound();
	int SelectShoveSchedule( void );

	bool TryToEnterPistolSlot( int nSquadSlot );


	// Handle flinching
	bool IsHeavyDamage( const CTakeDamageInfo &info );

	// Compute a predicted enemy position n seconds into the future
	/*void PredictShootTargetPosition( float flDeltaTime, float flMinLeadDist, float flAddVelocity, Vector *pVecTarget, Vector *pVecTargetVel );

	// Compute a predicted velocity n seconds into the future (given a known acceleration rate)
	void PredictShootTargetVelocity( float flDeltaTime, Vector *pVecTargetVel );*/

	// How many shots will I fire in a particular amount of time?
	int CountShotsInTime( float flDeltaTime ) const;
	float GetTimeForShots( int nShotCount ) const;


	// Can me enemy see me? 
	bool CanEnemySeeMe( );

	// How many squad members are trying to arrest the player?
	//int SquadArrestCount();

	// He's resisting arrest!
	//void EnemyResistingArrest();

	// Rappel
	virtual bool IsWaitingToRappel( void ) { return m_RappelBehavior.IsWaitingToRappel(); }
	void BeginRappel() { m_RappelBehavior.BeginRappel(); }

private:


	enum
	{
		COND_HONGOREBEL_ON_FIRE = BaseClass::NEXT_CONDITION,
	//	COND_HONGOREBEL_ENEMY_RESISTING_ARREST,
		COND_HONGOREBEL_CHANGE_BATON_STATE,
		COND_HONGOREBEL_PHYSOBJECT_ASSAULT,

		COND_CP_PLAYERHEALREQUEST,
		COND_CP_COMMANDHEAL,
		COND_CP_SCRIPT_INTERRUPT,

	};

	enum
	{
		//SCHED_HONGOREBEL_WALK = BaseClass::NEXT_SCHEDULE,
		SCHED_HONGOREBEL_WAKE_ANGRY = BaseClass::NEXT_SCHEDULE,
		SCHED_HONGOREBEL_HARASS,
		SCHED_HONGOREBEL_CHASE_ENEMY,
		SCHED_HONGOREBEL_ESTABLISH_LINE_OF_FIRE,
		SCHED_HONGOREBEL_DRAW_PISTOL,
		SCHED_HONGOREBEL_DEPLOY_MANHACK,
		SCHED_HONGOREBEL_ADVANCE,
		SCHED_HONGOREBEL_CHARGE,
		SCHED_HONGOREBEL_BURNING_RUN,
		SCHED_HONGOREBEL_BURNING_STAND,
		SCHED_HONGOREBEL_INVESTIGATE_SOUND,
		SCHED_HONGOREBEL_SHOVE,
		SCHED_HONGOREBEL_ACTIVATE_BATON,
		SCHED_HONGOREBEL_DEACTIVATE_BATON,
		SCHED_HONGOREBEL_ALERT_FACE_BESTSOUND,
		SCHED_HONGOREBEL_SMASH_PROP,

		SCHED_HONGOREBEL_HEAL,
		SCHED_HONGOREBEL_HEAL_TOSS,
	};

	enum 
	{
		TASK_HONGOREBEL_HARASS = BaseClass::NEXT_TASK,
		TASK_HONGOREBEL_DIE_INSTANTLY,
		TASK_HONGOREBEL_GET_PATH_TO_BESTSOUND_LOS,
		TASK_HONGOREBEL_ACTIVATE_BATON,
		TASK_HONGOREBEL_WAIT_FOR_SENTENCE,

		TASK_CP_HEAL,
		TASK_CP_HEAL_TOSS,
	};

private:

	int				m_iPistolClips;		// How many clips the cop has in reserve
	int				m_iManhacks;		// How many manhacks the cop has
	bool			m_fWeaponDrawn;		// Is my weapon drawn? (ready to use)
	bool			m_bSimpleCops;		// The easy version of the cops
	int				m_LastShootSlot;
	CRandSimTimer	m_TimeYieldShootSlot;
	CSimpleSimTimer m_BatonSwingTimer;
	CSimpleSimTimer m_NextChargeTimer;


	float			m_flTaskCompletionTime;
	
	bool			m_bShouldActivateBaton;
	float			m_flBatonDebounceTime;	// Minimum amount of time before turning the baton off
	float			m_flLastPhysicsFlinchTime;
	float			m_flLastDamageFlinchTime;
	
	// Sentences
	float			m_flNextPainSoundTime;
	float			m_flNextLostSoundTime;
	int				m_nIdleChatterType;
	bool			m_bPlayerIsNear;

	// Outputs
	COutputEvent	m_OnCupCopped;

	AIHANDLE		m_hManhack;
	CHandle<CPhysicsProp>	m_hBlockingProp;

	CAI_FuncTankBehavior	m_FuncTankBehavior;
	CAI_RappelBehavior		m_RappelBehavior;
	CAI_PolicingBehavior	m_PolicingBehavior;
	CAI_RechargeBehavior	m_RechargeBehavior;

	CHandle<CAI_FollowGoal>	m_hSavedFollowGoalEnt;

	bool					m_bNotifyNavFailBlocked;
	bool					m_bNeverLeavePlayerSquad; // Don't leave the player squad unless killed, or removed via Entity I/O. 

	CAI_Sentence< CNPC_HongoRebel > m_Sentences;

	int				m_nRecentDamage;
	float			m_flRecentDamageTime;

	// The last hit direction, measured as a yaw relative to our orientation
	float			m_flLastHitYaw;

	static float	gm_flTimeLastSpokePeek;

// TERO: EVERYTHING THAT FOLLOWS FROM HERE IS COPIED FROM THE CITIZEN COMMANDABLE STUFF:

private:

	float			m_flNextHealthSearchTime;

	bool			m_bMedkitHidden;

	float			m_flPlayerHealTime;
	float			m_flAllyHealTime;
	float			m_flPlayerGiveAmmoTime;
	string_t		m_iszAmmoSupply;
	int				m_iAmmoAmount;
	string_t		m_iszOriginalSquad;
	float			m_flTimeJoinedPlayerSquad;
	bool			m_bWasInPlayerSquad;
	float			m_flTimeLastCloseToPlayer;
	string_t		m_iszDenyCommandConcept;

	CSimpleSimTimer	m_AutoSummonTimer;
	Vector			m_vAutoSummonAnchor;

	static CSimpleSimTimer gm_PlayerSquadEvaluateTimer;

	float			m_flTimePlayerStare;	// The game time at which the player started staring at me.
	float			m_flTimeNextHealStare;	// Next time I'm allowed to heal a player who is staring at me.

	//-----------------------------------------------------
	//	Outputs
	//-----------------------------------------------------
	COutputEvent		m_OnJoinedPlayerSquad;
	COutputEvent		m_OnLeftPlayerSquad;
	COutputEvent		m_OnFollowOrder;
	COutputEvent		m_OnStationOrder; 
	COutputEvent		m_OnPlayerUse;
	COutputEvent		m_OnNavFailBlocked;

public:

	void				UpdatePlayerGiveAmmoTime(float flGiveAmmoTime);

	void				PostNPCInit();
	void				OnRestore();
	bool 				ShouldAlwaysThink();
	bool				ShouldBehaviorSelectSchedule( CAI_BehaviorBase *pBehavior );
	int					SelectSchedulePriorityAction();
	int					SelectScheduleHeal();
	bool				ShouldDeferToFollowBehavior();

	void				PickupItem( CBaseEntity *pItem );
	int					SelectScheduleRetrieveItem();

	void 				OnChangeActiveWeapon( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon );
	void				GiveWeapon( string_t iszWeaponName );

	bool				ShouldLookForBetterWeapon();

	void				PredictPlayerPush();

	bool				ShouldAcceptGoal( CAI_BehaviorBase *pBehavior, CAI_GoalEntity *pGoal );
	void				OnClearGoal( CAI_BehaviorBase *pBehavior, CAI_GoalEntity *pGoal );
	void				OnAnimEventHeal();
	void				TaskFail( AI_TaskFailureCode_t code );

	bool 				IsMedic() 			{ return HasSpawnFlags(SF_HONGOREBEL_MEDIC); }
	bool 				IsAmmoResupplier() 	{ return HasSpawnFlags(SF_HONGOREBEL_AMMORESUPPLIER); }
	
	bool 				CanHeal();
	bool 				ShouldHealTarget( CBaseEntity *pTarget, bool bActiveUse = false );
	bool				ShouldHealItself();
	bool 				ShouldHealTossTarget( CBaseEntity *pTarget, bool bActiveUse = false );
	void 				Heal();

	bool				ShouldLookForHealthItem();

	void				TossHealthKit( CBaseCombatCharacter *pThrowAt, const Vector &offset ); // create a healthkit and throw it at someone
	void				InputForceHealthKitToss( inputdata_t &inputdata );

	
	//---------------------------------
	// Inputs
	//---------------------------------
	void			InputRemoveFromPlayerSquad( inputdata_t &inputdata ) { RemoveFromPlayerSquad(); }
	void			InputSetCommandable( inputdata_t &inputdata );
	void			InputSetNotCommandable( inputdata_t &inputdata );
	void			InputSetMedicOn( inputdata_t &inputdata );
	void			InputSetMedicOff( inputdata_t &inputdata );
	void			InputSetAmmoResupplierOn( inputdata_t &inputdata );
	void			InputSetAmmoResupplierOff( inputdata_t &inputdata );

	void			InputSetRechargerOn( inputdata_t &inputdata );
	void			InputSetRechargerOff( inputdata_t &inputdata );

	//---------------------------------
	// Commander mode
	//---------------------------------
	bool 			IsCommandable();
	bool			IsPlayerAlly( CBasePlayer *pPlayer = NULL );
	bool			CanJoinPlayerSquad();
	bool			WasInPlayerSquad();
	bool			HaveCommandGoal() const;
	bool			IsCommandMoving();
	bool			ShouldAutoSummon();
	bool 			IsValidCommandTarget( CBaseEntity *pTarget );
	bool 			NearCommandGoal();
	bool 			VeryFarFromCommandGoal();
	bool 			TargetOrder( CBaseEntity *pTarget, CAI_BaseNPC **Allies, int numAllies );
	void 			MoveOrder( const Vector &vecDest, CAI_BaseNPC **Allies, int numAllies );
	void			OnMoveOrder();
	void 			CommanderUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	bool			ShouldSpeakRadio( CBaseEntity *pListener );
	void			OnMoveToCommandGoalFailed();
	void			AddToPlayerSquad();
	void			RemoveFromPlayerSquad();
	void 			TogglePlayerSquadState();
	void			UpdatePlayerSquad();
	static int __cdecl PlayerSquadCandidateSortFunc( const SquadCandidate_t *, const SquadCandidate_t * );
	void 			FixupPlayerSquad();
	void 			ClearFollowTarget();
	void 			UpdateFollowCommandPoint();
	bool			IsFollowingCommandPoint();
	CAI_BaseNPC *	GetSquadCommandRepresentative();
	void			SetSquad( CAI_Squad *pSquad );
	void			AddInsignia();
	void			RemoveInsignia();

//TERO: UNIQUE CIVIL PROTECTION OFFICERS
protected:

	enum unique_cps 
	{
		HONGOREBEL_NORMAL = 0,
		HONGOREBEL_LARSON,
		HONGOREBEL_NOAH,
		HONGOREBEL_ELOISE,
	};

	int		m_iUniqueHongorebel;

	inline bool IsUnique() { return (m_iUniqueHongorebel != HONGOREBEL_NORMAL); }
	inline bool IsEloise() { return (m_iUniqueHongorebel == HONGOREBEL_ELOISE); }
	inline bool IsLarson() { return (m_iUniqueHongorebel == HONGOREBEL_LARSON); }
	inline bool IsNoah()   { return (m_iUniqueHongorebel == HONGOREBEL_NOAH); }

	bool	m_bCanRecharge;

public:
	DEFINE_CUSTOM_AI;
};

//---------------------------------------------------------
//---------------------------------------------------------
inline bool CNPC_HongoRebel::NearCommandGoal()
{
	const float flDistSqr = COMMAND_GOAL_TOLERANCE * COMMAND_GOAL_TOLERANCE;
	return ( ( GetAbsOrigin() - GetCommandGoal() ).LengthSqr() <= flDistSqr );
}

//---------------------------------------------------------
//---------------------------------------------------------
inline bool CNPC_HongoRebel::VeryFarFromCommandGoal()
{
	const float flDistSqr = (12*50) * (12*50);
	return ( ( GetAbsOrigin() - GetCommandGoal() ).LengthSqr() > flDistSqr );
}

class CSquadInsignia : public CBaseAnimating
{
	DECLARE_CLASS( CSquadInsignia, CBaseAnimating );
	void Spawn();
};


#endif // NPC_HONGOREBEL_H