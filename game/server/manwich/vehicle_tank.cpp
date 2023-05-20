//Lychy world...

#include "cbase.h"
#include "vehicle_base.h"
#include "in_buttons.h"
#include "explode.h"
#include "npc_vehicledriver.h"
#include "smoke_trail.h"
#include "saverestore_bitstring.h"
#include "IEffects.h"
#include "fire.h"
#include "gib.h"
#include "func_break.h"
#include "EntityFlame.h"
#include "vphysics/friction.h"
#include "particle_parse.h"
#include "soundent.h"

CBaseEntity* BreakModelCreateSingle(CBaseEntity* pOwner, breakmodel_t* pModel, const Vector& position, const QAngle& angles, const Vector& velocity, const AngularImpulse& angVelocity, int nSkin, const breakablepropparams_t& params);

ConVar sk_tank_shell_damage("sk_tank_shell_damage", "0");
ConVar sk_tank_health("sk_tank_health", "0");
ConVar tank_shell_speed("sk_tank_shell_speed", "10000");
ConVar tank_push_scalar("tank_push_scalar", "2000");
ConVar tank_passive_push_scalar("tank_passive_push_scalar", "1000");
ConVar tank_unstick_check_dist("tank_unstick_check_dist", "5");


ConVar tank_debug("tank_debug", "0");

#define JEEP_GUN_YAW				"vehicle_weapon_yaw"
#define JEEP_GUN_PITCH				"vehicle_weapon_pitch"

#define SMOKEPOINT_ATTACHMENT "smokepoint"

#define CANNON_MAX_UP_PITCH			70
#define CANNON_MAX_DOWN_PITCH		20#define CANNON_MAX_LEFT_YAW			180
#define CANNON_MAX_RIGHT_YAW		180

#define TANK_DEFAULTMODEL "models/vehicles/merkava.mdl"
#define TANK_GAMESOUND_FIRE "Tank.Single"
#define TANK_GAMESOUND_READYFIRE "Tank.ReadyFire"
#define TANK_GAMESOUND_EJECTCASING "Tank.EjectCasing"

#define TANK_GAMESOUND_TURRET_MOVE_START	"Tank.TurretMoveStart"
#define TANK_GAMESOUND_TURRET_MOVE_LOOP		"Tank.TurretMoveLoop"
#define TANK_GAMESOUND_TURRET_MOVE_END		"Tank.TurretMoveEnd"

#define TANK_GAMESOUND_TANKSHELL_FLYBY	"Tank.ShellFlyBy"

#define TANK_GAMESOUND_TREADS		"Tank.Treads"
#define TANK_GAMESOUND_DIE		"Tank.DieExplode"

#define TANK_GIB_TURRET_MODEL	"models/vehicles/merkava_tank_turret_gib.mdl"

#define TANK_CENTIPEDE_LEG_LENGTH 15.0f
#define TANK_CENTIPEDE_LEG_GAP 5

#define SF_INDESTRUCTIBLE 1 << 0

#pragma region Tank Shell
#define TANKSHELL_MODEL "models/weapons/tank_shell.mdl"
#define TANKCASING_MODEL "models/shell_casings/missilecasing01.mdl"

class CTankShell : public CBaseAnimating
{
	DECLARE_DATADESC();
	void Spawn();
	void Precache();
	void Touch(CBaseEntity* pOther);
	void Think();
	
	bool m_bHasSounded;

};

LINK_ENTITY_TO_CLASS(tank_shell, CTankShell);

BEGIN_DATADESC(CTankShell)
	DEFINE_THINKFUNC(Think),
END_DATADESC()

void CTankShell::Precache()
{
	PrecacheModel(TANKSHELL_MODEL);
	PrecacheScriptSound(TANK_GAMESOUND_TANKSHELL_FLYBY);
}

void CTankShell::Spawn()
{
	Precache();
	SetModel(TANKSHELL_MODEL);
	SetMoveType(MOVETYPE_FLYGRAVITY);
	SetSolid(SOLID_BBOX);
	SetEFlags(FL_OBJECT);
	


	SmokeTrail* pSmokeTrail = SmokeTrail::CreateSmokeTrail();
	pSmokeTrail->m_SpawnRate = 130;
	pSmokeTrail->m_ParticleLifetime = 2;
	pSmokeTrail->m_StartColor.Init(0.1, 0.1, 0.1);
	pSmokeTrail->m_EndColor.Init(0.5, 0.5, 0.5);
	pSmokeTrail->m_StartSize = 10;
	pSmokeTrail->m_EndSize = 50;
	pSmokeTrail->m_SpawnRadius = 1;
	//pSmokeTrail->m_MinSpeed = 15;
	//pSmokeTrail->m_MaxSpeed = 25;
	pSmokeTrail->SetLifetime(120);
	pSmokeTrail->FollowEntity(this);


	SetThink(&CTankShell::Think);
	SetNextThink(gpGlobals->curtime + 0.01f);
}

void CTankShell::Touch(CBaseEntity* pOther)
{
	if (!pOther->IsSolid())
		return;
	ExplosionCreate(WorldSpaceCenter(), vec3_angle, GetOwnerEntity(), 500, 500, true);
	UTIL_Remove(this);

}
void CTankShell::Think()
{
	QAngle angles;

	VectorAngles(GetAbsVelocity(), angles);
	SetAbsAngles(angles);

	CSoundEnt::InsertSound(SOUND_DANGER | SOUND_MOVE_AWAY, GetAbsOrigin(), 1024, 1.0f, this, SOUNDENT_CHANNEL_REPEATED_DANGER);
	
	if (!m_bHasSounded )
	{
		CBasePlayer* pPlayer = UTIL_GetLocalPlayer();
		vec_t distanceToPlayer = (pPlayer->GetAbsOrigin() - GetAbsOrigin()).Length();
		//Msg("Distance: %f\n", distanceToPlayer);
		if (distanceToPlayer <= 3000)
		{
			EmitSound(TANK_GAMESOUND_TANKSHELL_FLYBY);
			m_bHasSounded = true;
		}

	}
	SetNextThink(gpGlobals->curtime);
}
#pragma endregion

