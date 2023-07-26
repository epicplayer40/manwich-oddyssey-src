//========= Totally Not Copyright © 2023, Valve Cut Content, All rights reserved. ============//
//Who are combine janitor
//They are Well known as Cremators
//Their Job was Cleaning Human dead body and XEN creatures in streets with his flame thrower
//His head was made by Child labor without their willing
//Cremator are the Cut enemy from Half - life 2
//but, they still exist in half - life universe
//Cremator become famous among THE GAMERS
//So, Valve Decided to Bring back Cremator in Half - life alyx
//But, again Cremators was Cut From Game
//
//=============================================================================//
#include "cbase.h"
#include "ai_default.h"
#include "ai_task.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "ai_squad.h"
#include "soundent.h"
#include "game.h"
#include "npcevent.h"
#include "entitylist.h"
#include "activitylist.h"
#include "ai_basenpc.h"
#include "engine/IEngineSound.h"
#include "particle_parse.h"
#include "te_particlesystem.h"
#include "weapon_immolator.h"
#include "soundenvelope.h"

#define CREMATOR_MODEL "models/cremator.mdl"
#define CREMATOR_MODEL_BODY "models/cremator_body.mdl" //The Cremator's head is gonna get blown off if the killing blow is a headshot - epicplayer
#define CREMATOR_MODEL_HEAD "models/cremator_head.mdl"

#define	ENVELOPE_CONTROLLER		(CSoundEnvelopeController::GetController())



ConVar	sk_cremator_health( "sk_cremator_health","200" );
ConVar	sk_cremator_limp_health( "sk_cremator_limp_health", "100" );

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
// Private animevents
//=========================================================
int AE_CREMATOR_TURNONHOSE;
int AE_CREMATOR_TURNOFFHOSE;



//=========================================================
// Private activities
//=========================================================
Activity ACT_CREMATOR_GUN_JAM;

//=========================================================
//=========================================================
class CNPC_CrematorManod : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_CrematorManod, CAI_BaseNPC );
	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;

public:
	void	Precache( void );
	void	Spawn( void );
	Class_T Classify( void );

	void HandleAnimEvent( animevent_t *pEvent );
	int  OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo );
	void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );
	void PainSound(const CTakeDamageInfo &info);
	void DeathSound( const CTakeDamageInfo &info );
	void AlertSound() OVERRIDE;
	Activity NPC_TranslateActivity( Activity eNewActivity );
	void PostNPCInit();
	Vector		GetShootEnemyDir(const Vector& shootOrigin, bool bNoisy = true) OVERRIDE;
	Vector		Weapon_ShootPosition()	OVERRIDE;		// gun position at current position/orientation

	bool	IsImmolatable(void) OVERRIDE { return false; } 

	void BridgeStartImmolating(CWeaponImmolator* pImmolator);
	void BridgeStopImmolating(CWeaponImmolator* pImmolator);

	int SelectSchedule() OVERRIDE;
	int TranslateSchedule(int scheduleType) OVERRIDE;

	CWeaponImmolator* GetImmolator() const;

	void InputEnableHose(inputdata_t& data);
	void InputDisableHose(inputdata_t& data);

	CSoundPatch* m_pBreatheSound;
	bool m_bMadBreathing;
	void CreateBreatheSound();
	void StartMadBreathe();
	void DampenBreathing();
	void StopLoopingSounds() OVERRIDE;

private:
	enum
	{
		SCHED_CREMATOR_SPECIAL_ATTACK1 = BaseClass::NEXT_SCHEDULE, //Long range
		SCHED_CREMATOR_SPECIAL_ATTACK2, // Spray
		NEXT_SCHEDULE
	};

	enum HitFeedback_t
	{
		LOWER = -1,
		NONE,
		HIGHER = 1,
		OBSTRUCTED,
	};

private:
	//Combat vars
	float m_fNextDirectedBeamTime;
	float m_fNextSprayTime;
	float m_fPrevHitOffsetDistance;
	bool m_bDontHose;


	HitFeedback_t m_eHitFeedback;
};

