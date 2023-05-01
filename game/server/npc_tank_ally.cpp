//========= Copyright © LOLOLOL, All rights reserved. ============//
//
// Purpose: VERY LOW BUDGET-ASS TANK
//
//
//=============================================================================//



#include "cbase.h"
#include "ai_task.h"
#include "ai_default.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "ai_motor.h"
#include "ai_memory.h"
#include "npc_combine.h"
#include "physics.h"
#include "bitstring.h"
#include "activitylist.h"
#include "game.h"
#include "npcevent.h"
#include "player.h"
#include "entitylist.h"
#include "ai_interactions.h"
#include "soundent.h"
#include "gib.h"
#include "shake.h"
#include "ammodef.h"
#include "Sprite.h"
#include "explode.h"
#include "grenade_homer.h"
#include "ai_basenpc.h"
#include "ai_squad.h"
#include "soundenvelope.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "te_effect_dispatch.h"
#include "te_particlesystem.h"

#include "npc_playercompanion.h"


#include 	<time.h>

#define TANKALLY_GIB_COUNT			5 

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar sk_tank_ally_health("sk_tank_ally_health", "800");
ConVar sk_tank_shell_velocity("sk_tank_shell_velocity", "1600");

#define CONCEPT_PLAYER_USE "FollowWhenUsed"

class CSprite;

extern void CreateConcussiveBlast(const Vector &origin, const Vector &surfaceNormal, CBaseEntity *pOwner, float magnitude);

#define	TANKALLY_MODEL	"models/tank_ally.mdl"

#define TANKALLY_MSG_SHOT	1
#define TANKALLY_MSG_SHOT_START 2

enum
{
	TANKALLY_REF_INVALID = 0,
	TANKALLY_REF_MUZZLE,
	TANKALLY_REF_LEFT_SHOULDER,
	TANKALLY_REF_HUMAN_HEAD,
	TANKALLY_REF_RIGHT_ARM_HIGH,
	TANKALLY_REF_RIGHT_ARM_LOW,
	TANKALLY_REF_LEFT_ARM_HIGH,
	TANKALLY_REF_LEFT_ARM_LOW,
	TANKALLY_REF_TORSO,
	TANKALLY_REF_LOWER_TORSO,
	TANKALLY_REF_RIGHT_THIGH_HIGH,
	TANKALLY_REF_RIGHT_THIGH_LOW,
	TANKALLY_REF_LEFT_THIGH_HIGH,
	TANKALLY_REF_LEFT_THIGH_LOW,
	//	TANKALLY_SHOVE,
	TANKALLY_REF_RIGHT_SHIN_HIGH,
	TANKALLY_REF_RIGHT_SHIN_LOW,
	TANKALLY_REF_LEFT_SHIN_HIGH,
	TANKALLY_REF_LEFT_SHIN_LOW,
	TANKALLY_REF_RIGHT_SHOULDER,

	NUM_TANKALLY_ATTACHMENTS,
};




class CNPC_TankAlly : public CAI_BaseNPC
{
	DECLARE_CLASS(CNPC_TankAlly, CAI_BaseNPC);
public:


	CNPC_TankAlly(void);

	int	OnTakeDamage_Alive(const CTakeDamageInfo &info);
	void Event_Killed(const CTakeDamageInfo &info);
	int	TranslateSchedule(int type);
	//int	MeleeAttack1Conditions(float flDot, float flDist);
	int	RangeAttack1Conditions(float flDot, float flDist);
	void Gib(void);
	void ShootMG(void);


	int		GetSoundInterests(void);

	void Precache(void);
	void Spawn(void);
	void PrescheduleThink(void);
	void TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, trace_t *ptr, int bitsDamageType);
	void HandleAnimEvent(animevent_t *pEvent);
	void StartTask(const Task_t *pTask);
	void RunTask(const Task_t *pTask);


	void			DoMuzzleFlash(void);


	void PainSound(const CTakeDamageInfo &info);
	void DeathSound(const CTakeDamageInfo &info);
	void AlertSound(void);



	float MaxYawSpeed(void);

	Class_T	Classify(void);

	Activity NPC_TranslateActivity(Activity baseAct);

	virtual int	SelectSchedule(void);


	void BuildScheduleTestBits(void);


	bool m_fOffBalance;
	float m_flNextClobberTime;

	int	GetReferencePointForBodyGroup(int bodyGroup);

	bool CreateBehaviors(void);

	void Shove(void);
	void FireRangeWeapon(void);

	CSprite *m_pGlowSprite[NUM_TANKALLY_ATTACHMENTS];


	int	m_nGibModel;

	int		m_iAmmoType;

	float m_flGlowTime;
	float m_flLastRangeTime;

	float m_aimYaw;
	float m_aimPitch;

	int	m_YawControl;
	int	m_PitchControl;
	int	m_MuzzleAttachment;

	DECLARE_DATADESC();

