//========= Totally Not Copyright © 2024, Valve Cut Content, All rights reserved. ============//
//
// Purpose:		Rifle capable of launching frag grenades from it's barrel
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "basehlcombatweapon.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "player.h"
#include "gamerules.h"
#include "in_buttons.h"
#include "soundent.h"
#include "game.h"
#include "vstdlib/random.h"
#include "gamestats.h"

#include "basegrenade_shared.h"
#include "grenade_frag.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	M14_FASTEST_REFIRE_TIME		0.1f
#define	M14_FASTEST_DRY_REFIRE_TIME	0.2f
#define GRENADE_RADIUS	4.0f // inches

//-----------------------------------------------------------------------------
// CWeaponM14
//-----------------------------------------------------------------------------

class CWeaponM14 : public CBaseHLCombatWeapon
{
	DECLARE_DATADESC();

public:
	DECLARE_CLASS( CWeaponM14, CBaseHLCombatWeapon );

	CWeaponM14(void);

	DECLARE_SERVERCLASS();

	void	Precache( void );
	void	ItemPostFrame( void );
	void	PrimaryAttack( void );
	void	SecondaryAttack( void );	
	void	AddViewKick( void );
	void	DryFire( void );
	void	Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	void	WeaponIdle( void );

	int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	Activity	GetPrimaryAttackActivity( void );
	virtual void	FireBullets(const FireBulletsInfo_t& info);
	void AttachGrenade( void );
	bool Deploy(void);
	virtual bool Reload( void );
	void	FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles );

	virtual const Vector& GetBulletSpread( void )
	{		
		// Handle NPCs first
		static Vector npcCone = VECTOR_CONE_3DEGREES;
		if ( GetOwner() && GetOwner()->IsNPC() )
			return npcCone;
			
		static Vector cone;


			cone = VECTOR_CONE_PRECALCULATED;
		

		return cone;
	}
	
	virtual int	GetMinBurst() 
	{ 
		return 1; 
	}

	virtual int	GetMaxBurst() 
	{ 
		return 3; 
	}

	virtual float GetFireRate( void ) 
	{
		return 0.5f; 
	}

	DECLARE_ACTTABLE();

private:
	void	CheckThrowPosition( CBasePlayer *pPlayer, const Vector &vecEye, Vector &vecSrc );

	float	m_flSoonestPrimaryAttack;
	float	m_flLastAttackTime;
	bool	m_bHasGrenadeAttached; //Is a grenade currently attached to the rifle's barrel?
	bool	m_bGrenadeMode; //Are we supposed to be in grenade launching mode?
};


IMPLEMENT_SERVERCLASS_ST(CWeaponM14, DT_WeaponM14)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_m14, CWeaponM14 );
PRECACHE_WEAPON_REGISTER( weapon_m14 );

BEGIN_DATADESC( CWeaponM14 )

	DEFINE_FIELD( m_flSoonestPrimaryAttack, FIELD_TIME ),
	DEFINE_FIELD( m_flLastAttackTime,		FIELD_TIME ),
	DEFINE_FIELD( m_bHasGrenadeAttached,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bGrenadeMode,			FIELD_BOOLEAN ),

END_DATADESC()

acttable_t	CWeaponM14::m_acttable[] = 
{
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_AR2,			false },
	{ ACT_RELOAD,					ACT_RELOAD_SMG1,				false },		
	{ ACT_IDLE,						ACT_IDLE_SMG1,					false },		// FIXME: hook to AR2 unique
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SMG1,			false },		// FIXME: hook to AR2 unique

	{ ACT_WALK,						ACT_WALK_RIFLE,					false },

// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED,				ACT_IDLE_SMG1_RELAXED,			false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_SMG1_STIMULATED,		false },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_SMG1,			false },//always aims

	{ ACT_WALK_RELAXED,				ACT_WALK_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_RIFLE_STIMULATED,		false },
	{ ACT_WALK_AGITATED,			ACT_WALK_AIM_RIFLE,				false },//always aims

	{ ACT_RUN_RELAXED,				ACT_RUN_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_RIFLE_STIMULATED,		false },
	{ ACT_RUN_AGITATED,				ACT_RUN_AIM_RIFLE,				false },//always aims

// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_SMG1_RELAXED,			false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_AIM_RIFLE_STIMULATED,	false },
	{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_SMG1,			false },//always aims

	{ ACT_WALK_AIM_RELAXED,			ACT_WALK_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_RIFLE_STIMULATED,	false },
	{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_RIFLE,				false },//always aims

	{ ACT_RUN_AIM_RELAXED,			ACT_RUN_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_RIFLE_STIMULATED,	false },
	{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_RIFLE,				false },//always aims
