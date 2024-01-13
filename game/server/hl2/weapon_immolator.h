//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#ifndef	WEAPONIMMOLATOR_H
#define	WEAPONIMMOLATOR_H

#include "cbase.h"
#include "basehlcombatweapon.h"

enum IMMOLATOR_FIRESTATE
{
	FIRE_OFF,
	FIRE_STARTUP,
	FIRE_CHARGE
};

class CWeaponImmolator : public CBaseHLCombatWeapon
{
public:
	DECLARE_CLASS( CWeaponImmolator, CBaseHLCombatWeapon );

	DECLARE_SERVERCLASS();
	
	void Precache( void );
	void Spawn( void );
	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void ItemPostFrame( void );

	int CapabilitiesGet( void ) {	return bits_CAP_WEAPON_RANGE_ATTACK1;	}

	void ImmolationDamage( const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore );
	virtual bool WeaponLOSCondition( const Vector &ownerPos, const Vector &targetPos, bool bSetConditions );	
	virtual int	WeaponRangeAttack1Condition( float flDot, float flDist );

	void UpdateThink();

	void StartImmolating();
	void StopImmolating();
	bool IsImmolating() { return m_fireState == FIRE_CHARGE; }

	int		GetMinBurst() { return 50; }
	int		GetMaxBurst() { return 100; }

	void Operator_ForceNPCFire( CBaseCombatCharacter  *pOperator, bool bSecondary );
	void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

	bool Deploy( void );
	bool Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	void WeaponIdle( void );
	void UseAmmo( int count );

	float GetFireRate( void ) { return 0.1f; }

	void	FireBeam( void );

	float GetBeamVelocity() const;
	void CalculateMaxDistance();
	void StopLoopingSounds();

	DECLARE_ACTTABLE();
	DECLARE_DATADESC();

private:

	float m_flBurnRadius;
	float m_flTimeLastUpdatedRadius;

	Vector  m_vecImmolatorTarget;

	IMMOLATOR_FIRESTATE		m_fireState;
	float				m_flAmmoUseTime;	// since we use < 1 point of ammo per update, we subtract ammo on a timer.
	float				m_flStartFireTime;	
	bool				m_bUsingAnimEvents;
};

#endif	//WEAPONIMMOLATOR_H