private:


	DEFINE_CUSTOM_AI;
};

void CNPC_TankAlly::PainSound(const CTakeDamageInfo &info)
{

	//EmitSound( "Hassault.Pain" );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_TankAlly::DeathSound(const CTakeDamageInfo &info)
{
	//EmitSound( "Hassault.Die" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_TankAlly::AlertSound(void)
{
	EmitSound("Tank.Alert");

}


void CNPC_TankAlly::ShootMG(void)
{
	Vector vecShootOrigin;

	//	UTIL_MakeVectors(pev->angles); Big fucking mess of vectors and angles here over trying to get the goddamn fucking rocket to shoot down or up depending on enemy's position fuck fuck fuck
	vecShootOrigin = GetAbsOrigin() + Vector(0, 0, 100);
	Vector vecShootDir = GetShootEnemyDir(vecShootOrigin);
	QAngle	angShootDir;
	Vector vecSrc, vecAim;
	vecAim = GetShootEnemyDir(vecSrc);
	QAngle	vecAngles;
	VectorAngles(vecShootDir, vecAngles);

	VectorNormalize(vecAim);


	CEffectData data;
	data.m_nAttachmentIndex = LookupAttachment("Muzzle");
	data.m_nEntIndex = entindex();
	GetAttachment("Muzzle", vecShootOrigin);

	CMissile *pRocket = (CMissile *)CBaseEntity::Create("rpg_missile", vecShootOrigin, vecAngles, this);
	//pRocket->SetAbsVelocity( vecAim * sk_tank_shell_velocity.GetFloat() );
	pRocket->SetAbsVelocity(vecShootDir * sk_tank_shell_velocity.GetFloat());
	pRocket->DumbFire();
	pRocket->SetNextThink(gpGlobals->curtime + 0.1f);

	SetAim(vecShootDir);
}

#define	TANKALLY_DEFAULT_ARMOR_HEALTH	50

#define	TANKALLY_MELEE1_RANGE	92
#define	TANKALLY_MELEE1_CONE	0.5f

#define	TANKALLY_RANGE1_RANGE	600
#define	TANKALLY_RANGE1_CONE	0.0f

#define	TANKALLY_GLOW_TIME			0.5f



BEGIN_DATADESC(CNPC_TankAlly)

DEFINE_FIELD(m_fOffBalance, FIELD_BOOLEAN),
DEFINE_FIELD(m_flNextClobberTime, FIELD_TIME),
DEFINE_ARRAY(m_pGlowSprite, FIELD_CLASSPTR, NUM_TANKALLY_ATTACHMENTS),
DEFINE_FIELD(m_nGibModel, FIELD_INTEGER),
DEFINE_FIELD(m_flGlowTime, FIELD_TIME),
DEFINE_FIELD(m_flLastRangeTime, FIELD_TIME),
DEFINE_FIELD(m_aimYaw, FIELD_FLOAT),
DEFINE_FIELD(m_aimPitch, FIELD_FLOAT),
DEFINE_FIELD(m_YawControl, FIELD_INTEGER),
DEFINE_FIELD(m_PitchControl, FIELD_INTEGER),
DEFINE_FIELD(m_MuzzleAttachment, FIELD_INTEGER),
DEFINE_FIELD(m_iAmmoType, FIELD_INTEGER),

END_DATADESC()

enum CombineGuardSchedules
{
	SCHED_TANKALLY_RANGE_ATTACK1 = LAST_SHARED_SCHEDULE,
	SCHED_TANKALLY_CLOBBERED,
	SCHED_TANKALLY_TOPPLE,
	SCHED_TANKALLY_HELPLESS,
};

enum CombineGuardTasks
{
	TASK_TANKALLY_RANGE_ATTACK1 = LAST_SHARED_TASK,
	TASK_TANKALLY_SET_BALANCE,
};

enum CombineGuardConditions
{
	COND_TANKALLY_CLOBBERED = LAST_SHARED_CONDITION,
};

int	ACT_TANKALLY_IDLE_TO_ANGRY;
int ACT_TANKALLY_CLOBBERED;
int ACT_TANKALLY_TOPPLE;
int ACT_TANKALLY_GETUP;
int ACT_TANKALLY_HELPLESS;

#define	TANKALLY_AE_SHOVE	11
#define	TANKALLY_AE_FIRE 12
#define	TANKALLY_AE_FIRE_START 13
#define	TANKALLY_AE_GLOW 14
#define TANKALLY_AE_LEFTFOOT 20
#define TANKALLY_AE_RIGHTFOOT	21
#define TANKALLY_AE_SHAKEIMPACT 22

CNPC_TankAlly::CNPC_TankAlly(void)
{
}

Class_T	CNPC_TankAlly::Classify(void)
{
	if (FClassnameIs(this, "npc_tank_ally"))
	{
		return	CLASS_CONSCRIPT;
	}

	if (FClassnameIs(this, "monster_green_tank"))
	{
		return	CLASS_CONSCRIPT;
	}

	if (FClassnameIs(this, "monster_tan_tank"))
	{
		return	CLASS_TEAM1;
	}

	if (FClassnameIs(this, "monster_grey_tank"))
	{
		return	CLASS_TEAM2;
	}

	if (FClassnameIs(this, "monster_blue_tank"))
	{
		return	CLASS_TEAM3;
	}

	if (FClassnameIs(this, "monster_red_soldier"))
	{
		return	CLASS_TEAM4;
	}

	return CLASS_NONE;
}

LINK_ENTITY_TO_CLASS(npc_tank_ally, CNPC_TankAlly);
LINK_ENTITY_TO_CLASS(monster_green_tank, CNPC_TankAlly);
LINK_ENTITY_TO_CLASS(monster_tan_tank, CNPC_TankAlly);
LINK_ENTITY_TO_CLASS(monster_grey_tank, CNPC_TankAlly);
LINK_ENTITY_TO_CLASS(monster_blue_tank, CNPC_TankAlly);
LINK_ENTITY_TO_CLASS(monster_red_tank, CNPC_TankAlly);

void CNPC_TankAlly::Precache(void)
{

	m_iAmmoType = GetAmmoDef()->Index("SMG1");

	PrecacheModel("models/tank_ally.mdl");
	PrecacheModel("models/plastic_tank.mdl");

	PrecacheModel("models/gibs/metal_gib1.mdl");

	PrecacheModel("sprites/blueflare1.vmt");

	PrecacheScriptSound("NPC_CombineGuard.FootstepLeft");
	PrecacheScriptSound("NPC_CombineGuard.FootstepRight");
	PrecacheScriptSound("Tank.FireGun");
	PrecacheScriptSound("Tank.PrimeGun");

	PrecacheScriptSound("Hassault.Pain");
	PrecacheScriptSound("Hassault.Die");
	PrecacheScriptSound("Tank.Alert");
	PrecacheScriptSound("Hassault.Idle");

	BaseClass::Precache();
}

void CNPC_TankAlly::Spawn(void)
{
	Precache();


	if (FClassnameIs(this, "npc_tank_ally"))
	{
		SetModel("models/tank_ally.mdl");
	}
	else
	{
		SetModel("models/Plastic_Tank.mdl");
	}

	UTIL_SetSize( this, Vector( -70.00, -145.00,    0.00 ), Vector( 70.00,  135.00,   80.00 ) );
	CollisionProp()->SetSurroundingBoundsType( USE_HITBOXES );


	SetHullType(HULL_LARGE);
	SetHullSizeNormal();

	SetNavType(NAV_GROUND);
	m_NPCState = NPC_STATE_NONE;
	SetBloodColor(BLOOD_COLOR_MECH);

	if (FClassnameIs(this, "monster_green_tank"))
	{
		SetRenderColor( 69, 156, 75, 255);
	}

	if (FClassnameIs(this, "monster_tan_tank"))
	{
		SetRenderColor( 171, 142, 54, 255);
	}

	if (FClassnameIs(this, "monster_blue_tank"))
	{
		SetRenderColor( 0, 64, 255, 255);
	}

	if (FClassnameIs(this, "monster_grey_tank"))
	{
		SetRenderColor( 150, 150, 150, 255);
	}

	if (FClassnameIs(this, "monster_red_tank"))
	{
		SetRenderColor( 255, 50, 50, 255);
	}


	m_iHealth = m_iMaxHealth = sk_tank_ally_health.GetFloat();
	m_flFieldOfView = 0.707;

	SetSolid(SOLID_BBOX);
	AddSolidFlags(FSOLID_NOT_STANDABLE);
	SetMoveType(MOVETYPE_STEP);


	m_flGlowTime = gpGlobals->curtime;
	m_flLastRangeTime = gpGlobals->curtime;
	m_aimYaw = 0;
	m_aimPitch = 0;

	m_flNextClobberTime = gpGlobals->curtime;

	CapabilitiesClear();
	CapabilitiesAdd(bits_CAP_MOVE_GROUND | bits_CAP_INNATE_RANGE_ATTACK1 | bits_CAP_AIM_GUN | bits_CAP_SQUAD | bits_CAP_NO_HIT_SQUADMATES | bits_CAP_FRIENDLY_DMG_IMMUNE);

	AddSpawnFlags( SF_NPC_LONG_RANGE | SF_NPC_ALWAYSTHINK );
	NPCInit();


	SetBodygroup(1, true);

	m_YawControl = LookupPoseParameter("aim_yaw");
	m_PitchControl = LookupPoseParameter("aim_pitch");
	m_MuzzleAttachment = LookupAttachment("muzzle");

	m_fOffBalance = false;

	BaseClass::Spawn();
}

int CNPC_TankAlly::GetSoundInterests(void)
{
	return	SOUND_WORLD |
		SOUND_COMBAT |
		SOUND_CARCASS |
		SOUND_MEAT |
		SOUND_GARBAGE |
		//		SOUND_DANGER	| Tank crew does not fear danger
		SOUND_PLAYER;
}


void CNPC_TankAlly::DoMuzzleFlash(void)
{
	BaseClass::DoMuzzleFlash();

	CEffectData data;

	data.m_nAttachmentIndex = LookupAttachment("Muzzle");
	data.m_nEntIndex = entindex();
	DispatchEffect("AirboatMuzzleFlash", data);
}


void CNPC_TankAlly::PrescheduleThink(void)
{
	BaseClass::PrescheduleThink();

	if (HasCondition(COND_TANKALLY_CLOBBERED))
	{
		Msg("CLOBBERED!\n");
	}

	for (int i = 1; i < NUM_TANKALLY_ATTACHMENTS; i++)
	{
		if (m_pGlowSprite[i] == NULL)
			continue;

		if (m_flGlowTime > gpGlobals->curtime)
		{
			float brightness;
			float perc = (m_flGlowTime - gpGlobals->curtime) / TANKALLY_GLOW_TIME;

			m_pGlowSprite[i]->TurnOn();

			brightness = perc * 92.0f;
			m_pGlowSprite[i]->SetTransparency(kRenderGlow, brightness, brightness, brightness, 255, kRenderFxNoDissipation);

			m_pGlowSprite[i]->SetScale(perc * 1.0f);
		}
		else
		{
			m_pGlowSprite[i]->TurnOff();
		}
	}


	Vector vecDamagePoint;
	QAngle vecDamageAngles;
}





void CNPC_TankAlly::Event_Killed(const CTakeDamageInfo &info)
{
	BaseClass::Event_Killed(info);

	UTIL_Remove(this);

	ExplosionCreate(GetAbsOrigin(), GetAbsAngles(), NULL, random->RandomInt(30, 40), 0, true);

	Gib();
}

void CNPC_TankAlly::Gib(void)
{
	// Sparks
	for (int i = 0; i < 4; i++)
	{
		Vector sparkPos = GetAbsOrigin();
		sparkPos.x += random->RandomFloat(-12, 12);
		sparkPos.y += random->RandomFloat(-12, 12);
		sparkPos.z += random->RandomFloat(-12, 12);
		g_pEffects->Sparks(sparkPos);
	}
	// Smoke
	UTIL_Smoke(GetAbsOrigin(), random->RandomInt(10, 15), 10);

	// Light
	CBroadcastRecipientFilter filter;
	te->DynamicLight(filter, 0.0,
		&GetAbsOrigin(), 255, 180, 100, 0, 100, 0.1, 0);

	// Throw gibs
	CGib::SpawnSpecificGibs(this, TANKALLY_GIB_COUNT, 800, 1000, "models/gibs/metal_gib1.mdl");


	UTIL_Remove(this);
}


void CNPC_TankAlly::HandleAnimEvent(animevent_t *pEvent)
{
	switch (pEvent->event)
	{
	case TANKALLY_AE_SHAKEIMPACT:
		Shove();
		UTIL_ScreenShake(GetAbsOrigin(), 25.0, 150.0, 1.0, 750, SHAKE_START);
		break;

	case TANKALLY_AE_LEFTFOOT:
	{
								 EmitSound("NPC_CombineGuard.FootstepLeft");
	}
		break;

	case TANKALLY_AE_RIGHTFOOT:
	{
								  EmitSound("NPC_CombineGuard.FootstepRight");
	}
		break;

	case TANKALLY_AE_SHOVE:
		Shove();
		break;

	case TANKALLY_AE_FIRE:
	{
							 m_flLastRangeTime = gpGlobals->curtime + 1.5f;
							 //FireRangeWeapon();
							 ShootMG();
							 DoMuzzleFlash();

							 EmitSound("Tank.FireGun");

							 EntityMessageBegin(this, true);
							 WRITE_BYTE(TANKALLY_MSG_SHOT);
							 MessageEnd();
	}
		break;

	case TANKALLY_AE_FIRE_START:
	{
								   EmitSound("Tank.PrimeGun");

								   EntityMessageBegin(this, true);
								   WRITE_BYTE(TANKALLY_MSG_SHOT_START);
								   MessageEnd();
	}
		break;

	case TANKALLY_AE_GLOW:
		m_flGlowTime = gpGlobals->curtime + TANKALLY_GLOW_TIME;
		break;

	default:
		BaseClass::HandleAnimEvent(pEvent);
		return;
	}
}

void CNPC_TankAlly::Shove(void)
{
	if (GetEnemy() == NULL)
		return;

	CBaseEntity *pHurt = NULL;

	Vector forward;
	AngleVectors(GetLocalAngles(), &forward);

	float flDist = (GetEnemy()->GetAbsOrigin() - GetAbsOrigin()).Length();
	Vector2D v2LOS = (GetEnemy()->GetAbsOrigin() - GetAbsOrigin()).AsVector2D();
	Vector2DNormalize(v2LOS);
	float flDot = DotProduct2D(v2LOS, forward.AsVector2D());

	flDist -= GetEnemy()->WorldAlignSize().x * 0.5f;

	if (flDist < TANKALLY_MELEE1_RANGE && flDot >= TANKALLY_MELEE1_CONE)
	{
		Vector vStart = GetAbsOrigin();
		vStart.z += WorldAlignSize().z * 0.5;

		Vector vEnd = GetEnemy()->GetAbsOrigin();
		vEnd.z += GetEnemy()->WorldAlignSize().z * 0.5;

		pHurt = CheckTraceHullAttack(vStart, vEnd, Vector(-16, -16, 0), Vector(16, 16, 24), 25, DMG_CLUB);
	}

	if (pHurt)
	{
		pHurt->ViewPunch(QAngle(-20, 0, 20));

		UTIL_ScreenShake(pHurt->GetAbsOrigin(), 100.0, 1.5, 1.0, 2, SHAKE_START);

		color32 white = { 255, 255, 255, 64 };
		UTIL_ScreenFade(pHurt, white, 0.5f, 0.1f, FFADE_IN);

		if (pHurt->IsPlayer())
		{
			Vector forward, up;
			AngleVectors(GetLocalAngles(), &forward, NULL, &up);
			pHurt->ApplyAbsVelocityImpulse(forward * 300 + up * 250);
		}
	}
}

int CNPC_TankAlly::SelectSchedule(void)
{

	switch (m_NPCState)
	{

	case NPC_STATE_ALERT:
	{
		if (HasCondition(COND_HEAR_DANGER) || HasCondition(COND_HEAR_COMBAT))
				{
						return SCHED_INVESTIGATE_SOUND;
				}
	}
		break;


	case NPC_STATE_COMBAT:
	{
							 // dead enemy
							 if (HasCondition(COND_ENEMY_DEAD))
								 return BaseClass::SelectSchedule(); // call base class, all code to handle dead enemies is centralized there.

							 // always act surprized with a new enemy
							 if (HasCondition(COND_NEW_ENEMY) && HasCondition(COND_LIGHT_DAMAGE))
								 return SCHED_SMALL_FLINCH;

							 //	if ( HasCondition( COND_HEAVY_DAMAGE ) )
							 //	 return SCHED_TAKE_COVER_FROM_ENEMY;

							 if (!HasCondition(COND_SEE_ENEMY))
							 {
								 // we can't see the enemy
								 if (!HasCondition(COND_ENEMY_OCCLUDED))
								 {
									 // enemy is unseen, but not occluded!
									 // turn to face enemy
									 return SCHED_COMBAT_FACE;
								 }
								 else
								 {
									 return SCHED_CHASE_ENEMY;
								 }
							 }

							 if (HasCondition(COND_SEE_ENEMY))
							 {
								 if (HasCondition(COND_TOO_FAR_TO_ATTACK))
								 {
									 return SCHED_CHASE_ENEMY;
								 }
							 }
		}
		break;

	case NPC_STATE_IDLE:
		if (HasCondition(COND_LIGHT_DAMAGE) || HasCondition(COND_HEAVY_DAMAGE))
		{
			// flinch if hurt
			return SCHED_SMALL_FLINCH;
		}

		if (GetEnemy() == NULL) //&& GetFollowTarget() )
		{
			{

				return SCHED_TARGET_FACE;
			}
		}

		if (HasCondition(COND_HEAR_DANGER) || HasCondition(COND_HEAR_COMBAT))
		{
						return SCHED_INVESTIGATE_SOUND;
		}

		// try to say something about smells
		break;
	}

	return BaseClass::SelectSchedule();
}


void CNPC_TankAlly::StartTask(const Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_TANKALLY_SET_BALANCE:
		if (pTask->flTaskData == 0.0)
		{
			m_fOffBalance = false;
		}
		else
		{
			m_fOffBalance = true;
		}

		TaskComplete();

		break;

	case TASK_TANKALLY_RANGE_ATTACK1:
	{
		Vector flEnemyLKP = GetEnemyLKP();
		GetMotor()->SetIdealYawToTarget(flEnemyLKP);
	}
		SetActivity(ACT_RANGE_ATTACK1);
		return;

	default:
		BaseClass::StartTask(pTask);
		break;
	}
}

void CNPC_TankAlly::RunTask(const Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_TANKALLY_RANGE_ATTACK1:
	{
		Vector flEnemyLKP = GetEnemyLKP();
		GetMotor()->SetIdealYawToTargetAndUpdate(flEnemyLKP);

		if (IsActivityFinished())
		{
			TaskComplete();
		return;
		}
	}
		return;
	}

	BaseClass::RunTask(pTask);
}


