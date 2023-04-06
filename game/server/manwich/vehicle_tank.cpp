//Lychy world...

#include "cbase.h"
#include "vehicle_base.h"
#include "in_buttons.h"
#include "explode.h"
#include "npc_vehicledriver.h"
#include "smoke_trail.h"

ConVar tank_shell_speed("tank_shell_speed", "10000");

#define JEEP_GUN_YAW				"vehicle_weapon_yaw"
#define JEEP_GUN_PITCH				"vehicle_weapon_pitch"

#define CANNON_MAX_UP_PITCH			70
#define CANNON_MAX_DOWN_PITCH		20
#define CANNON_MAX_LEFT_YAW			180
#define CANNON_MAX_RIGHT_YAW		180

#pragma region Tank Shell
#define TANKSHELLMODEL "models/shell_casings/missilecasing01.mdl"

class CTankShell : public CBaseAnimating
{
	DECLARE_DATADESC();
	CHandle<SmokeTrail> m_hSmokeTrail;
	void Spawn();
	void Precache();
	void Touch(CBaseEntity* pOther);
	void Think();

};

LINK_ENTITY_TO_CLASS(tank_shell, CTankShell);

BEGIN_DATADESC(CTankShell)
	DEFINE_THINKFUNC(Think),
	DEFINE_FIELD(m_hSmokeTrail, FIELD_EHANDLE),
END_DATADESC()

void CTankShell::Precache()
{
	PrecacheModel(TANKSHELLMODEL);
}

void CTankShell::Spawn()
{
	Precache();
	SetModel(TANKSHELLMODEL);
	SetMoveType(MOVETYPE_FLYGRAVITY);
	SetSolid(SOLID_BBOX);
	SetEFlags(FL_OBJECT);
	m_hSmokeTrail = SmokeTrail::CreateSmokeTrail();
	m_hSmokeTrail->m_SpawnRate = 90;
	m_hSmokeTrail->m_ParticleLifetime = 3;
	m_hSmokeTrail->m_StartColor.Init(0.1, 0.1, 0.1);
	m_hSmokeTrail->m_EndColor.Init(0.5, 0.5, 0.5);
	m_hSmokeTrail->m_StartSize = 10;
	m_hSmokeTrail->m_EndSize = 50;
	m_hSmokeTrail->m_SpawnRadius = 1;
	m_hSmokeTrail->m_MinSpeed = 15;
	m_hSmokeTrail->m_MaxSpeed = 25;
	m_hSmokeTrail->SetLifetime(120);
	m_hSmokeTrail->FollowEntity(this);

	SetThink(&CTankShell::Think);
	SetNextThink(gpGlobals->curtime + 0.01f);
}

void CTankShell::Touch(CBaseEntity* pOther)
{
	if (!pOther->IsSolid())
		return;
	ExplosionCreate(WorldSpaceCenter(), vec3_angle, this, 500, 500, true);
	UTIL_Remove(this);

}
void CTankShell::Think()
{
	QAngle angles;

	VectorAngles(GetAbsVelocity().Normalized(), angles);
	SetAbsAngles(angles + QAngle(0, 90, 0));
}
#pragma endregion

#pragma region Main Vehicle

class CVehicleTank : public CPropVehicleDriveable
{
	DECLARE_CLASS(CVehicleTank, CPropVehicleDriveable);
	DECLARE_DATADESC();
public:
	void Spawn();
	void Precache();
	void Think();
	void FireCannon();
	void DriveVehicle(float flFrameTime, CUserCmd* ucmd, int iButtonsDown, int iButtonsReleased); // Driving Button handling
	void SetupMove(CBasePlayer* player, CUserCmd* ucmd, IMoveHelper* pHelper, CMoveData* move);
	void ShootThink();
	void AimGunAt(Vector* endPos);
	void AimPrimaryWeapon();
	void Restore();
	int DrawDebugTextOverlays();
	void NPCAimThink();
	bool ShouldNPCFire();
	CNPC_VehicleDriver* GetNPCDriver();


	float			m_aimYaw;
	float			m_aimPitch;
	float			m_flNextAllowedShootTime;
	bool			m_bIsFiring;
	bool			m_bIsGoodAimVector;

	Vector			m_vecNPCTarget;

};

