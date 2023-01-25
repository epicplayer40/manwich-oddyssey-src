//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "player_control.h"
#include "hl2_player.h"
#include "decals.h"
#include "shake.h"
#include "soundent.h"
#include "ndebugoverlay.h"
#include "smoke_trail.h"
#include "grenade_homer.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "hl2_shareddefs.h"

#define PMISSILE_MISSILE_MODEL			"models/weapons/w_missile.mdl"
#define PMISSILE_HULL_MINS				Vector(-22,-22,-22)
#define PMISSILE_HULL_MAXS				Vector(22,22,22)
#define PMISSILE_SPEED					800
#define PMISSILE_TURN_RATE				100
#define PMISSILE_DIE_TIME				2
#define PMISSILE_TRAIL_LIFE				6
#define PMISSILE_LOSE_CONTROL_DIST		30
#define PMISSILE_DANGER_SOUND_DURATION	0.1


extern short	g_sModelIndexFireball;


class CPlayer_Missile : public CPlayer_Control
{
public:
	DECLARE_CLASS( CPlayer_Missile, CPlayer_Control );

	void	Precache(void);
	void	Spawn(void);
	Class_T	Classify( void); 

	void ControlActivate( void );
	void ControlDeactivate( void );

	// Inputs
	void InputActivate( inputdata_t &inputdata );
	void InputDeactivate( inputdata_t &inputdata );

	void	Launch(void);
	void	FlyThink(void);
	void	ExplodeThink(void);
	void	RemoveThink(void);
	void	FlyTouch( CBaseEntity *pOther );
	void	BlowUp(void);
	void	LoseMissileControl(void);

	void	Event_Killed( const CTakeDamageInfo &info );
	int		OnTakeDamage_Alive( const CTakeDamageInfo &info );

	CPlayer_Missile();

	float			m_flLaunchDelay;
	float			m_flDamage;
	float			m_flDamageRadius;
	CHandle<SmokeTrail>	m_hSmokeTrail;
	Vector			m_vSpawnPos;
	QAngle			m_vSpawnAng;
	Vector			m_vBounceVel;
	bool			m_bShake;
	float			m_flStatic;
	float			m_flNextDangerTime;

	DECLARE_DATADESC();
};