#define TRANSLATE_SCHEDULE( type, in, out ) { if ( type == in ) return out; }

Activity CNPC_TankAlly::NPC_TranslateActivity(Activity baseAct)
{

	if (baseAct == ACT_IDLE && m_NPCState != NPC_STATE_IDLE)
	{
		return (Activity)ACT_IDLE_ANGRY;
	}

	return baseAct;
}

int CNPC_TankAlly::TranslateSchedule(int type)
{
	switch (type)
	{
	case SCHED_RANGE_ATTACK1:
		return SCHED_TANKALLY_RANGE_ATTACK1;
		break;
	}

	return BaseClass::TranslateSchedule(type);
}





int CNPC_TankAlly::OnTakeDamage_Alive(const CTakeDamageInfo &info)
{
	CTakeDamageInfo newInfo = info;
	if (newInfo.GetDamageType() & DMG_BURN)
	{
		newInfo.ScaleDamage(0);
		DevMsg("Fire damage; no actual damage is taken\n");
	}

	if (newInfo.GetDamageType() & DMG_BULLET)
	{
		newInfo.ScaleDamage(0.2);
		DevMsg("Bullet Damage ; only 20% damage is taken\n");
	}


	int nDamageTaken = BaseClass::OnTakeDamage_Alive(newInfo);


	return nDamageTaken;
}