BEGIN_DATADESC(CVehicleTank)
	DEFINE_THINKFUNC(ShootThink),
	DEFINE_THINKFUNC(NPCAimThink),
	DEFINE_FIELD(m_aimYaw, FIELD_FLOAT),
	DEFINE_FIELD(m_aimPitch, FIELD_FLOAT),
	DEFINE_FIELD(m_bIsGoodAimVector, FIELD_BOOLEAN),
	DEFINE_FIELD(m_vecNPCTarget, FIELD_VECTOR),

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

	void Weapon_PrimaryRanges(float* flMinRange, float* flMaxRange) override
	{
		*flMinRange = 5.0f;
		*flMaxRange = 9999.0f;
	}
	bool NPC_IsOmniDirectional() override { return true; }
	CVehicleTank* GetTank() { return m_hTank;}

	void CheckAndSetTank()
	{
		if (!GetTank())
			m_hTank = dynamic_cast<CVehicleTank*>(GetVehicleEnt());
	}
	
	void NPC_DriveVehicle() override
	{
		//If the tank is unable to get a shot, then dont try fire
		CheckAndSetTank();
		if(!GetTank()->ShouldNPCFire())
			m_nNPCButtons &= ~IN_ATTACK;

		BaseClass::NPC_DriveVehicle();
	}
	bool NPC_HasPrimaryWeapon() { return true; }

	void NPC_TurnCenter() override
	{
		m_flTurnDegrees = 0.0f;
		BaseClass::NPC_TurnCenter();
	}

	void NPC_AimPrimaryWeapon(Vector vecTarget) override
	{
		CheckAndSetTank();
		if (gpGlobals->curtime > m_flNextAimTime)
		{
			GetTank()->m_vecNPCTarget = vecTarget;
			m_flNextAimTime = gpGlobals->curtime + 0.25f;
		}
		GetTank()->AimPrimaryWeapon();
	}

	//Lychy: The tank should not move if the NPC driver has a good aiming vector
	void NPC_ThrottleReverse(void) override
	{
		CheckAndSetTank();
		if (GetTank()->ShouldNPCFire())
			NPC_Brake();
		else
			BaseClass::NPC_ThrottleReverse();
	}

	void NPC_ThrottleForward(void) override
	{
		CheckAndSetTank();
		if (GetTank()->ShouldNPCFire())
			NPC_Brake();
		else
			BaseClass::NPC_ThrottleForward();
	}

	void NPC_TurnLeft(float flDegrees) override
	{	
		CheckAndSetTank();
		if (GetTank()->ShouldNPCFire())
			NPC_TurnCenter();
		else
			BaseClass::NPC_TurnLeft(flDegrees);
	}

	void NPC_TurnRight(float flDegrees) override
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
		PrecacheModel("models/vehicles/merkava.mdl");
	PrecacheScriptSound("vehicles/tank_readyfire1.wav");
	PrecacheScriptSound("Weapon_Mortar.Single");
	PrecacheScriptSound("vehicles/tank_shellcasing1.wav");
}
void CVehicleTank::Spawn()
{
	Precache();

	if (!m_vehicleScript)
		m_vehicleScript = MAKE_STRING("scripts/vehicles/jeep_test.txt");
	if (!GetModelName())
		SetModel("models/vehicles/merkava.mdl");

	SetPoseParameter(JEEP_GUN_YAW, 0);
	SetPoseParameter(JEEP_GUN_PITCH, 0);
	SetVehicleType(VEHICLE_TYPE_AIRBOAT_RAYCAST);

	RegisterThinkContext("ShootThink");
	SetContextThink(&CVehicleTank::ShootThink, TICK_NEVER_THINK, "ShootThink");
	SetContextThink(&CVehicleTank::NPCAimThink, TICK_NEVER_THINK, "NPCAimThink");
	m_bHasGun = true;

	BaseClass::Spawn();

	delete m_pServerVehicle;
	m_pServerVehicle = new CTankFourWheelServerVehicle();
	m_pServerVehicle->SetVehicle(this);
	m_bIsGoodAimVector = false;
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

	//Lychy: compensate for gravity
	if (pDriver)
	{
		//shit calculation
		aimPos.z += Vector2D(aimPos.x, aimPos.y).Length() * 0.02f;//(aimPos.z - GetAbsOrigin().z);
		//localEnemyPosition = VecCheckThrow(this, GetAbsOrigin(), localEnemyPosition, 3500.0f);
	}

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
	//NDebugOverlay::Line(vecMuzzle, m_vecNPCTarget, 255, 0, 0, 0, 0.05f);
	targetYaw = newTargetYaw;
	targetPitch = newTargetPitch;

	// Exponentially approach the target
	float yawSpeed = 1;
	float pitchSpeed = 1;

	//Lychy: turning speed ramps down
	if (pDriver)
	{
		yawSpeed = -1 * (dotPr * dotPr) + 1.5f;
		pitchSpeed = -1 * (dotPr * dotPr) + 1.5f; // -x^2 + 1.5
	}

	m_aimYaw = UTIL_Approach(targetYaw, m_aimYaw, yawSpeed);
	m_aimPitch = UTIL_Approach(targetPitch, m_aimPitch, pitchSpeed);

	//Msg("AimYaw: %f, AimPitch: %f\n", m_aimYaw, m_aimPitch);
	if (abs(m_aimYaw) > 180)
	{
		m_aimYaw *= -(179 / abs(m_aimYaw));
		//SetPoseParameter(JEEP_GUN_YAW, -targetYaw);
	}

	SetPoseParameter(JEEP_GUN_YAW, -m_aimYaw);
	SetPoseParameter(JEEP_GUN_PITCH, -m_aimPitch);
	InvalidateBoneCache();

	// read back to avoid drift when hitting limits
	// as long as the velocity is less than the delta between the limit and 180, this is fine.
	m_aimPitch = -GetPoseParameter(JEEP_GUN_PITCH);
	m_aimYaw = -GetPoseParameter(JEEP_GUN_YAW);

	if(pDriver)
		if (dotPr > 0.99) /* && pNPC->HasCondition(COND_SEE_ENEMY)*/
			m_bIsGoodAimVector = true;
		else
			m_bIsGoodAimVector = false;
	
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
	Vector	muzzleOrigin;
	QAngle	muzzleAngles;
	Vector aimVector;
	GetAttachment(LookupAttachment("gun_ref"), muzzleOrigin, muzzleAngles);

	AngleVectors(muzzleAngles, &aimVector);
	CTankShell* pShell = (CTankShell*)CreateEntityByName("tank_shell");
	DispatchSpawn(pShell);
	pShell->SetOwnerEntity(this);
	pShell->SetAbsOrigin(muzzleOrigin + aimVector * 10);
	pShell->SetAbsAngles(muzzleAngles + QAngle(0, 90, 0));
	pShell->SetAbsVelocity(aimVector * tank_shell_speed.GetFloat());
	pShell->SetGravity(0.3f);

	m_bIsFiring = false;
	EmitSound("Weapon_Mortar.Single");
	m_flNextAllowedShootTime = gpGlobals->curtime + 5.0f;

	//Rock the car
	IPhysicsObject* pObj = VPhysicsGetObject();

	if (pObj != NULL)
	{
		Vector	shoveDir = aimVector * -(250 * 250.0f);
		pObj->ApplyForceOffset(shoveDir, muzzleOrigin);
	}

}

