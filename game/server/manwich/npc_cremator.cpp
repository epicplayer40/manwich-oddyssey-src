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
#include "ai_baseactor.h"
#include "movevars_shared.h"

#define CREMATOR_MODEL "models/cremator.mdl"
#define CREMATOR_MODEL_BODY "models/cremator_body.mdl" //The Cremator's head is gonna get blown off if the killing blow is a headshot - epicplayer
#define CREMATOR_MODEL_HEAD "models/cremator_head.mdl"

#define	ENVELOPE_CONTROLLER		(CSoundEnvelopeController::GetController())



ConVar	sk_cremator_health( "sk_cremator_health","0" );
ConVar	sk_cremator_limp_health( "sk_cremator_limp_health", "0" );
ConVar sk_cremator_maxrange("sk_cremator_max_range", "0");
ConVar sk_cremator_minrange("sk_cremator_min_range", "0");
ConVar sk_cremator_fuel("sk_cremator_fuel", "0");
ConVar sk_cremator_special_rangemultiplier("sk_cremator_special_rangemultiplier", "0");
ConVar sk_cremator_special_sphereradius("sk_cremator_special_sphereradius", "0");
ConVar sk_cremator_special_delay1("sk_cremator_special_delay1", "0");
ConVar sk_cremator_special_delay2("sk_cremator_special_delay2", "0");
ConVar sk_cremator_aim_speed("sk_cremator_aim_speed", "0");
ConVar sk_cremator_yaw_speed("sk_cremator_yaw_speed", "0");

extern ConVar sk_immolator_gravity;
extern ConVar immolator_debug;

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
class CNPC_CrematorManod : public CAI_BaseActor
{
	DECLARE_CLASS(CNPC_CrematorManod, CAI_BaseActor);
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

	void BridgeStartImmolating();
	void BridgeStopImmolating();

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
	virtual float	MaxYawSpeed( void );
	void CalculateMaxDistance();
	void RefillAmmo();
	void SetAim(const Vector& aimDir);
	void SetTurnActivity(void);
	void StartTask(const Task_t* pTask);
	virtual bool OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval );

private:

	int SelectCombatSchedule();
	enum
	{
		SCHED_CREMATOR_SPECIAL_ATTACK1 = BaseClass::NEXT_SCHEDULE, //Long range
		SCHED_CREMATOR_SPECIAL_ATTACK2, // Spray
		SCHED_CREMATOR_ESTABLISH_LINE_OF_FIRE, 
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

	float m_fNextPainSoundingTime;

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
	SetHullType( HULL_MEDIUM );
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	SetBloodColor( BLOOD_COLOR_RED );
	m_bloodColor = DONT_BLEED; //Emit Hunter blood - epicplayer
	m_iHealth	= sk_cremator_health.GetFloat();
	m_flFieldOfView	= 0.5; // indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState	= NPC_STATE_NONE;
	m_flDistTooFar = sk_cremator_maxrange.GetFloat();
	CreateBreatheSound();

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_TURN_HEAD | bits_CAP_MOVE_GROUND | bits_CAP_USE_WEAPONS | bits_CAP_AIM_GUN | bits_CAP_WEAPON_RANGE_ATTACK1 | bits_CAP_MOVE_SHOOT );

	NPCInit();

	// We need to bloat the absbox to encompass all the hitboxes
	Vector absMin = -Vector(100,100,0);
	Vector absMax = Vector(100,100,128);

	CollisionProp()->SetSurroundingBoundsType( USE_SPECIFIED_BOUNDS, &absMin, &absMax );
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
			RefillAmmo(); // To set it to sk_cremator_fuel
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
			BridgeStartImmolating();
		}
		return;
	}

	else if ( pEvent->event == AE_CREMATOR_TURNOFFHOSE) //Forcefully turn off immolator
	{
		if( pImmolator && pImmolator->IsImmolating())
		{
			BridgeStopImmolating();
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

	case EVENT_WEAPON_REFILL:
		if (GetImmolator())
		{
			RefillAmmo();
			ClearCondition(COND_LOW_PRIMARY_AMMO);
			ClearCondition(COND_NO_PRIMARY_AMMO);
			ClearCondition(COND_NO_SECONDARY_AMMO);
		}
		break;
	case EVENT_WEAPON_SMG1:
		if (GetImmolator())
		{
			break;
		}
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
	if (gpGlobals->curtime >= m_fNextPainSoundingTime)
	{
		EmitSound("NPC_Cremator.Pain");
		m_fNextPainSoundingTime = gpGlobals->curtime + random->RandomFloat(0.5, 1.2);
	}
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
	CBaseEntity* pEnemy = GetEnemy();
	CWeaponImmolator* pImmolator = GetImmolator();

	if (pImmolator && pEnemy)
	{
		Vector vecEnemyLKP = GetEnemyLKP();
		Vector vecEnemyOffset = pEnemy->WorldSpaceCenter() - pEnemy->GetAbsOrigin();

		if (vecEnemyLKP.z <= GetAbsOrigin().z)
		{
			VectorClear(vecEnemyOffset);
		}

		Vector delta = vecEnemyOffset + vecEnemyLKP - shootOrigin;
		if (immolator_debug.GetBool())
		{
			NDebugOverlay::Line(shootOrigin, shootOrigin + delta, 255, 0, 0, 0, 0.1);
		}
		Vector vecDir = delta.Normalized();
		VectorNormalize(vecDir);
		QAngle angles;
		VectorAngles(vecDir, angles);

		//
		// Using angle = arctan(( v^2 + sqrt( v^4 - g(gx^2 + 2yv^2)) / gx)
		//
		float x = sqrtf((delta.x * delta.x) + (delta.y * delta.y));
		float y = delta.z;
		float v = pImmolator->GetBeamVelocity();
		float g = GetCurrentGravity() * sk_immolator_gravity.GetFloat();
		float projectionAngle = RAD2DEG(atan2f(powf(v, 2) - sqrtf(powf(v, 4) - g * (g * powf(x, 2) + 2 * y * pow(v, 2))), g * x));
		if (projectionAngle != projectionAngle) //Invalid, cannot reach with the current velocity
		{
			projectionAngle = 45;
		}	
	
		Vector retVal;
		angles.x = -projectionAngle;
		angles.y += RandomFloat(-1.0f, 1.0f);
		AngleVectors(angles, &retVal);
		return retVal;
	}
	else
	{
		return BaseClass::GetShootEnemyDir(shootOrigin, bNoisy);
	}

}

int CNPC_CrematorManod::SelectSchedule()
{
	switch (m_NPCState)
	{
	case NPC_STATE_COMBAT:
		{
			return SelectCombatSchedule();
		}
	}
	// We fell through, no conditions met for attacking, stop firing
	BridgeStopImmolating();
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
	case SCHED_ESTABLISH_LINE_OF_FIRE:
		return SCHED_CREMATOR_ESTABLISH_LINE_OF_FIRE;
	case SCHED_HIDE_AND_RELOAD:
		return SCHED_RELOAD;		//Be Brave!
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
		BridgeStopImmolating();
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

		Vector absOrigin;
		pImmolator->GetAttachment(pImmolator->m_nMuzzleFlashAttachment, absOrigin);
		return absOrigin;
	}

	return BaseClass::Weapon_ShootPosition();
}

void CNPC_CrematorManod::BridgeStartImmolating()
{
	CWeaponImmolator* pImmolator = GetImmolator();
	if ( pImmolator && !pImmolator->IsImmolating() && !m_bDontHose)
	{
		pImmolator->StartImmolating();
	}
}