LINK_ENTITY_TO_CLASS( npc_cremator, CNPC_CrematorManod );

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_CrematorManod )

	DEFINE_SOUNDPATCH(m_pBreatheSound),
	DEFINE_FIELD(m_fNextDirectedBeamTime, FIELD_TIME),
	DEFINE_FIELD(m_fNextSprayTime, FIELD_TIME),
	DEFINE_FIELD(m_bDontHose, FIELD_BOOLEAN),
	

	DEFINE_INPUTFUNC(FIELD_VOID,"EnableHose", InputEnableHose),
	DEFINE_INPUTFUNC(FIELD_VOID,"DisableHose", InputDisableHose),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CNPC_CrematorManod::Precache( void )
{
	PrecacheModel( CREMATOR_MODEL );
	PrecacheModel( CREMATOR_MODEL_BODY );
	PrecacheModel( CREMATOR_MODEL_HEAD );
	PrecacheScriptSound( "NPC_Cremator.FootstepLeft" ); //footsteps
	PrecacheScriptSound( "NPC_Cremator.FootstepRight" );
	PrecacheScriptSound( "NPC_Cremator.CloakSwish" ); //triggered by anim events
	PrecacheScriptSound( "NPC_Cremator.Pain" );
	PrecacheScriptSound( "NPC_Cremator.Die" );
	PrecacheScriptSound( "NPC_Cremator.SeeEnemy" ); //see's hostile enemy
	PrecacheScriptSound( "NPC_Cremator.SeeObject" ); //target is a bullseye for scripted areas
	PrecacheScriptSound( "NPC_Cremator.Breathe" ); //constant breathing sound
	PrecacheScriptSound( "NPC_Cremator.Breathe_Mad" ); //breathing when limping
	PrecacheScriptSound( "Weapon_Immolator.Start" );
	PrecacheScriptSound( "Weapon_Immolator.Off" );
	PrecacheScriptSound( "Weapon_Immolator.Run" );

	PrecacheParticleSystem( "blood_impact_synth_01" );

	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CNPC_CrematorManod::Spawn( void )
{
	Precache();

	SetModel( CREMATOR_MODEL );
	SetHullType(HULL_MEDIUM);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	SetBloodColor( BLOOD_COLOR_RED );
	m_bloodColor = DONT_BLEED; //Emit Hunter blood - epicplayer
	m_iHealth	= sk_cremator_health.GetFloat();
	m_flFieldOfView	= 0.5; // indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState	= NPC_STATE_NONE;
	CreateBreatheSound();

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_TURN_HEAD | bits_CAP_MOVE_GROUND | bits_CAP_USE_WEAPONS | bits_CAP_AIM_GUN | bits_CAP_WEAPON_RANGE_ATTACK1 | bits_CAP_MOVE_SHOOT );

	NPCInit();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CrematorManod::PostNPCInit()
{
	CBaseCombatWeapon *pWeapon = GetActiveWeapon();
		// Give a warning if a Cremator is equipped with anything other than
		// an immolator. 
		if( !GetActiveWeapon() /*|| !FClassnameIs( GetActiveWeapon(), "weapon_immolator" )*/ )
		{
			DevWarning("**Cremator spawned without weapon, giving Immolator\n");
			pWeapon = (CBaseCombatWeapon *)CREATE_UNSAVED_ENTITY(CWeaponImmolator, "weapon_immolator");
			DispatchSpawn(pWeapon);
			pWeapon->Activate();
			Weapon_Equip(pWeapon);

		}

	BaseClass::PostNPCInit();
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
// Output : 
//-----------------------------------------------------------------------------
Class_T	CNPC_CrematorManod::Classify( void )
{
	return	CLASS_COMBINE;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_CrematorManod::HandleAnimEvent( animevent_t *pEvent )
{
	CWeaponImmolator* pImmolator	= GetImmolator();

	if ( pEvent->event == AE_CREMATOR_TURNONHOSE) //Forcefully turn on the piss hose, have some kinda emergency fallback if the turnoff never triggers
	{
		if (pImmolator && !pImmolator->IsImmolating())
		{
			BridgeStartImmolating(pImmolator);
		}
		return;
	}

	if ( pEvent->event == AE_CREMATOR_TURNOFFHOSE) //Forcefully turn off immolator
	{
		if( pImmolator && pImmolator->IsImmolating())
		{
			BridgeStopImmolating(pImmolator);
		}
		return;
	}

	switch( pEvent->event )
	{
	case NPC_EVENT_LEFTFOOT:
		{
			EmitSound( "NPC_Cremator.FootstepLeft", pEvent->eventtime );
		}
		break;

	case NPC_EVENT_RIGHTFOOT:
		{
			EmitSound( "NPC_Cremator.FootstepRight", pEvent->eventtime );
		}
		break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// TakeDamage - get provoked when injured
//=========================================================

int CNPC_CrematorManod::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	CTakeDamageInfo newInfo = info;
	if( newInfo.GetDamageType() & DMG_BURN)
	{
		DevMsg( "Cremator is immune to fire damage!\n" ); //doesn't currently work, cremator shouldn't be ignitable but could be killed by direct plasma damage from other immolators - epicplayer
		return 0;
	}	

	CBaseEntity* inflictor = info.GetInflictor(); 
	CBaseEntity* owner = info.GetInflictor()->GetOwnerEntity();
	if (inflictor && owner  == this )
		return 0;

	int nDamageTaken = BaseClass::OnTakeDamage_Alive(info);

	if (GetHealth() < sk_cremator_limp_health.GetFloat()) //Lychy: Breathe mad if less than half health
	{
		StartMadBreathe();
	}

	return nDamageTaken;
}


void CNPC_CrematorManod::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{

	if ( info.GetDamageType() & DMG_SHOCK )
	 return;

	// Even though the damage might not hurt us, we want to react to it
	// if it's from the player.
	if ( info.GetAttacker()->IsPlayer() )
	{
		if ( !HasMemory( bits_MEMORY_PROVOKED ) )
		{
			GetEnemies()->ClearMemory( info.GetAttacker() );
			Remember( bits_MEMORY_PROVOKED );
			PainSound(info);
			SetCondition( COND_HEAVY_DAMAGE );
		}
	}

	// Makes the Cremator emit hunter damage particles - epicplayer
	if ( ( info.GetDamageType() & DMG_BULLET ) ||
		 ( info.GetDamageType() & DMG_BUCKSHOT ) ||
		 ( info.GetDamageType() & DMG_CLUB ) ||
		 ( info.GetDamageType() & DMG_NEVERGIB ) )
	{
		QAngle vecAngles;
		VectorAngles( ptr->plane.normal, vecAngles );
		DispatchParticleEffect( "blood_impact_synth_01", ptr->endpos, vecAngles );
	}

	BaseClass::TraceAttack( info, vecDir, ptr, pAccumulator );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CrematorManod::PainSound( const CTakeDamageInfo &info )
{
	DampenBreathing();
	EmitSound( "NPC_Cremator.Pain" );
}

//=========================================================
// DieSound
//=========================================================

void CNPC_CrematorManod::DeathSound( const CTakeDamageInfo &info )
{
	EmitSound( "NPC_Cremator.Die" );
}

//=========================================================
// SetActivity 
//=========================================================
Activity CNPC_CrematorManod::NPC_TranslateActivity( Activity eNewActivity )
{
	switch (eNewActivity)
	{
	case ACT_RUN:
		if (m_iHealth <= sk_cremator_limp_health.GetFloat())
		{
			// limp!
			return ACT_RUN_HURT;
		}
		else
		{
			return eNewActivity;
		}
		break;
	case ACT_WALK:
		if (m_iHealth <= sk_cremator_limp_health.GetFloat())
		{
			// limp!
			return ACT_RUN_HURT;
		}
		else
		{
			return eNewActivity;
		}
		break;
	case ACT_IDLE:
		if (m_NPCState == NPC_STATE_COMBAT)
		{
			eNewActivity = ACT_IDLE_ANGRY;
		}

		break;
	}

	return BaseClass::NPC_TranslateActivity(eNewActivity);
}
	
Vector	CNPC_CrematorManod::GetShootEnemyDir(const Vector& shootOrigin, bool bNoisy)
{
	//Extra stuff?
	return BaseClass::GetShootEnemyDir(shootOrigin, bNoisy);

}

int CNPC_CrematorManod::SelectSchedule()
{
	CWeaponImmolator* pImmolator = GetImmolator();
	switch (m_NPCState)
	{
	case NPC_STATE_COMBAT:
		if (pImmolator && HasCondition(COND_CAN_RANGE_ATTACK1))
		{
			pImmolator->m_fMaxRange1 *= 2; //Cremator can shoot further with his special attack
			Vector delta = GetEnemyLKP() - GetAbsOrigin();
			if (m_bDontHose)
			{
				ClearCondition(COND_CAN_RANGE_ATTACK1);
			}
			if (delta.IsLengthLessThan(1200))
			{
				CBaseEntity* pEntityList[8];
				int entityCount = UTIL_EntitiesInSphere(pEntityList, 8, GetAbsOrigin(), 1200, FL_NPC);
				int enemyCount = 0;
				for (int i = 0; i < entityCount && enemyCount < 2; i++)
				{
					CBaseEntity* pEntity = pEntityList[i];
					if (!pEntity)
						continue;
					CBaseCombatCharacter* pCharacter = pEntity->MyCombatCharacterPointer();
					if (!pCharacter || pCharacter == this)
						continue;

					if (FindEntityRelationship(pCharacter)->disposition == D_HT)
					{
						enemyCount++;
						continue;
					}
					else if (FindEntityRelationship(pCharacter)->disposition == D_LI)
					{
						break; //Dont Spray if theres someone we like
					}
				}

				if (enemyCount > 1 && gpGlobals->curtime > m_fNextSprayTime)
				{
					m_fNextSprayTime = gpGlobals->curtime + 20;
					pImmolator->m_bShootFar = false;
					return SCHED_SPECIAL_ATTACK2;
				}
			}
			else if (gpGlobals->curtime > m_fNextDirectedBeamTime)
			{
				m_fNextDirectedBeamTime = gpGlobals->curtime + 20;
				pImmolator->m_bShootFar = true;
				return SCHED_SPECIAL_ATTACK1;
			}
			pImmolator->m_bShootFar = false;
			pImmolator->m_fMaxRange1 /= 2; //Reset dat
			return BaseClass::SelectSchedule();
		}
		break;
	}
	if (pImmolator)
	{
		BridgeStopImmolating(pImmolator);
	}
	return BaseClass::SelectSchedule();
}

int CNPC_CrematorManod::TranslateSchedule(int scheduleType)
{
	switch (scheduleType)
	{
	case SCHED_SPECIAL_ATTACK1:
		return SCHED_CREMATOR_SPECIAL_ATTACK1;
	case SCHED_SPECIAL_ATTACK2:
		return SCHED_CREMATOR_SPECIAL_ATTACK2;
	}
	return BaseClass::TranslateSchedule(scheduleType);
}

void CNPC_CrematorManod::InputEnableHose(inputdata_t& data)
{
	m_bDontHose = false;
}

void CNPC_CrematorManod::InputDisableHose(inputdata_t& data)
{
	CWeaponImmolator* pImmolator = GetImmolator();
	if (pImmolator)
	{
		BridgeStopImmolating(pImmolator);
	}
	m_bDontHose = true;
}

CWeaponImmolator* CNPC_CrematorManod::GetImmolator(void) const
{
	return dynamic_cast<CWeaponImmolator*>(GetActiveWeapon());
}

//-----------------------------------------------------------------------------
// Purpose: Get shoot position of BCC at current position/orientation
// Input  :
// Output :
//-----------------------------------------------------------------------------
Vector CNPC_CrematorManod::Weapon_ShootPosition()
{
	CWeaponImmolator* pImmolator = GetImmolator();

	if (pImmolator)
	{
		if (CWeaponImmolator::m_nMuzzleAttachment == 0)
		{
			CWeaponImmolator::m_nMuzzleAttachment = pImmolator->LookupAttachment("muzzle");
		}
		Vector absOrigin;
		pImmolator->GetAttachment(CWeaponImmolator::m_nMuzzleAttachment, absOrigin);
		return absOrigin;
	}

	return BaseClass::Weapon_ShootPosition();
}

void CNPC_CrematorManod::BridgeStartImmolating(CWeaponImmolator* pImmolator)
{
	if (!m_bDontHose && !pImmolator->IsImmolating())
	{
		EmitSound("Weapon_Immolator.Start");
		pImmolator->m_fireState = FIRE_CHARGE;
		pImmolator->StartImmolating();
	}
}

void CNPC_CrematorManod::BridgeStopImmolating(CWeaponImmolator* pImmolator)
{
	if (pImmolator->IsImmolating())
	{
		EmitSound("Weapon_Immolator.Off");
		pImmolator->StopImmolating();
	}
}

void CNPC_CrematorManod::CreateBreatheSound()
{
	CPASAttenuationFilter filter(this);
	m_pBreatheSound = ENVELOPE_CONTROLLER.SoundCreate(filter, entindex(), CHAN_STATIC, "NPC_Cremator.Breathe", ATTN_NORM);
	ENVELOPE_CONTROLLER.Play(m_pBreatheSound, 1.0, PITCH_NORM);
}

void CNPC_CrematorManod::StartMadBreathe()
{
	if (!m_bMadBreathing)
	{
		m_bMadBreathing = true;
		CPASAttenuationFilter filter(this);
		ENVELOPE_CONTROLLER.SoundDestroy(m_pBreatheSound);
		m_pBreatheSound = ENVELOPE_CONTROLLER.SoundCreate(filter, entindex(), CHAN_STATIC, "NPC_Cremator.Breathe_Mad", ATTN_NORM);
		ENVELOPE_CONTROLLER.Play(m_pBreatheSound, 1.0, PITCH_NORM);
	}
}

void CNPC_CrematorManod::StopLoopingSounds()
{
	ENVELOPE_CONTROLLER.SoundDestroy(m_pBreatheSound);
	m_pBreatheSound = NULL;
	BaseClass::StopLoopingSounds();
}

void CNPC_CrematorManod::DampenBreathing()
{
	ENVELOPE_CONTROLLER.SoundChangeVolume(m_pBreatheSound, 0, 0);
	ENVELOPE_CONTROLLER.SoundChangeVolume(m_pBreatheSound, 1, 4);
}

void CNPC_CrematorManod::AlertSound()
{
	if (GetEnemy() && GetEnemy()->Classify() == CLASS_NONE)
	{
		EmitSound("NPC_Cremator.SeeObject");
	}
	else
	{
		EmitSound("NPC_Cremator.SeeEnemy");
	}
	DampenBreathing();
}

AI_BEGIN_CUSTOM_NPC(npc_cremator, CNPC_CrematorManod)
	DECLARE_ANIMEVENT(AE_CREMATOR_TURNONHOSE)
	DECLARE_ANIMEVENT(AE_CREMATOR_TURNOFFHOSE)


DEFINE_SCHEDULE
(
	SCHED_CREMATOR_SPECIAL_ATTACK1,

	"	Tasks"
	"		TASK_STOP_MOVING		0"
	"		TASK_FACE_ENEMY			0"
	"		TASK_SPECIAL_ATTACK1	0"
	""
);
DEFINE_SCHEDULE
(
	SCHED_CREMATOR_SPECIAL_ATTACK2,

	"	Tasks"
	"		TASK_STOP_MOVING		0"
	"		TASK_FACE_ENEMY			0"
	"		TASK_SPECIAL_ATTACK2	0"
	""
);
AI_END_CUSTOM_NPC()