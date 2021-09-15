//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "basehlcombatweapon.h"
#include "NPCevent.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "player.h"
#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define DAMAGE_PER_SECOND 10

#define MAX_SETTINGS	5

#define	AR1_ACCURACY_SHOT_PENALTY_TIME		0.2f	// Applied amount of time each shot adds to the time we must recover from
#define	AR1_ACCURACY_MAXIMUM_PENALTY_TIME	1.5f	// Maximum penalty to deal out


float RateOfFire[ MAX_SETTINGS ] = 
{
	0.1,
	0.2,
	0.5,
	0.7,
	1.0,
};

float Damage[ MAX_SETTINGS ] =
{
	2,
	4,
	10,
	14,
	20,
};


//=========================================================
//=========================================================
class CWeaponAR1 : public CHLMachineGun
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( CWeaponAR1, CHLMachineGun );

	DECLARE_SERVERCLASS();

	CWeaponAR1();

	int m_ROF;

	void	Precache( void );
	bool	Deploy( void );
	void	AddViewKick( void );

	int		GetMinBurst() { return 5; }
	int		GetMaxBurst() { return 10; }

	float	m_flSoonestPrimaryAttack;
	float	m_flLastAttackTime;
	float	m_flAccuracyPenalty;
	int		m_nNumShotsFired;



	float GetFireRate( void ) {return RateOfFire[ m_ROF ];}

	int CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }

	void SecondaryAttack( void );

	virtual void FireBullets( const FireBulletsInfo_t &info );


	virtual const Vector& GetBulletSpread( void )
	{
		static const Vector cone = VECTOR_CONE_10DEGREES;
		return cone;
	}
	/*virtual const Vector& GetBulletSpread( void )
	{		
		// Handle NPCs first
		static Vector npcCone = VECTOR_CONE_10DEGREES;
		if ( GetOwner() && GetOwner()->IsNPC() )
			return npcCone;
			
		static Vector cone;
		
				//static Vector cone;

			float ramp = RemapValClamped(	m_flAccuracyPenalty, 
											0.0f, 
											AR1_ACCURACY_MAXIMUM_PENALTY_TIME, 
											0.0f, 
											1.0f ); 

			// We lerp from very accurate to inaccurate over time
			VectorLerp( VECTOR_CONE_4DEGREES, VECTOR_CONE_10DEGREES, ramp, cone );
		return cone;
	}*/

void CWeaponAR1::UpdatePenaltyTime( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	// Check our penalty time decay
	if ( ( ( pOwner->m_nButtons & IN_ATTACK ) == false ) && ( m_flSoonestPrimaryAttack < gpGlobals->curtime ) )
	{
		m_flAccuracyPenalty -= gpGlobals->frametime;
		m_flAccuracyPenalty = clamp( m_flAccuracyPenalty, 0.0f, AR1_ACCURACY_MAXIMUM_PENALTY_TIME );
	}
}

void CWeaponAR1::ItemPreFrame( void )
{
	UpdatePenaltyTime();

	BaseClass::ItemPreFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponAR1::ItemBusyFrame( void )
{
	UpdatePenaltyTime();

	BaseClass::ItemBusyFrame();
}


void CWeaponAR1::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
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
			CBaseCombatWeapon::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	}
}


void CWeaponAR1::FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles )
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

	WeaponSoundRealtime( SINGLE_NPC );

	CSoundEnt::InsertSound( SOUND_COMBAT|SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy() );

	pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2 );

	// NOTENOTE: This is overriden on the client-side
	 pOperator->DoMuzzleFlash();

	m_iClip1 = m_iClip1 - 1;
}

void CWeaponAR1::Operator_ForceNPCFire( CBaseCombatCharacter *pOperator, bool bSecondary )
{
		// Ensure we have enough rounds in the clip
		m_iClip1++;

		FireNPCPrimaryAttack( pOperator, true );
}


	DECLARE_ACTTABLE();

	private:
	    CNetworkVar( int, m_iBurst );
};