void CNPC_CrematorManod::BridgeStopImmolating()
{
	CWeaponImmolator* pImmolator = GetImmolator();
	if (pImmolator && pImmolator->IsImmolating())
	{
		pImmolator->StopImmolating();
	}
}

void CNPC_CrematorManod::CreateBreatheSound()
{
	CPASAttenuationFilter filter(this);
	m_pBreatheSound = ENVELOPE_CONTROLLER.SoundCreate(filter, entindex(), "NPC_Cremator.Breathe");
	ENVELOPE_CONTROLLER.Play(m_pBreatheSound, 1.0, PITCH_NORM);
}

void CNPC_CrematorManod::StartMadBreathe()
{
	if (!m_bMadBreathing)
	{
		m_bMadBreathing = true;
		CPASAttenuationFilter filter(this);
		ENVELOPE_CONTROLLER.SoundDestroy(m_pBreatheSound);
		m_pBreatheSound = ENVELOPE_CONTROLLER.SoundCreate(filter, entindex(), "NPC_Cremator.Breathe_Mad");
		ENVELOPE_CONTROLLER.Play(m_pBreatheSound, 1.0, PITCH_NORM);
	}
}

void CNPC_CrematorManod::StopLoopingSounds()
{
	ENVELOPE_CONTROLLER.SoundDestroy(m_pBreatheSound);
	m_pBreatheSound = NULL;
	if (GetActiveWeapon())
	{
		GetActiveWeapon()->StopLoopingSounds();
	}
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

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CNPC_CrematorManod::MaxYawSpeed( void )
{
	Activity eActivity = GetActivity();

	CBaseEntity *pEnemy = GetEnemy();
	GetAbsAngles();

	if ( pEnemy != NULL && pEnemy->IsPlayer() == false )
		return 16.0f;

	switch( eActivity )
	{	
	case ACT_SPECIAL_ATTACK1:
	case ACT_SPECIAL_ATTACK2:
		return 0.0f; //Don't turn while immolating
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		return 40.0f;
		break;
	case ACT_RANGE_ATTACK1:
		return sk_cremator_yaw_speed.GetFloat();
	case ACT_RUN:
	default:
		return 20.0f;
		break;
	}
}

int CNPC_CrematorManod::SelectCombatSchedule()
{
	CWeaponImmolator* pImmolator = GetImmolator();
	if (!pImmolator)
	{
		return BaseClass::SelectSchedule();
	}
	CalculateMaxDistance();
	pImmolator->CalculateMaxDistance();

	if (!HasCondition(COND_SEE_ENEMY))
	{
		return BaseClass::SelectSchedule();
	}

	if (!HasCondition(COND_NO_PRIMARY_AMMO))
	{
		if (HasCondition(COND_ENEMY_TOO_FAR))
		{
			CBaseEntity* pEnemy = GetEnemy();
			if (
				gpGlobals->curtime > m_fNextDirectedBeamTime &&
				EnemyDistance(pEnemy) < m_flDistTooFar * sk_cremator_special_rangemultiplier.GetFloat()
				)
			{
				m_fNextDirectedBeamTime = gpGlobals->curtime + sk_cremator_special_delay1.GetFloat();
				return SCHED_SPECIAL_ATTACK1;
			}
		}
		else if (HasCondition(COND_CAN_RANGE_ATTACK1))
		{
			if (gpGlobals->curtime > m_fNextSprayTime)
			{
				Vector delta = GetEnemyLKP() - GetAbsOrigin();
				if (m_bDontHose)
				{
					ClearCondition(COND_CAN_RANGE_ATTACK1);
					return BaseClass::SelectSchedule();
				}
				CBaseEntity* pEntityList[8];
				int entityCount = UTIL_EntitiesInSphere(pEntityList, 8, GetAbsOrigin(), sk_cremator_special_sphereradius.GetFloat(), FL_NPC | FL_CLIENT);
				int enemyCount = 0;
				for (int i = 0; i < entityCount; i++)
				{
					CBaseCombatCharacter* pCharacter = pEntityList[i]->MyCombatCharacterPointer();
					if (!pCharacter || pCharacter == this)
						continue;

					trace_t tr;
					UTIL_TraceLine(WorldSpaceCenter(), pCharacter->WorldSpaceCenter(), MASK_SHOT, this, COLLISION_GROUP_NPC, &tr);
					if (tr.fraction == 1.0 && FindEntityRelationship(pCharacter)->disposition == D_HT)
					{
						enemyCount++;
					}
					if (enemyCount > 1)
					{
						m_fNextSprayTime = gpGlobals->curtime + sk_cremator_special_delay2.GetFloat();
						return SCHED_SPECIAL_ATTACK2;
					}
				}
			}
			return BaseClass::SelectSchedule();
		}
	}
	BridgeStopImmolating();
	return BaseClass::SelectSchedule();
}

void CNPC_CrematorManod::CalculateMaxDistance()
{
	m_flDistTooFar = sk_cremator_maxrange.GetFloat();
}

void CNPC_CrematorManod::RefillAmmo()
{
	CBaseCombatWeapon* pWeapon = GetActiveWeapon();
	if (!pWeapon)
		return;
	SetAmmoCount(sk_cremator_fuel.GetInt(), pWeapon->m_iPrimaryAmmoType);
}

//-----------------------------------------------------------------------------
// Purpose: Set direction that the NPC aiming their gun
//-----------------------------------------------------------------------------

void CNPC_CrematorManod::SetAim(const Vector& aimDir)
{
	QAngle angDir;
	VectorAngles(aimDir, angDir);
	float curPitch = GetPoseParameter(m_poseAim_Pitch);
	float curYaw = GetPoseParameter(m_poseAim_Yaw);

	float newPitch;
	float newYaw;

	if (GetEnemy())
	{
		// clamp and dampen movement
		newPitch = curPitch + sk_cremator_aim_speed.GetFloat() * UTIL_AngleDiff(UTIL_ApproachAngle(angDir.x, curPitch, 20), curPitch);

		float flRelativeYaw = UTIL_AngleDiff(angDir.y, GetAbsAngles().y);
		float flNewTargetYaw = UTIL_ApproachAngle( flRelativeYaw, curYaw, 20 );
		newYaw = curYaw + sk_cremator_aim_speed.GetFloat() * UTIL_AngleDiff( flNewTargetYaw, curYaw );

	}
	else
	{
		// Sweep your weapon more slowly if you're not fighting someone
		newPitch = curPitch + 0.6 * UTIL_AngleDiff(UTIL_ApproachAngle(angDir.x, curPitch, 20), curPitch);

		float flRelativeYaw = UTIL_AngleDiff(angDir.y, GetAbsAngles().y);
		newYaw = curYaw + 0.6 * UTIL_AngleDiff(flRelativeYaw, curYaw);
	}

	newPitch = AngleNormalize(newPitch);
	newYaw = AngleNormalize(newYaw);

	SetPoseParameter(m_poseAim_Pitch, newPitch);
	SetPoseParameter(m_poseAim_Yaw, newYaw);

	// Msg("yaw %.0f (%.0f %.0f)\n", newYaw, angDir.y, GetAbsAngles().y );

	// Calculate our interaction yaw.
	// If we've got a small adjustment off our abs yaw, use that. 
	// Otherwise, use our abs yaw.
	if (fabs(newYaw) < 20)
	{
		m_flInteractionYaw = angDir.y;
	}
	else
	{
		m_flInteractionYaw = GetAbsAngles().y;
	}
}

//-----------------------------------------------------------------------------
// Start task!
//-----------------------------------------------------------------------------
void CNPC_CrematorManod::StartTask(const Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_STOP_MOVING:
		if ((GetNavigator()->IsGoalSet() && GetNavigator()->IsGoalActive()) || GetNavType() == NAV_JUMP)
		{
			DbgNavMsg(this, "Start TASK_STOP_MOVING\n");
			if (pTask->flTaskData == 1)
			{
				DbgNavMsg(this, "Initiating stopping path\n");
				GetNavigator()->StopMoving(false);
			}
			else
			{
				GetNavigator()->ClearGoal();
			}

			// E3 Hack
			if (HasPoseMoveYaw())
			{
				SetPoseParameter(m_poseMove_Yaw, 0);
			}
		}
		else
		{
			if (pTask->flTaskData == 1 && GetNavigator()->SetGoalFromStoppingPath())
			{
				DbgNavMsg(this, "Start TASK_STOP_MOVING\n");
				DbgNavMsg(this, "Initiating stopping path\n");
			}
			else
			{
				GetNavigator()->ClearGoal();
				TaskComplete();
			}
		}
		break;
	default:
		BaseClass::StartTask(pTask);
	}


}