#pragma region Main Vehicle

class CVehicleTank : public CPropVehicleDriveable
{
	DECLARE_CLASS(CVehicleTank, CPropVehicleDriveable);
	DECLARE_DATADESC();
public:
	CVehicleTank()
	{
		EnableFiring();
	}
	void Spawn() OVERRIDE;
	void Precache() OVERRIDE;
	void PrecacheBreakModels();
	void Think() OVERRIDE;
	void Event_Killed(const CTakeDamageInfo& info);

	void FireCannon();
	void DriveVehicle(float flFrameTime, CUserCmd* ucmd, int iButtonsDown, int iButtonsReleased) OVERRIDE; // Driving Button handling
	void SetupMove(CBasePlayer* player, CUserCmd* ucmd, IMoveHelper* pHelper, CMoveData* move) OVERRIDE ;
	void CreateServerVehicle() OVERRIDE;
	void ExitVehicle(int nRole) OVERRIDE;

	void ShootThink();
	void AimGunAt(Vector* endPos);
	void NPCAimThink();
	void ReloadThink();
	void FinishReloadThink();
	void EjectCasingThink();

	void EnableFiring();
	void DisableFiring();

	void ResetViewForward();
	void EnableFire();
	bool ShouldNPCFire();

	int DrawDebugTextOverlays();

	int OnTakeDamage(const CTakeDamageInfo& info) OVERRIDE;
	int RandomSmokeEnable();
	int GetNumSmokePointAttachments();
	void SpawnFlamingGibsAtAttachment(int iAttachment);
	void EnableSmokeAttachment(int iAttachment);
	void SparkShowerAtAttachment(int iAttachment);
	void SpawnMegaGibs();

	bool CentipedeCheck(int iStartAttachment, int iEndAttachment);
	bool TraceCentipedeLeg(const Vector& startTrace);
	void InitialiseRaytraceAttachments();
	void ApplyPush(bool backward);
	void ApplyPassivePush(bool backward);
	void UnStickCheckThink();

	void StopLoopingSounds() OVERRIDE;

	CNPC_VehicleDriver* GetNPCDriver();
	
	float			m_aimYaw;
	float			m_aimPitch;

	float			m_flNextAllowedShootTime;
	float			m_flPrevAimTime;
	float			m_flPrevUnStickTime;

	bool			m_bIsFiring;
	bool			m_bIsGoodAimVector;
	bool			m_bIsResettingAim;
	bool			m_bIsOnFire;
	bool			m_bIsSounding;
	bool			m_bHasNotYetReloaded;
	

	int				m_RaytraceAttachmentRL;
	int				m_RaytraceAttachmentRR;
	int				m_RaytraceAttachmentFL;
	int				m_RaytraceAttachmentFR;

	CBitVec<8>		m_bitsSmoking;

	Vector			m_vecNPCTarget;
	Vector			m_vecPrevUnStickOrigin;
	Vector2D		m_vec2DPrevAimVector;

};

BEGIN_DATADESC(CVehicleTank)
	DEFINE_THINKFUNC(FinishReloadThink),
	DEFINE_THINKFUNC(ReloadThink),
	DEFINE_THINKFUNC(NPCAimThink),
	DEFINE_THINKFUNC(ShootThink),
	DEFINE_THINKFUNC(EjectCasingThink),
	DEFINE_THINKFUNC(UnStickCheckThink),
	DEFINE_FIELD(m_aimYaw, FIELD_FLOAT),
	DEFINE_FIELD(m_aimPitch, FIELD_FLOAT),
	DEFINE_FIELD(m_bIsGoodAimVector, FIELD_BOOLEAN),
	DEFINE_FIELD(m_bIsOnFire, FIELD_BOOLEAN),
	DEFINE_FIELD(m_bHasNotYetReloaded, FIELD_BOOLEAN),
	DEFINE_FIELD(m_vecNPCTarget, FIELD_VECTOR),
	DEFINE_BITSTRING(m_bitsSmoking),

END_DATADESC()

LINK_ENTITY_TO_CLASS(prop_vehicle_tank, CVehicleTank);



class CTankFourWheelServerVehicle : public CFourWheelServerVehicle
{
	typedef CFourWheelServerVehicle BaseClass;

public:

	CTankFourWheelServerVehicle()
	{
		m_flNextAimTime = 0.0f;
	}

	void Weapon_PrimaryRanges(float* flMinRange, float* flMaxRange) OVERRIDE
	{
		*flMinRange = 250.0f;
		*flMaxRange = 9999.0f;
	}
	bool NPC_IsOmniDirectional() OVERRIDE 
	{
		return true; 
	}
	CVehicleTank* GetTank() 
	{
		return m_hTank;
	}

	void CheckAndSetTank()
	{
		if (!GetTank())
			m_hTank = dynamic_cast<CVehicleTank*>(GetVehicleEnt());
	}
	
	void NPC_DriveVehicle() OVERRIDE
	{
		//If the tank is unable to get a shot, then dont try fire
		CheckAndSetTank();
		if(!GetTank()->ShouldNPCFire())
			m_nNPCButtons &= ~IN_ATTACK;

		BaseClass::NPC_DriveVehicle();
	}
	bool NPC_HasPrimaryWeapon() { return true; }

	void NPC_TurnCenter() OVERRIDE
	{
		m_flTurnDegrees = 0.0f;
		BaseClass::NPC_TurnCenter();
	}

	void NPC_AimPrimaryWeapon(Vector vecTarget) OVERRIDE
	{
		CheckAndSetTank();
		if (gpGlobals->curtime > m_flNextAimTime)
		{
			GetTank()->m_vecNPCTarget = vecTarget;
			m_flNextAimTime = gpGlobals->curtime + 0.25f;
		}
		GetTank()->NPCAimThink();
	}

