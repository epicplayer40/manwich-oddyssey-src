// Lychy!!! :D

#include "cbase.h"
#include "te_effect_dispatch.h"
#include "in_buttons.h"
#include "vehicle_apc.h"
#include "ammodef.h"

ConVar	sk_apc_conscript_damagetype("sk_apc_conscript_damagetype", "Largeround");
ConVar	sk_apc_conscript_burst_time("sk_apc_conscript_burst_time", "0.075f");
ConVar	sk_apc_conscript_burst_pausetime("sk_apc_conscript_pause_time", "2.0f");
ConVar	sk_apc_conscript_burst_size("sk_apc_conscript_burst_size", "10");
// The conscript APC, inherits from normal APC, but with few stylistic changes


class CPropAPCConscript : public CPropAPC
{
	DECLARE_CLASS(CPropAPCConscript, CPropAPC);
	DECLARE_DATADESC();
	
	// Muzzle flashes
	const char* GetTracerType(void);
	void			DoImpactEffect(trace_t& tr, int nDamageType);
	void			DoMuzzleFlash(void);

	const char* GetBulletType() const;
	const char* GetFireMachineGunSound() const;

	void FireMachineGun(int iAttachment);
	void AimPrimaryWeapon(const Vector& vecWorldTarget);

	void	DriveVehicle(float flFrameTime, CUserCmd* ucmd, int iButtonsDown, int iButtonsReleased);

	void	Activate();
	void	Spawn();

	void InputEnableGun(inputdata_t& inputdata);

private:
	int		m_nMachineGunMuzzleRAttachment;
	bool	m_bShootRightBarrel;
};

LINK_ENTITY_TO_CLASS(prop_vehicle_apc_conscript, CPropAPCConscript);

BEGIN_DATADESC(CPropAPCConscript)
	DEFINE_INPUTFUNC(FIELD_BOOLEAN, "EnableGun", InputEnableGun),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char* CPropAPCConscript::GetTracerType(void)
{
	return "Tracer";
}