/*int CNPC_TankAlly::MeleeAttack1Conditions(float flDot, float flDist)
{
	if (flDist > TANKALLY_MELEE1_RANGE)
		return COND_TOO_FAR_TO_ATTACK;

	if (flDot < TANKALLY_MELEE1_CONE)
		return COND_NOT_FACING_ATTACK;

	return COND_CAN_MELEE_ATTACK1;
}
*/

int CNPC_TankAlly::RangeAttack1Conditions( float flDot, float flDist )
{
	if ( flDist > 6000 )
		return COND_TOO_FAR_TO_ATTACK;
	
	if ( flDot < 0.0f )
		return COND_NOT_FACING_ATTACK;

	//if ( m_flLastRangeTime > gpGlobals->curtime )
		//return COND_TOO_FAR_TO_ATTACK;

	return COND_CAN_RANGE_ATTACK1;
}


#define	DEBUG_AIMING 0



float CNPC_TankAlly::MaxYawSpeed(void)
{
	float flYS = 0;

	switch (GetActivity())
	{
	case	ACT_WALK:			flYS = 15;	break;
	case	ACT_RUN:			flYS = 15;	break;
	case	ACT_IDLE:			flYS = 15;	break;
	default:
		flYS = 30;
		break;
	}

	return flYS;
}