	//Lychy: The tank should not move if the NPC driver has a good aiming vector
	void NPC_ThrottleReverse(void) OVERRIDE
	{	
		CheckAndSetTank();
		if (GetTank()->ShouldNPCFire())
			NPC_Brake();
		else
			BaseClass::NPC_ThrottleReverse();
	}

	void NPC_ThrottleForward(void) OVERRIDE
	{
		CheckAndSetTank();
		if (GetTank()->ShouldNPCFire())
			NPC_Brake();
		else
			BaseClass::NPC_ThrottleForward();
	}

	void NPC_TurnLeft(float flDegrees) OVERRIDE
	{	
		CheckAndSetTank();
		if (GetTank()->ShouldNPCFire())
			NPC_TurnCenter();
		else
			BaseClass::NPC_TurnLeft(flDegrees);
	}

	void NPC_TurnRight(float flDegrees) OVERRIDE
	{
		CheckAndSetTank();
		if (GetTank()->ShouldNPCFire())
			NPC_TurnCenter();
		else
			BaseClass::NPC_TurnRight(flDegrees);
	}

private:
	CHandle<CVehicleTank> m_hTank;
	float m_flNextAimTime;
};


void CVehicleTank::Precache()
{
	if (!GetModelName())
	{
		PrecacheModel("models/vehicles/merkava.mdl");
	}

	PrecacheScriptSound(TANK_GAMESOUND_EJECTCASING);
	PrecacheScriptSound(TANK_GAMESOUND_FIRE);
	PrecacheScriptSound(TANK_GAMESOUND_READYFIRE);

	PrecacheScriptSound(TANK_GAMESOUND_TURRET_MOVE_START);
	PrecacheScriptSound(TANK_GAMESOUND_TURRET_MOVE_LOOP);
	PrecacheScriptSound(TANK_GAMESOUND_TURRET_MOVE_END);

	PrecacheScriptSound(TANK_GAMESOUND_TREADS);
	PrecacheScriptSound(TANK_GAMESOUND_DIE);

	PrecacheParticleSystem("Explosion_2");
	PrecacheBreakModels();

	UTIL_PrecacheOther("env_fire");
	UTIL_PrecacheOther("env_smoketrail");
	UTIL_PrecacheOther("env_fire_trail");
	UTIL_PrecacheOther("tank_shell");
	
}
void CVehicleTank::Spawn()
{
	Precache();

	if (!m_vehicleScript)
	{
		m_vehicleScript = MAKE_STRING("scripts/vehicles/vehicle_tank.txt");
	}

	if (!GetModelName())
	{
		SetModel("models/vehicles/merkava.mdl");
	}

	SetPoseParameter(JEEP_GUN_YAW, 0);
	SetPoseParameter(JEEP_GUN_PITCH, 0);
	SetVehicleType(VEHICLE_TYPE_AIRBOAT_RAYCAST);

	RegisterThinkContext("FinishReloadThink");
	RegisterThinkContext("ReloadThink");
	RegisterThinkContext("ShootThink");
	RegisterThinkContext("NPCAimThink");
	RegisterThinkContext("EjectCasingThink");
	RegisterThinkContext("UnStickCheckThink");
	SetContextThink(&CVehicleTank::FinishReloadThink, TICK_NEVER_THINK, "FinishReloadThink");
	SetContextThink(&CVehicleTank::ReloadThink, TICK_NEVER_THINK, "ReloadThink");
	SetContextThink(&CVehicleTank::ShootThink, TICK_NEVER_THINK, "ShootThink");
	SetContextThink(&CVehicleTank::NPCAimThink, TICK_NEVER_THINK, "NPCAimThink");
	SetContextThink(&CVehicleTank::EjectCasingThink, TICK_NEVER_THINK, "EjectCasingThink");
	SetContextThink(&CVehicleTank::UnStickCheckThink, TICK_NEVER_THINK, "UnStickCheckThink");


	m_bHasGun = true;

	SetMaxHealth(sk_tank_health.GetInt());
	SetHealth(sk_tank_health.GetInt());
	BaseClass::Spawn();
	PrecacheBreakModels();

	m_takedamage = DAMAGE_YES;
	delete m_pServerVehicle;
	m_pServerVehicle = new CTankFourWheelServerVehicle();
	m_pServerVehicle->SetVehicle(this);
	m_bIsGoodAimVector = false;

	//IPhysicsObject* physObj = VPhysicsGetObject();
	//physObj->SetMass(GetModelPtr()->mass());

	InitialiseRaytraceAttachments();

	//for(int i = 0; i<4;i++)
	//	m_VehiclePhysics.GetVehicleController()->SetWheelFriction(i, 1.0f);
	/*
	for (int i = 0; i < m_bitsSmoking.GetNumBits(); i++)
	{
		if (m_bitsSmoking.IsBitSet(i))
		{
			int baseAttachment = LookupAttachment(SMOKEPOINT_ATTACHMENT"1");
			EnableSmokeAttachment(baseAttachment + i - 1);
		}
	}*/
}