IMPLEMENT_SERVERCLASS_ST(CWeaponAR1, DT_WeaponAR1)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_ar1, CWeaponAR1 );
PRECACHE_WEAPON_REGISTER(weapon_ar1);

acttable_t	CWeaponAR1::m_acttable[] = 
{
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_AR2,			false },
	{ ACT_RELOAD,					ACT_RELOAD_SMG1,				false },		
	{ ACT_IDLE,						ACT_IDLE_SMG1,					false },		
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SMG1,			false },		

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
	{ ACT_COVER_LOW,				ACT_COVER_SMG1_LOW,				false },		
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_AR2_LOW,			false },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_SMG1_LOW,		false },		
	{ ACT_RELOAD_LOW,				ACT_RELOAD_SMG1_LOW,			false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_SMG1,		false },
};

IMPLEMENT_ACTTABLE(CWeaponAR1);

CWeaponAR1::CWeaponAR1( void )
{
	m_flSoonestPrimaryAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0.0f;

	m_ROF = 0;

	m_fMinRange1		= 24;
	m_fMaxRange1		= 1500;
	m_fMinRange2		= 24;
	m_fMaxRange2		= 1500;
}



//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CWeaponAR1 )

	DEFINE_FIELD( m_ROF,			FIELD_INTEGER ),
	DEFINE_FIELD( m_flSoonestPrimaryAttack, FIELD_TIME ),
	DEFINE_FIELD( m_flLastAttackTime,		FIELD_TIME ),
	DEFINE_FIELD( m_flAccuracyPenalty,		FIELD_FLOAT ), //NOTENOTE: This is NOT tracking game time
	DEFINE_FIELD( m_nNumShotsFired,			FIELD_INTEGER ),

END_DATADESC()


/*CWeaponAR1::CWeaponAR1( )
{
	m_ROF = 0;
}*/


void CWeaponAR1::Precache( void )
{
	BaseClass::Precache();
}

bool CWeaponAR1::Deploy( void )
{
	//CBaseCombatCharacter *pOwner  = m_hOwner;
	return BaseClass::Deploy();
}


//=========================================================
//=========================================================
void CWeaponAR1::FireBullets( const FireBulletsInfo_t &info )
{
	if(CBasePlayer *pPlayer = ToBasePlayer( GetOwner() ))
	{
		pPlayer->FireBullets( info );
	}
}

 

void CWeaponAR1::SecondaryAttack( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( pPlayer )
	{
		pPlayer->m_nButtons &= ~IN_ATTACK2;
	}

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.1;

	m_ROF += 1;

	if( m_ROF == MAX_SETTINGS )
	{
		m_ROF = 0;
	}

	int i;

	Msg( "\n" );
	for( i = 0 ; i < MAX_SETTINGS ; i++ )
	{
		if( i == m_ROF )
		{
			Msg( "|" );
		}
		else
		{
			Msg( "-" );
		}
	}
	Msg( "\n" );
}

void CWeaponAR1::AddViewKick( void )
{
	#define	EASY_DAMPEN			3.0f
	#define	MAX_VERTICAL_KICK	6.0f	//Degrees
	#define	SLIDE_LIMIT			3.7f	//Seconds
	
	//Get the view kick
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (!pPlayer)
		return;

	/*QAngle viewPunch;
	viewPunch.x = (random->RandomFloat(-2.5f, 0.5f ) );
	viewPunch.y = (random->RandomFloat( -1.1f, 1.1f ) );
	viewPunch.z = 0;
	if ( pPlayer->GetFlags() & FL_DUCKING )
	{
		viewPunch *= 0.25;
	}
	pPlayer->ViewPunch( viewPunch );*/
	DoMachineGunKick( pPlayer, EASY_DAMPEN, MAX_VERTICAL_KICK, m_fFireDuration, SLIDE_LIMIT );
}