void CNPC_TankAlly::BuildScheduleTestBits(void)
{
	SetCustomInterruptCondition(COND_TANKALLY_CLOBBERED);
}


AI_BEGIN_CUSTOM_NPC(npc_tank_ally, CNPC_TankAlly)

DECLARE_TASK(TASK_TANKALLY_RANGE_ATTACK1)
DECLARE_TASK(TASK_TANKALLY_SET_BALANCE)

DECLARE_CONDITION(COND_TANKALLY_CLOBBERED)

DECLARE_ACTIVITY(ACT_TANKALLY_IDLE_TO_ANGRY)
DECLARE_ACTIVITY(ACT_TANKALLY_CLOBBERED)
DECLARE_ACTIVITY(ACT_TANKALLY_TOPPLE)
DECLARE_ACTIVITY(ACT_TANKALLY_GETUP)
DECLARE_ACTIVITY(ACT_TANKALLY_HELPLESS)

DEFINE_SCHEDULE
(
SCHED_TANKALLY_RANGE_ATTACK1,

"	Tasks"
"		TASK_STOP_MOVING			0"
"		TASK_FACE_ENEMY				0"
"		TASK_ANNOUNCE_ATTACK		1"
"		TASK_TANKALLY_RANGE_ATTACK1	0"
"	"
"	Interrupts"
"		COND_NEW_ENEMY"
"		COND_ENEMY_DEAD"
"		COND_NO_PRIMARY_AMMO"
)