void CVehicleTank::Think()
{
	BaseClass::Think();


	CBasePlayer* pPlayer = UTIL_GetLocalPlayer();

	if (m_bEngineLocked)
	{
		m_bUnableToFire = true;

		if (pPlayer != NULL)
		{
			pPlayer->m_Local.m_iHideHUD |= HIDEHUD_VEHICLE_CROSSHAIR;
		}
	}
	else
	{
		// Start this as false and update it again each frame
		m_bUnableToFire = false;

		if (pPlayer != NULL)
		{
			pPlayer->m_Local.m_iHideHUD &= ~HIDEHUD_VEHICLE_CROSSHAIR;
		}
	}


	SetSimulationTime(gpGlobals->curtime);

	SetNextThink(gpGlobals->curtime);
	SetAnimatedEveryTick(true);

	// Aim gun based on the player view direction.
	if (m_hPlayer && !m_bExitAnimOn && !m_bEnterAnimOn)
	{
		Vector vecEyeDir, vecEyePos;
		m_hPlayer->EyePositionAndVectors(&vecEyePos, &vecEyeDir, NULL, NULL);

		if (g_pGameRules->GetAutoAimMode() == AUTOAIM_ON_CONSOLE)
		{
			autoaim_params_t params;

			m_hPlayer->GetAutoaimVector(params);

			// Use autoaim as the eye dir if there is an autoaim ent.
			vecEyeDir = params.m_vecAutoAimDir;
		}

		// Trace out from the player's eye point.
		Vector	vecEndPos = vecEyePos + (vecEyeDir * MAX_TRACE_LENGTH);
		trace_t	trace;
		UTIL_TraceLine(vecEyePos, vecEndPos, MASK_SHOT, this, COLLISION_GROUP_NONE, &trace);

		// See if we hit something, if so, adjust end position to hit location.
		if (trace.fraction < 1.0)
		{
			vecEndPos = vecEyePos + (vecEyeDir * MAX_TRACE_LENGTH * trace.fraction);
		}

		//m_vecLookCrosshair = vecEndPos;
		AimGunAt(&vecEndPos);
	}

	StudioFrameAdvance();



	// If the enter or exit animation has finished, tell the server vehicle
	if (IsSequenceFinished() && (m_bExitAnimOn || m_bEnterAnimOn))
	{
		if (m_bEnterAnimOn)
		{
			m_VehiclePhysics.ReleaseHandbrake();
			StartEngine();

			// HACKHACK: This forces the jeep to play a sound when it gets entered underwater
			if (m_VehiclePhysics.IsEngineDisabled())
			{
				CBaseServerVehicle* pServerVehicle = dynamic_cast<CBaseServerVehicle*>(GetServerVehicle());
				if (pServerVehicle)
				{
					pServerVehicle->SoundStartDisabled();
				}
			}
		}

		// Reset on exit anim
		GetServerVehicle()->HandleEntryExitFinish(m_bExitAnimOn, m_bExitAnimOn);
	}

}


void CVehicleTank::DriveVehicle(float flFrameTime, CUserCmd* ucmd, int iButtonsDown, int iButtonsReleased)	
{
	int iButtons = ucmd->buttons;
	// If we're holding down an attack button, update our state
	

	if (iButtons & IN_ATTACK && gpGlobals->curtime > m_flNextAllowedShootTime)
	{
		FireCannon();
	}
	if (iButtons & IN_FORWARD ) 
	{
		UnStickCheckThink();
		CSoundEnt::InsertSound(SOUND_DANGER | SOUND_MOVE_AWAY, GetAbsOrigin(), 1024, 1.0f, this, SOUNDENT_CHANNEL_REPEATED_DANGER);
		if (CentipedeCheck(m_RaytraceAttachmentFL, m_RaytraceAttachmentRL) || CentipedeCheck(m_RaytraceAttachmentFR, m_RaytraceAttachmentRR))
		{
			ApplyPush(false);
		}
	}
	else if (iButtons & IN_BACK && (CentipedeCheck(m_RaytraceAttachmentFL, m_RaytraceAttachmentRL) || CentipedeCheck(m_RaytraceAttachmentFR, m_RaytraceAttachmentRR)))
	{
		ApplyPush(true);
	}

	BaseClass::DriveVehicle(flFrameTime, ucmd, iButtonsDown, iButtonsReleased);
}

void CVehicleTank::SetupMove(CBasePlayer* player, CUserCmd* ucmd, IMoveHelper* pHelper, CMoveData* move)
{

	// If the throttle is disabled or we're upside-down, don't allow throttling (including turbo)
	CUserCmd tmp;
	if (IsOverturned())
	{
		m_bUnableToFire = true;

		tmp = (*ucmd);
		tmp.buttons &= ~(IN_FORWARD | IN_BACK | IN_SPEED);
		ucmd = &tmp;
	}

	BaseClass::SetupMove(player, ucmd, pHelper, move);
}