//=========================================================
// SetTurnActivity - measures the difference between the way
// the NPC is facing and determines whether or not to
// select one of the 180 turn animations.
//=========================================================
void CNPC_CrematorManod::SetTurnActivity(void)
{
	//TODO: Turning gesture?
}

//-----------------------------------------------------------------------------
// Purpose: Override how we face our target as we move
// Output :
//-----------------------------------------------------------------------------
bool CNPC_CrematorManod::OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval )
{
  	Vector		vecFacePosition = vec3_origin;
	CBaseEntity	*pFaceTarget = NULL;
	bool		bFaceTarget = false;

	if ( GetEnemy() && (IsCurSchedule( SCHED_CREMATOR_ESTABLISH_LINE_OF_FIRE ) || IsCurSchedule( SCHED_CHASE_ENEMY ) ) )
	{
		// Always face our enemy when trying to attack
		vecFacePosition = GetEnemy()->EyePosition();
		pFaceTarget = GetEnemy();
		bFaceTarget = true;
	}
	else if ( GetEnemy() && GetNavigator()->GetMovementActivity() == ACT_RUN )
  	{
		Vector vecEnemyLKP = GetEnemyLKP();
		
		// Only start facing when we're close enough
		if ( ( UTIL_DistApprox( vecEnemyLKP, GetAbsOrigin() ) < 512 ) || IsCurSchedule( SCHED_COMBAT_FACE ) )
		{
			vecFacePosition = vecEnemyLKP;
			pFaceTarget = GetEnemy();
			bFaceTarget = true;
		}
	}

	// Face
	if ( bFaceTarget )
	{
		AddFacingTarget( pFaceTarget, vecFacePosition, 1.0, 0.2 );
	}

	return BaseClass::OverrideMoveFacing( move, flInterval );
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
	"	Interrupts"
	"		COND_HEAVY_DAMAGE"
);
DEFINE_SCHEDULE
(
	SCHED_CREMATOR_SPECIAL_ATTACK2,

	"	Tasks"
	"		TASK_STOP_MOVING		0"
//	"		TASK_FACE_ENEMY			0"
	"		TASK_SPECIAL_ATTACK2	0"
	""
);

//=========================================================
// ESTABLISH_LINE_OF_FIRE
//
//  Go to a location from which I can shoot my enemy
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_CREMATOR_ESTABLISH_LINE_OF_FIRE,

	"	Tasks "
	"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_ESTABLISH_LINE_OF_FIRE_FALLBACK"
	"		TASK_GET_PATH_TO_ENEMY_LOS		0"
	"		TASK_SPEAK_SENTENCE				1"
	"		TASK_RUN_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_COMBAT_FACE"
	""
	"	Interrupts "
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_SEE_ENEMY"
	"		COND_LOST_ENEMY"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_RANGE_ATTACK2"
	"		COND_CAN_MELEE_ATTACK2"
	"		COND_HEAR_DANGER"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
);
AI_END_CUSTOM_NPC()	