DEFINE_SCHEDULE
(
SCHED_TANKALLY_CLOBBERED,

"	Tasks"
"		TASK_STOP_MOVING						0"
"		TASK_TANKALLY_SET_BALANCE			1"
"		TASK_PLAY_SEQUENCE						ACTIVITY:ACT_TANKALLY_CLOBBERED"
"		TASK_TANKALLY_SET_BALANCE			0"
"	"
"	Interrupts"
)

DEFINE_SCHEDULE
(
SCHED_TANKALLY_TOPPLE,

"	Tasks"
"		TASK_STOP_MOVING				0"
"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_TANKALLY_TOPPLE"
"		TASK_WAIT						1"
"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_TANKALLY_GETUP"
"		TASK_TANKALLY_SET_BALANCE	0"
"	"
"	Interrupts"
)

DEFINE_SCHEDULE
(
SCHED_TANKALLY_HELPLESS,

"	Tasks"
"	TASK_STOP_MOVING				0"
"	TASK_PLAY_SEQUENCE				ACTIVITY:ACT_TANKALLY_TOPPLE"
"	TASK_WAIT						2"
"	TASK_PLAY_SEQUENCE				ACTIVITY:ACT_TANKALLY_HELPLESS"
"	TASK_WAIT_INDEFINITE			0"
"	"
"	Interrupts"
)

AI_END_CUSTOM_NPC()