//-----------------------------------------------------------------------------
// Purpose: Aim Gun at a target
//-----------------------------------------------------------------------------
void CVehicleTank::AimGunAt(Vector* endPos)
{
	Vector	aimPos = *endPos;
	float dotPr = 1;
	CNPC_VehicleDriver* pDriver = GetNPCDriver();



	// See if the gun should be allowed to aim
	if (IsOverturned() || m_bEngineLocked)
	{
		SetPoseParameter(JEEP_GUN_YAW, 0);
		SetPoseParameter(JEEP_GUN_PITCH, 0);
		return;

		// Make the gun go limp and look "down"
		Vector	v_forward, v_up;
		AngleVectors(GetLocalAngles(), NULL, &v_forward, &v_up);
		aimPos = WorldSpaceCenter() + (v_forward * -32.0f) - Vector(0, 0, 128.0f);
	}

	matrix3x4_t gunMatrix;
	GetAttachment(LookupAttachment("gun_ref"), gunMatrix);

	// transform the enemy into gun space
	Vector localEnemyPosition;
	VectorITransform(aimPos, gunMatrix, localEnemyPosition);

	/*//Lychy: compensate for gravity
	if (pDriver)
	{
		//shit calculation
		aimPos.z += Vector2D(aimPos.x, aimPos.y).Length() * 0.02f;//(aimPos.z - GetAbsOrigin().z);
		//localEnemyPosition = VecCheckThrow(this, GetAbsOrigin(), localEnemyPosition, 3500.0f);
	}*/

	// do a look at in gun space (essentially a delta-lookat)
	QAngle localEnemyAngles;
	VectorAngles(localEnemyPosition, localEnemyAngles);

	// convert to +/- 180 degrees
	localEnemyAngles.x = UTIL_AngleDiff(localEnemyAngles.x, 0);
	localEnemyAngles.y = UTIL_AngleDiff(localEnemyAngles.y, 0);

	float targetYaw = m_aimYaw + localEnemyAngles.y;
	float targetPitch = m_aimPitch + localEnemyAngles.x;

	// Constrain our angles
	float newTargetYaw = targetYaw;// clamp(targetYaw, -CANNON_MAX_LEFT_YAW, CANNON_MAX_RIGHT_YAW);
	float newTargetPitch = targetPitch;// clamp(targetPitch, -CANNON_MAX_DOWN_PITCH, CANNON_MAX_UP_PITCH);

	// If the angles have been clamped, we're looking outside of our valid range
	if (fabs(newTargetYaw - targetYaw) > 1e-4 || fabs(newTargetPitch - targetPitch) > 1e-4)
	{
		m_bUnableToFire = true;
	}
	// Now draw crosshair for actual aiming point
	Vector	vecMuzzle, vecMuzzleDir;
	QAngle	vecMuzzleAng;

	GetAttachment("Muzzle", vecMuzzle, vecMuzzleAng);
	AngleVectors(vecMuzzleAng, &vecMuzzleDir);

	
	if (pDriver)
		dotPr = DotProduct(vecMuzzleDir, (m_vecNPCTarget - vecMuzzle).Normalized());


	if (tank_debug.GetBool())
	{
		NDebugOverlay::Line(vecMuzzle, m_vecNPCTarget, 255, 0, 0, 0, 0.05f);
		NDebugOverlay::Line(vecMuzzle, vecMuzzle + (vecMuzzleDir) * 50, 0, 255, 0, 0, 0.05f);
	}
	


	targetYaw = newTargetYaw;
	targetPitch = newTargetPitch;

	// Exponentially approach the target
	float yawSpeed = 1;
	float pitchSpeed = 1;
	float lookSpeed = 0;

	//Lychy: turning speed ramps down
	if (pDriver)
	{
		yawSpeed = -1 * (dotPr * dotPr) + 1.5f;
		pitchSpeed = -1 * (dotPr * dotPr) + 1.5f; // -x^2 + 1.5
	}

	m_aimYaw = UTIL_Approach(targetYaw, m_aimYaw, yawSpeed);
	m_aimPitch = UTIL_Approach(targetPitch, m_aimPitch, pitchSpeed);



	SetPoseParameter(JEEP_GUN_YAW, -m_aimYaw);
	SetPoseParameter(JEEP_GUN_PITCH, -m_aimPitch);
	InvalidateBoneCache();

	// read back to avoid drift when hitting limits
	// as long as the velocity is less than the delta between the limit and 180, this is fine.
	m_aimPitch = -GetPoseParameter(JEEP_GUN_PITCH);
	m_aimYaw = -GetPoseParameter(JEEP_GUN_YAW);


	Vector2D currentVector(m_aimYaw, m_aimPitch);
	if (m_vec2DPrevAimVector.IsValid())		//NaN check
		lookSpeed = (currentVector - m_vec2DPrevAimVector).Length();

	m_vec2DPrevAimVector = currentVector;
	//Msg("lookspeed: %f\n", lookSpeed);

	if (lookSpeed > 0.5)
	{
		if (!m_bIsSounding)
		{
			EmitSound(TANK_GAMESOUND_TURRET_MOVE_START);
			EmitSound(TANK_GAMESOUND_TURRET_MOVE_LOOP);
			m_flPrevAimTime = gpGlobals->curtime;
			m_bIsSounding = true;
		}
	}
	else if (m_bIsSounding && (gpGlobals->curtime - m_flPrevAimTime) >= 0.35f)
	{
		StopLoopingSounds();
		EmitSound(TANK_GAMESOUND_TURRET_MOVE_END);
		m_bIsSounding = false;
	}

	if(pDriver)
		if (dotPr > 0.99) /* && pNPC->HasCondition(COND_SEE_ENEMY)*/
			m_bIsGoodAimVector = true;
		else
			m_bIsGoodAimVector = false;


	//Lychy: make it aim forward if no enemy
	if (pDriver && pDriver->GetLastEnemyTime() + 5.0f < gpGlobals->curtime)
	{
		ResetViewForward();

		if (m_bIsGoodAimVector)
		{
			SetNextThink(TICK_NEVER_THINK, "NPCAimThink");
			StopLoopingSounds();
			EmitSound(TANK_GAMESOUND_TURRET_MOVE_END);
			m_bIsResettingAim = false;
		}
	}
	trace_t	tr;
	UTIL_TraceLine(vecMuzzle, vecMuzzle + (vecMuzzleDir * MAX_TRACE_LENGTH), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);

	// see if we hit something, if so, adjust endPos to hit location
	if (tr.fraction < 1.0)
	{
		m_vecGunCrosshair = vecMuzzle + (vecMuzzleDir * MAX_TRACE_LENGTH * tr.fraction);
	}
}

void CVehicleTank::ShootThink()
{
	//Find the direction the gun is pointing in
	if (GetHealth() > 0) //BETTER BE ALIVE 
	{
		Vector	muzzleOrigin;
		QAngle	muzzleAngles;
		Vector aimVector;
		int muzzleAttachment = LookupAttachment("muzzle");
		GetAttachment(muzzleAttachment, muzzleOrigin, muzzleAngles);
		AngleVectors(muzzleAngles, &aimVector);

		CTankShell* pShell = (CTankShell*)CreateEntityByName("tank_shell");
		pShell->SetOwnerEntity(GetDriver());
		pShell->SetAbsOrigin(muzzleOrigin + aimVector * 10);
		pShell->SetAbsAngles(muzzleAngles);
		pShell->SetAbsVelocity(aimVector * tank_shell_speed.GetFloat());
		pShell->SetGravity(0.3f);
		DispatchSpawn(pShell);

		
		DisableFiring();
		//m_flNextAllowedShootTime = gpGlobals->curtime + 5.0f;
		SetNextThink(gpGlobals->curtime + 3.0f, "ReloadThink");
		SetNextThink(gpGlobals->curtime + 1.5f, "EjectCasingThink");


		EmitSound(TANK_GAMESOUND_FIRE);

#if 1
		DispatchParticleEffect("Explosion_2", PATTACH_POINT_FOLLOW, this, muzzleAttachment );
#else

		ExplosionCreate(muzzleOrigin, muzzleAngles, this, 5, 100, SF_ENVEXPLOSION_NODECAL | SF_ENVEXPLOSION_NOSOUND | SF_ENVEXPLOSION_NOFIREBALLSMOKE | SF_ENVEXPLOSION_NOPARTICLES);
		g_pEffects->MuzzleFlash(muzzleOrigin, muzzleAngles, 25, MUZZLEFLASH_TYPE_DEFAULT);
		UTIL_Smoke(muzzleOrigin, 500, 1);
#endif
		//Rock the car
		IPhysicsObject* pObj = VPhysicsGetObject();

		if (pObj != NULL)
		{
			Vector	shoveDir = aimVector * -(250 * 250.0f);
			pObj->ApplyForceOffset(shoveDir, muzzleOrigin);
		}

		m_bIsFiring = false;
	}
}