//End readiness activities

	{ ACT_WALK_AIM,					ACT_WALK_AIM_RIFLE,				false },
	{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,			false },
	{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,		false },
	{ ACT_RUN,						ACT_RUN_RIFLE,					false },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_RIFLE,				false },
	{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,			false },
	{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,		false },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_AR2,	false },
	{ ACT_COVER_LOW,				ACT_COVER_SMG1_LOW,				false },		// FIXME: hook to AR2 unique
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_AR2_LOW,			false },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_SMG1_LOW,		false },		// FIXME: hook to AR2 unique
	{ ACT_RELOAD_LOW,				ACT_RELOAD_SMG1_LOW,			false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_SMG1,		false },
};


IMPLEMENT_ACTTABLE( CWeaponM14 );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponM14::CWeaponM14( void )
{
	m_flSoonestPrimaryAttack = gpGlobals->curtime;

	m_fMinRange1		= 124;
	m_fMaxRange1		= 1500;
	m_fMinRange2		= 24;
	m_fMaxRange2		= 200;

	m_bFiresUnderwater	= false;

	m_bGrenadeMode		= false;
	m_bHasGrenadeAttached = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponM14::Precache( void )
{
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeaponM14::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
		case EVENT_WEAPON_AR2:
		{
			Vector vecShootOrigin, vecShootDir;
			vecShootOrigin = pOperator->Weapon_ShootPosition();


			FireNPCPrimaryAttack( pOperator, false );
			CAI_BaseNPC *npc = pOperator->MyNPCPointer();
			ASSERT( npc != NULL );
			vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );
			WeaponSound(SINGLE_NPC);
			pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2 );
			pOperator->DoMuzzleFlash();
		}
		break;
		default:
			BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponM14::DryFire( void )
{
	WeaponSound( EMPTY );
	
	m_flSoonestPrimaryAttack	= gpGlobals->curtime + M14_FASTEST_DRY_REFIRE_TIME;
	m_flNextPrimaryAttack		= gpGlobals->curtime + SequenceDuration();
}

//-----------------------------------------------------------------------------
// Purpose: Reset grenade status when deploying
//-----------------------------------------------------------------------------
bool CWeaponM14::Deploy( void )
{
	m_bGrenadeMode = false;
	m_bHasGrenadeAttached = false;
	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponM14::PrimaryAttack( void )
{

	m_flLastAttackTime = gpGlobals->curtime;
	m_flSoonestPrimaryAttack = gpGlobals->curtime + M14_FASTEST_REFIRE_TIME;
	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, GetOwner() );

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if( pOwner )
	{
		pOwner->ViewPunchReset();
	}

	//BaseClass::PrimaryAttack();

	//Sorry guys but FireBullets isn't being called for some reason, so I'm pasting in all the regular primaryattack crap and that somehow fixes it - epicplayer
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
		return;

	pPlayer->DoMuzzleFlash();

	// To make the firing framerate independent, we may have to fire more than one bullet here on low-framerate systems, 
	// especially if the weapon we're firing has a really fast rate of fire.
	int iBulletsToFire = 0;
	float fireRate = GetFireRate();

	// MUST call sound before removing a round from the clip of a CHLMachineGun
	// while ( m_flNextPrimaryAttack <= gpGlobals->curtime )
	{
		if (m_bGrenadeMode) WeaponSound(WPN_DOUBLE, m_flNextPrimaryAttack); else WeaponSound(SINGLE, m_flNextPrimaryAttack);
		m_flNextPrimaryAttack = m_flNextPrimaryAttack + fireRate;
		iBulletsToFire++;
	}

	// Make sure we don't fire more than the amount in the clip, if this weapon uses clips
	if ( UsesClipsForAmmo1() )
	{
		if ( iBulletsToFire > m_iClip1 )
			iBulletsToFire = m_iClip1;
		TakeAwayClip1(iBulletsToFire);
	}

	m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired( pPlayer, true, GetClassname() );

	// Fire the bullets
	FireBulletsInfo_t info;
	info.m_iShots = iBulletsToFire;
	info.m_vecSrc = pPlayer->Weapon_ShootPosition( );
	info.m_vecDirShooting = pPlayer->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );
	info.m_vecSpread = pPlayer->GetAttackSpread( this );
	info.m_flDistance = MAX_TRACE_LENGTH;
	info.m_iAmmoType = m_iPrimaryAmmoType;
	info.m_iTracerFreq = 2;
	FireBullets( info );

	//Factor in the view kick
	AddViewKick();
	
	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0); 
	}

	SendWeaponAnim( GetPrimaryAttackActivity() );
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	// Register a muzzleflash for the AI
	pPlayer->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.2;

	m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired( pOwner, true, GetClassname() );
}

	// check a throw from vecSrc.  If not valid, move the position back along the line to vecEye