//-----------------------------------------------------------------------------
// Allows the shooter to change the impact effect of his bullets
//-----------------------------------------------------------------------------
void CPropAPCConscript::DoImpactEffect(trace_t& tr, int nDamageType)
{
	CPropVehicleDriveable::DoImpactEffect(tr, nDamageType);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAPCConscript::DoMuzzleFlash(void)
{
	CEffectData data;
	data.m_nEntIndex = entindex();
	data.m_nAttachmentIndex = m_nMachineGunMuzzleAttachment;
	data.m_flScale = 1.0f;
	if (m_bShootRightBarrel)
	{
		data.m_nAttachmentIndex = m_nMachineGunMuzzleRAttachment;
	}
	DispatchEffect("MuzzleFlash", data);
}

const char* CPropAPCConscript::GetBulletType() const
{
	return sk_apc_conscript_damagetype.GetString();
}

const char* CPropAPCConscript::GetFireMachineGunSound() const
{
	return "apc_conscript.shoot";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAPCConscript::DriveVehicle(float flFrameTime, CUserCmd* ucmd, int iButtonsDown, int iButtonsReleased)
{
	switch (m_lifeState)
	{
	case LIFE_ALIVE:
	{
		int iButtons = ucmd->buttons;
		if (iButtons & IN_ATTACK)
		{
			if (m_bShootRightBarrel)
			{
				FireMachineGun(m_nMachineGunMuzzleRAttachment);
			}
			else
			{
				FireMachineGun(m_nMachineGunMuzzleAttachment);	//Left
			}
			m_bShootRightBarrel = !m_bShootRightBarrel;
		}
	}
	BaseClass::DriveVehicle(flFrameTime, ucmd, iButtonsDown, iButtonsReleased);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAPCConscript::Activate()
{
	BaseClass::Activate();

	m_nMachineGunMuzzleAttachment = LookupAttachment( "muzzle_L" );
	m_nMachineGunMuzzleRAttachment = LookupAttachment( "muzzle_R" );


	// NOTE: gun_ref must have the same position as gun_base, but rotates with the gun
	int nMachineGunRefAttachment = LookupAttachment("gun_def");

	Vector vecWorldBarrelPos;
	matrix3x4_t matRefToWorld;
	GetAttachment(m_nMachineGunMuzzleAttachment, vecWorldBarrelPos);
	GetAttachment(nMachineGunRefAttachment, matRefToWorld);
	VectorITransform(vecWorldBarrelPos, matRefToWorld, m_vecBarrelPos);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAPCConscript::FireMachineGun(int iAttachment)
{
	if (m_flMachineGunTime > gpGlobals->curtime)
		return;

	// If we're still firing the salvo, fire quickly
	m_iMachineGunBurstLeft--;
	if (m_iMachineGunBurstLeft > 0)
	{
		m_flMachineGunTime = gpGlobals->curtime + sk_apc_conscript_burst_time.GetFloat();
	}
	else
	{
		// Reload the salvo
		m_iMachineGunBurstLeft = sk_apc_conscript_burst_size.GetInt();
		m_flMachineGunTime = gpGlobals->curtime + sk_apc_conscript_burst_pausetime.GetFloat();
	}

	Vector vecMachineGunShootPos;
	Vector vecMachineGunDir;
	GetAttachment(iAttachment, vecMachineGunShootPos, &vecMachineGunDir);

	// Fire the round
	int	bulletType = GetAmmoDef()->Index(GetBulletType());
	FireBullets(1, vecMachineGunShootPos, vecMachineGunDir, VECTOR_CONE_8DEGREES, MAX_TRACE_LENGTH, bulletType, 1);
	DoMuzzleFlash();

	EmitSound(GetFireMachineGunSound());
}

//------------------------------------------------
// Spawn
//------------------------------------------------
void CPropAPCConscript::Spawn(void)
{
	m_iMachineGunBurstLeft = sk_apc_conscript_burst_size.GetInt();
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Input handler to enable or disable the APC's mounted gun.
//-----------------------------------------------------------------------------
void CPropAPCConscript::InputEnableGun(inputdata_t& inputdata)
{
	m_bHasGun = inputdata.value.Bool();
	SetBodygroup(FindBodygroupByName("turret"), m_bHasGun);
}

//-----------------------------------------------------------------------------
// Primary gun 
//-----------------------------------------------------------------------------
void CPropAPCConscript::AimPrimaryWeapon(const Vector& vecWorldTarget)
{
	EntityMatrix parentMatrix;
	parentMatrix.InitFromEntity(this, m_nMachineGunBaseAttachment);
	Vector target = parentMatrix.WorldToLocal(vecWorldTarget);

	float quadTarget = target.LengthSqr();
	float quadTargetXY = target.x * target.x + target.y * target.y;

	// Target is too close!  Can't aim at it
	if (quadTarget > m_vecBarrelPos.LengthSqr())
	{
		// We're trying to aim the offset barrel at an arbitrary point.
		// To calculate this, I think of the target as being on a sphere with 
		// it's center at the origin of the gun.
		// The rotation we need is the opposite of the rotation that moves the target 
		// along the surface of that sphere to intersect with the gun's shooting direction
		// To calculate that rotation, we simply calculate the intersection of the ray 
		// coming out of the barrel with the target sphere (that's the new target position)
		// and use atan2() to get angles

		// angles from target pos to center
		float targetToCenterYaw = atan2(target.y, target.x);
		float centerToGunYaw = atan2(m_vecBarrelPos.y, sqrt(quadTarget - (m_vecBarrelPos.y * m_vecBarrelPos.y)));

		float targetToCenterPitch = atan2(target.z, sqrt(quadTargetXY));
		float centerToGunPitch = atan2(-m_vecBarrelPos.z, sqrt(quadTarget - (m_vecBarrelPos.z * m_vecBarrelPos.z)));

		QAngle angles;
		angles.Init(-RAD2DEG(targetToCenterPitch + centerToGunPitch), RAD2DEG(targetToCenterYaw + centerToGunYaw), 0);

		float curPitch = GetPoseParameter("vehicle_weapon_pitch");
		float curYaw = GetPoseParameter("vehicle_weapon_yaw");

		float pitchSpeed = 20;
		float yawSpeed = pitchSpeed;

		QAngle angleInterp(UTIL_Approach(angles.x, curPitch, pitchSpeed), UTIL_Approach(angles.y, curYaw, yawSpeed), 0);

		SetPoseParameter("vehicle_weapon_yaw", angleInterp.y);
		SetPoseParameter("vehicle_weapon_pitch", angleInterp.x);


		curPitch = GetPoseParameter("vehicle_weapon_pitch");
		curYaw = GetPoseParameter("vehicle_weapon_yaw");
		StudioFrameAdvance();


		m_bInFiringCone = (fabs(curPitch - angles.x) < 1e-3) && (fabs(curYaw - angles.y) < 1e-3);
	}
	else
	{
		m_bInFiringCone = false;
	}
}