void CVehicleTank::FireCannon()
{
	if (m_bUnableToFire)
		return;

	if (!m_bIsFiring)
	{
		m_bIsFiring = true;
		EmitSound("vehicles/tank_readyfire1.wav");
		SetNextThink(gpGlobals->curtime + 1.9f, "ShootThink");
	}
}

void CVehicleTank::AimPrimaryWeapon()
{
	SetContextThink(&CVehicleTank::NPCAimThink, gpGlobals->curtime + 0.01f, "NPCAimThink");
}

void CVehicleTank::NPCAimThink()
{
	AimGunAt(&m_vecNPCTarget);
	SetNextThink(gpGlobals->curtime + 0.01f, "NPCAimThink");
}

int CVehicleTank::DrawDebugTextOverlays()
{
	int text_offset = BaseClass::DrawDebugTextOverlays();
	char buffer[32];
	Q_snprintf(buffer, 32, "Good aim vector?: %i", m_bIsGoodAimVector);
	EntityText(text_offset, buffer, 0);
	return ++text_offset;
}

bool CVehicleTank::ShouldNPCFire()
{
	CNPC_VehicleDriver* pDriver = GetNPCDriver();
	Assert(pDriver);
	return m_bIsGoodAimVector && !pDriver->HasCondition(COND_ENEMY_OCCLUDED) && gpGlobals->curtime > m_flNextAllowedShootTime;
}

CNPC_VehicleDriver* CVehicleTank::GetNPCDriver()
{
	return dynamic_cast<CNPC_VehicleDriver*>(GetDriver());
}


#pragma endregion