void CVehicleTank::FireCannon()
{
	if (!m_bUnableToFire && !m_bHasNotYetReloaded && !m_bIsFiring)
	{
		SetNextThink(gpGlobals->curtime + 1.0f, "ShootThink");
		m_bIsFiring = true;
	}
}


void CVehicleTank::NPCAimThink()
{
	SetNextThink(gpGlobals->curtime + 0.01f, "NPCAimThink");
	AimGunAt(&m_vecNPCTarget);
	
}

int CVehicleTank::DrawDebugTextOverlays()
{
	int text_offset = BaseClass::DrawDebugTextOverlays();
	char buffer[32];
	Q_snprintf(buffer, 32, "Good aim vector?: %i", m_bIsGoodAimVector);
	EntityText(text_offset, buffer, 0);
	text_offset++;
	Q_snprintf(buffer, 327, "Health: %i", GetHealth());
	EntityText(text_offset, buffer, 0);
	return ++text_offset;

}


bool CVehicleTank::ShouldNPCFire()
{
	CNPC_VehicleDriver* pDriver = GetNPCDriver();
	Assert(pDriver);
	return pDriver && m_bIsGoodAimVector && !pDriver->HasCondition(COND_ENEMY_OCCLUDED) && pDriver->GetEnemy() && gpGlobals->curtime > m_flNextAllowedShootTime;
}

CNPC_VehicleDriver* CVehicleTank::GetNPCDriver()
{
	return dynamic_cast<CNPC_VehicleDriver*>(GetDriver());
}
#pragma endregion

void CVehicleTank::ResetViewForward()
{
	if (!m_bIsResettingAim)
		m_bIsGoodAimVector = false;
	m_bIsResettingAim = true;

	GetAttachment(LookupAttachment("muzzle_static"), m_vecNPCTarget);
}

//Enable a random smoke point on the tank
int CVehicleTank::RandomSmokeEnable()
{
	int baseAttachment = LookupAttachment(SMOKEPOINT_ATTACHMENT"1");
	int numAttachments = GetNumSmokePointAttachments();
	int random =  RandomInt(1, numAttachments) ;
	int attachmentChosen = baseAttachment + random - 1;

	if (!m_bitsSmoking.IsBitSet(random))
	{
		m_bitsSmoking.Set(random);
		EnableSmokeAttachment(attachmentChosen);
	}
	SparkShowerAtAttachment(attachmentChosen);
	return random;

}

void CVehicleTank::EnableSmokeAttachment(int iAttachment)
{
	Vector smokePos;

	SmokeTrail* pSmokeTrail;
	pSmokeTrail = SmokeTrail::CreateSmokeTrail();

	pSmokeTrail->m_SpawnRate = 25;
	pSmokeTrail->m_ParticleLifetime = 5;
	pSmokeTrail->m_StartColor.Init(0.4, 0.4, 0.4);
	pSmokeTrail->m_EndColor.Init(0.2, 0.2, 0.2);
	pSmokeTrail->m_StartSize = 5;
	pSmokeTrail->m_EndSize = 50;
	pSmokeTrail->m_SpawnRadius = 20;
	pSmokeTrail->m_MinDirectedSpeed = 25;
	pSmokeTrail->m_MaxDirectedSpeed = 40;
	pSmokeTrail->m_Opacity = 0.4;
	pSmokeTrail->SetLifetime(-1);
	pSmokeTrail->BaseClass::FollowEntity(this);
	pSmokeTrail->m_nAttachment = iAttachment;
}

int CVehicleTank::GetNumSmokePointAttachments()
{

	//Count the attachments
	int maxAttachment = 0;

	int length = strlen(SMOKEPOINT_ATTACHMENT) + 4;
	char* buffer;
	do
	{
		maxAttachment++;
		buffer = (char*)malloc(length);
		Q_snprintf(buffer, length, SMOKEPOINT_ATTACHMENT"%u", maxAttachment);

	} while (LookupAttachment(buffer));
	maxAttachment--;

	if (maxAttachment == 0)
	{
		AssertMsg(0, "Smokepoint attachments gone from Merkava tank model!");
		return 0;
	}
	return maxAttachment;

}

void CVehicleTank::EnableFire()
{
	if (!m_bIsOnFire)
	{
		CFireTrail* pTrail = CFireTrail::CreateFireTrail();
		pTrail->SetParent(this, LookupAttachment("firepoint"));
		pTrail->SetLocalOrigin(vec3_origin);
		DispatchSpawn(pTrail);
	}
}

int CVehicleTank::OnTakeDamage(const CTakeDamageInfo& info)
{
	if (!HasSpawnFlags(SF_INDESTRUCTIBLE) && info.GetAttacker() != GetDriver())
	{
		int prevQuarterDestroyed = (GetMaxHealth() - GetHealth()) / (GetMaxHealth() / 4);
		if (info.GetDamageType() & DMG_BLAST)
		{
			int ret = BaseClass::OnTakeDamage(info);
			int newQuart = (GetMaxHealth() - GetHealth()) / (GetMaxHealth() / 4);
			if (prevQuarterDestroyed != newQuart)
			{
				if (GetHealth() < GetMaxHealth() / 4)
					EnableFire();
				RandomSmokeEnable();
			}
			return ret;
		}
	}
	return 0;
}