void CWeaponM14::CheckThrowPosition( CBasePlayer *pPlayer, const Vector &vecEye, Vector &vecSrc )
{
	trace_t tr;

	UTIL_TraceHull( vecEye, vecSrc, -Vector(GRENADE_RADIUS+2,GRENADE_RADIUS+2,GRENADE_RADIUS+2), Vector(GRENADE_RADIUS+2,GRENADE_RADIUS+2,GRENADE_RADIUS+2), 
		pPlayer->PhysicsSolidMaskForEntity(), pPlayer, pPlayer->GetCollisionGroup(), &tr );
	
	if ( tr.DidHit() )
	{
		vecSrc = tr.endpos;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Don't fire an actual bullet if we're in grenade mode, because the grenade is the projectile
// Input  : &info - 
//-----------------------------------------------------------------------------
void CWeaponM14::FireBullets( const FireBulletsInfo_t& info )
{
	//Fire bullets when not in grenade mode
	if ( CBasePlayer *pPlayer = ToBasePlayer ( GetOwner() ) )
	{
		if (!m_bGrenadeMode) 
		{
			pPlayer->FireBullets(info);
		}

		Vector	vecEye = pPlayer->EyePosition();
		Vector	vForward, vRight;
		QAngle  angEyes;

		pPlayer->EyeVectors(&vForward, &vRight, NULL);
		Vector vecSrc = vecEye + vForward * 18.0f + vRight * 0.0f;
		CheckThrowPosition(pPlayer, vecEye, vecSrc);
		//	vForward[0] += 0.1f;
		vForward[2] += 0.1f;

		Vector vecThrow;
		pPlayer->GetVelocity(&vecThrow, NULL);
		vecThrow += vForward * 2000; //Launch force

		//Grenade launching code
		if ( pPlayer && m_bGrenadeMode && m_bHasGrenadeAttached )
		{
			//Msg("Fire Grenade!\n");
			//vecSrc = pPlayer->Weapon_ShootPosition();
			angEyes = pPlayer->EyeAngles() + QAngle(90, 0, 0);

			Fraggrenade_Create( vecSrc, angEyes, vecThrow, AngularImpulse( 0, 0, 0 ), this, 1.0f, false );


			//WeaponSound( WPN_DOUBLE, m_flNextPrimaryAttack );
			GetOwner()->RemoveAmmo( 1, m_iSecondaryAmmoType );
			m_bHasGrenadeAttached = false;
		}

	}


}

//-----------------------------------------------------------------------------
// Purpose: Attach or detach a grenade to toggle grenade mode
//-----------------------------------------------------------------------------
void CWeaponM14::SecondaryAttack( void )
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (pOwner)
	{	
	
		// change fire modes.

		switch (m_bGrenadeMode)
		{
		case false: //No grenade mode

			if (pOwner->GetAmmoCount(m_iSecondaryAmmoType) > 0) //Make sure we have grenades before attaching one
			{
		
				SendWeaponAnim( ACT_VM_DEPLOY );
				m_bGrenadeMode = true;
				m_bHasGrenadeAttached = true;
				m_flNextSecondaryAttack = gpGlobals->curtime + 2; //2 second delay
				m_flNextPrimaryAttack	= gpGlobals->curtime + 1.5;
			}
			else m_flNextSecondaryAttack = gpGlobals->curtime + 1; //or else 1 second delay

			break;

		case true: //Already in grenade mode
			
			SendWeaponAnim( ACT_VM_UNDEPLOY );
			m_bGrenadeMode = false;
			m_bHasGrenadeAttached = false;
			m_flNextSecondaryAttack = gpGlobals->curtime + 1; //1 second delay
			m_flNextPrimaryAttack	= gpGlobals->curtime + 1;
			break;
		}

		m_flSoonestPrimaryAttack = m_flNextPrimaryAttack;

		//m_flNextPrimaryAttack	= gpGlobals->curtime + SequenceDuration(); //Commented out because it doesn't account for if the player tries switching but has no grenade ammo


		m_iSecondaryAttacks++;
		gamestats->Event_WeaponFired(pOwner, false, GetClassname());
	}
}

//-----------------------------------------------------------------------------
// Purpose: Attach grenade to barrel when in launching mode
//-----------------------------------------------------------------------------
void CWeaponM14::AttachGrenade( void )
{
	CBaseCombatCharacter *pOwner = GetOwner();

	if (pOwner == NULL)
		return;

	if (!m_bGrenadeMode) //How did we get here?
		return;

	if (!m_bHasGrenadeAttached) //Only do stuff if we have no grenade on the barrel
	{
		if (pOwner->GetAmmoCount(m_iSecondaryAmmoType) > 0) //If we have more grenade ammo
		{
			// Attach grenade in launching mode
			SendWeaponAnim( ACT_VM_DEPLOY_1 );
			m_bHasGrenadeAttached = true;
		} 
		else
		{
			SendWeaponAnim( ACT_VM_UNDEPLOY_EMPTY );
			m_bGrenadeMode = false; //Exit grenade mode because we have no ammo left
		}

	pOwner->m_flNextAttack	= gpGlobals->curtime + SequenceDuration();
	m_flNextPrimaryAttack	= gpGlobals->curtime + SequenceDuration();
	m_flSoonestPrimaryAttack = gpGlobals->curtime + SequenceDuration();
	m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();

	}


}

//-----------------------------------------------------------------------------
// Purpose: Allows firing as fast as button is pressed
//-----------------------------------------------------------------------------
void CWeaponM14::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

	if ( (m_flSoonestPrimaryAttack < gpGlobals->curtime) && m_bGrenadeMode && !m_bHasGrenadeAttached ) //Attach a grenade if we need a new one
	{
		AttachGrenade();
	}

	if ( m_bInReload )
		return;
	
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;
	
	//Allow a refire as fast as the player can click
	if ( ( ( pOwner->m_nButtons & IN_ATTACK ) == false ) && ( m_flSoonestPrimaryAttack < gpGlobals->curtime ) )
	{
		m_flNextPrimaryAttack = gpGlobals->curtime - 0.1f;
	}
	else if ( ( pOwner->m_nButtons & IN_ATTACK ) && ( m_flNextPrimaryAttack < gpGlobals->curtime ) && ( m_iClip1 <= 0 ) )
	{
		DryFire();
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
Activity CWeaponM14::GetPrimaryAttackActivity( void )
{
	if ( m_bGrenadeMode )
		return ACT_VM_PRIMARYATTACK_DEPLOYED;

	return ACT_VM_PRIMARYATTACK;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CWeaponM14::Reload( void )
{
	bool fRet = DefaultReload( GetMaxClip1(), GetMaxClip2(), ( m_bHasGrenadeAttached ? ACT_VM_RELOAD_DEPLOYED : ACT_VM_RELOAD ) );
	if ( fRet )
	{
		WeaponSound( RELOAD );
	}
	return fRet;
}

void CWeaponM14::WeaponIdle( void )
{
	if ( !HasWeaponIdleTimeElapsed() )
		return;

	SendWeaponAnim( m_bHasGrenadeAttached ? ACT_VM_IDLE_DEPLOYED : ACT_VM_IDLE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponM14::AddViewKick( void )
{
	CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	QAngle	viewPunch = vec3_angle;

	viewPunch.x = random->RandomFloat( -0.5f, -1.5f );
	//viewPunch.y = random->RandomFloat( -1.6f, 1.6f );
	viewPunch.z = 0.0f;

	//Add it to the view punch
	pPlayer->ViewPunch( viewPunch );
}

//npc attack start
void CWeaponM14::FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles )
{
	Vector vecShootOrigin, vecShootDir;

	CAI_BaseNPC *npc = pOperator->MyNPCPointer();
	ASSERT( npc != NULL );

	if ( bUseWeaponAngles )
	{
		QAngle	angShootDir;
		GetAttachment( LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir );
		AngleVectors( angShootDir, &vecShootDir );
	}
	else 
	{
		vecShootOrigin = pOperator->Weapon_ShootPosition();
		vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );
	}

	CSoundEnt::InsertSound( SOUND_COMBAT|SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy() );

	pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2 );

	// NOTENOTE: This is overriden on the client-side
	 pOperator->DoMuzzleFlash();
	 m_iClip1 = m_iClip1 - 1;

}