void CVehicleTank::Event_Killed(const CTakeDamageInfo& info)
{
	m_takedamage = DAMAGE_NO;
	m_lifeState = LIFE_DEAD;
	if (info.GetAttacker())
	{
		info.GetAttacker()->Event_KilledOther(this, info);
	}
	CBaseEntity* pDriver = GetDriver();
	if (pDriver)
	{
		pDriver->TakeDamage(CTakeDamageInfo(this, this, pDriver->GetHealth(), DMG_BLAST));

		ExitVehicle(VEHICLE_ROLE_DRIVER);
		//GetNPCDriver()->ExitVehicle();
		UTIL_Remove(GetNPCDriver());
	}

	StopEngine();
	SetBodygroup( FindBodygroupByName("weapon"), 1);

	m_bLocked = true;
	m_bEngineLocked = true;

	Vector mawPos;
	int iMaw = LookupAttachment("maw");
	GetAttachment(iMaw, mawPos);

	ExplosionCreate(mawPos, GetAbsAngles(), this, 50, 200, true);

	CBaseEntity* pFire = FireSystem_StartFire(mawPos, 50, 5, 120.0f, SF_FIRE_START_ON | SF_FIRE_DONT_DROP, this);
	pFire->SetParent(this,iMaw);
	pFire->EmitSound("fire_large");

	EmitSound(TANK_GAMESOUND_DIE);

	SpawnMegaGibs();
	EnableSmokeAttachment(iMaw);

	SpawnFlamingGibsAtAttachment(iMaw);
	SparkShowerAtAttachment(iMaw);
	StopLoopingSounds();

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVehicleTank::CreateServerVehicle()
{
	// Create our server vehicle
	m_pServerVehicle = new CTankFourWheelServerVehicle();
	m_pServerVehicle->SetVehicle(this);
}

void CVehicleTank::SpawnFlamingGibsAtAttachment(int iAttachment)
{
	for (int iGib = 0; iGib <= RandomInt(10,15); iGib++)
	{
		CGib* pGib = CREATE_ENTITY(CGib, "gib");
		pGib->m_material = matMetal;
		pGib->m_lifeTime = gpGlobals->curtime + RandomInt(8,20);
		pGib->SetBloodColor(DONT_BLEED);

		const QAngle spitAngle(RandomFloat(-90, -45), RandomFloat(-180, 180), 0);
		Vector spitVector;
		AngleVectors(spitAngle, &spitVector);

		Vector attachmentPosition;
		GetAttachment(iAttachment, attachmentPosition);
		pGib->SetAbsOrigin(attachmentPosition);

		pGib->Spawn(g_PropDataSystem.GetRandomChunkModel("MetalChunks"));
		pGib->SetCollisionGroup(COLLISION_GROUP_DEBRIS);

		IPhysicsObject* pPhysicsObject = pGib->VPhysicsInitNormal(SOLID_VPHYSICS, pGib->GetSolidFlags(), false);
		pGib->SetMoveType(MOVETYPE_VPHYSICS);

		Vector vel = spitVector * RandomInt(250, 500);
		//pGib->SetBaseVelocity(spitVector * 400 );
		pPhysicsObject->ApplyForceCenter(vel);
		//pGib->Ignite(10.0f);

		CEntityFlame* pFlame = CEntityFlame::Create(pGib, false);
		if (pFlame != NULL)
		{
			pFlame->SetLifetime(pGib->m_lifeTime);
			pGib->SetFlame(pFlame);
		}
		//DispatchSpawn(pGib);

	}
}

void CVehicleTank::StopLoopingSounds()
{
	StopSound(TANK_GAMESOUND_TURRET_MOVE_LOOP);
	BaseClass::StopLoopingSounds();
}

void CVehicleTank::SparkShowerAtAttachment(int iAttachment)
{
	CBaseEntity* pEntity = CreateEntityByName("spark_shower");
	Vector attachmentPosition;
	GetAttachment(iAttachment, attachmentPosition);
	pEntity->SetAbsOrigin(attachmentPosition);
	DispatchSpawn(pEntity);
}

void CVehicleTank::ReloadThink()
{
	EmitSound(TANK_GAMESOUND_READYFIRE);
	SetNextThink(gpGlobals->curtime + 1.9f, "FinishReloadThink");
}

void CVehicleTank::FinishReloadThink()
{
	EnableFiring();
}

void CVehicleTank::DisableFiring()
{
	m_bHasNotYetReloaded = true;
}

void CVehicleTank::EnableFiring()
{
	m_bHasNotYetReloaded = false;
}


//	Since a tank can only have 4 wheels due to source stuff, i will create many fake raytrace wheels that will push along the tank if any of them detect solid ground, this will prevent it from getting stuck, especially when going over trenches

//Will trace multiple lines between these two
//Output: Did one of our traces touch the floor
bool CVehicleTank::CentipedeCheck(int iStartAttachment, int iEndAttachment)
{

	Vector startOriginWorld, endOriginWorld;
	Vector startOriginLocal, endOriginLocal;
	
	GetAttachment(iStartAttachment, startOriginWorld);
	GetAttachment(iEndAttachment, endOriginWorld);

	WorldToEntitySpace(startOriginWorld, &startOriginLocal);
	WorldToEntitySpace(endOriginWorld, &endOriginLocal);


	if ( TraceCentipedeLeg(startOriginLocal) && TraceCentipedeLeg(endOriginLocal))
	{
		ApplyPassivePush(false);
		return false; //The 4 wheel vehicle physics is already working, so the tank is not beached, so dont push anymore,
						// The tank may actually not be strong enough to manage a ramp with only 1 pair of wheels, so give it a boost.
	}
#if 1
	IPhysicsObject* physObj = VPhysicsGetObject();
	IPhysicsFrictionSnapshot* snapshot = physObj->CreateFrictionSnapshot();


	while (snapshot->IsValid())
	{
		Vector contactWorld, contactLocal;
		Vector maxPointWorld, maxPointLocal;
		Vector normal;

		GetAttachment(m_RaytraceAttachmentFL, maxPointWorld);
		WorldToEntitySpace(maxPointWorld, &maxPointLocal);

		snapshot->GetContactPoint(contactWorld);
		snapshot->GetSurfaceNormal(normal);

		WorldToEntitySpace(contactWorld, &contactLocal);

		if (contactLocal.z < maxPointLocal.z) //Contact point should be on the treads, anywhere above, it doesnt matter
		{ 
			snapshot->DeleteAllMarkedContacts(true);
			physObj->DestroyFrictionSnapshot(snapshot);

			return true;
		}
		snapshot->NextFrictionData();
	}
	snapshot->DeleteAllMarkedContacts(true);
	physObj->DestroyFrictionSnapshot(snapshot);
	return false;
#else
	Vector direction = endOriginLocal - startOriginLocal;
	vec_t distance = direction.NormalizeInPlace();

	for (vec_t scalar = TANK_CENTIPEDE_LEG_GAP; scalar < distance; scalar += TANK_CENTIPEDE_LEG_GAP) //Dont need to trace what we already have
	{
		Vector startTrace;
		VectorMA(startOriginLocal, scalar, direction, startTrace);
		if (TraceCentipedeLeg(startTrace) == true)
			return true;
	}
	return false;
#endif
}


//Input: starting vector, in local space
//Output: Did we hit the floor?
bool CVehicleTank::TraceCentipedeLeg( const Vector& startTrace)
{

	trace_t tr;
	const Vector endTrace = startTrace + Vector(0, 0, -TANK_CENTIPEDE_LEG_LENGTH);

	Vector startTraceWorld;
	Vector endTraceWorld;

	EntityToWorldSpace(startTrace,&startTraceWorld);
	EntityToWorldSpace(endTrace,&endTraceWorld);

	UTIL_TraceLine(startTraceWorld, endTraceWorld, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr);
	if (tank_debug.GetBool())
		NDebugOverlay::Line(startTraceWorld, endTraceWorld, 255, 0, 0, 0, 0.05f);
	
	return tr.DidHitWorld();
}

void CVehicleTank::InitialiseRaytraceAttachments()
{
	m_RaytraceAttachmentFL = LookupAttachment("raytrace_fl");
	m_RaytraceAttachmentFR = LookupAttachment("raytrace_fr");
	m_RaytraceAttachmentRL = LookupAttachment("raytrace_rl");
	m_RaytraceAttachmentRR = LookupAttachment("raytrace_rr");

	Assert(m_RaytraceAttachmentFL && m_RaytraceAttachmentFR && m_RaytraceAttachmentRR && m_RaytraceAttachmentRL);
}

void CVehicleTank::ApplyPush(bool backward)
{
	Vector vForward;
	GetVectors(&vForward,NULL,NULL);

	if(tank_debug.GetBool())
		Msg("Pushing tank...\n");

	IPhysicsObject* physObject = VPhysicsGetObject();

	float scalar = tank_push_scalar.GetFloat();
	Vector force = vForward * (scalar + (scalar * vForward.z));
	if (backward)
		force *= -1;

	physObject->ApplyForceCenter(force);
}

void CVehicleTank::ExitVehicle(int nRole)
{
	StopLoopingSounds();
	BaseClass::ExitVehicle(nRole);
}

void CVehicleTank::SpawnMegaGibs()
{
	CUtlVector<breakmodel_t> list;
	BreakModelList(list, GetModelIndex(), 0, 0);

	Vector attachmentOrigin;
	QAngle attachmentAngles;
	GetAttachment("turret_gib_spawn", attachmentOrigin, attachmentAngles);

	for (int i = 0; i < list.Count(); i++)
	{
		CPhysicsProp* pGib = CREATE_ENTITY(CPhysicsProp, "prop_physics_OVERRIDE");
		pGib->SetModel(list[i].modelName);

		Vector velocity = (attachmentOrigin - GetAbsOrigin()).Normalized();
		velocity *= 400;

		pGib->SetAbsAngles(attachmentAngles);
		pGib->SetAbsOrigin(attachmentOrigin);
		pGib->m_nSkin = m_nSkin;
		
		DispatchSpawn(pGib);	
		IPhysicsObject* pPhys = pGib->VPhysicsGetObject();
		AngularImpulse angImpulse = RandomAngularImpulse(-5, 5);
		pPhys->AddVelocity(&velocity, &angImpulse);
	}
}	



void CVehicleTank::PrecacheBreakModels()
{
	CUtlVector<breakmodel_t> list;
	BreakModelList(list, GetModelIndex(), 0, 0);

	for (int i = 0; i < list.Count(); i++)
	{
		PrecacheModel(list[i].modelName);
	}
}

void CVehicleTank::ApplyPassivePush(bool backward)
{
	Vector vForward;
	GetVectors(&vForward, NULL, NULL);

	IPhysicsObject* physObject = VPhysicsGetObject();

	float scalar = tank_passive_push_scalar.GetFloat();
	Vector force = vForward * scalar;
	if (backward)
		force *= -1;

	physObject->ApplyForceCenter(force);
}

void CVehicleTank::EjectCasingThink()
{
	EmitSound(TANK_GAMESOUND_EJECTCASING);
}

void CVehicleTank::UnStickCheckThink()
{
	if (GetNPCDriver() && m_flPrevUnStickTime < gpGlobals->curtime - 1)
	{
		if (m_vecPrevUnStickOrigin.IsValid() && (m_vecPrevUnStickOrigin - GetAbsOrigin()).Length() < tank_unstick_check_dist.GetFloat())
		{
			if (tank_debug.GetBool())
				Msg("Unstuck the tank..\n");
			ApplyPush(false);
		}
		m_vecPrevUnStickOrigin = GetAbsOrigin();
	}
	m_flPrevUnStickTime = gpGlobals->curtime;
}
