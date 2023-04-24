//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Generic audience member for final boss battle. Implemented because I didn't want to change all the models thru hammer
//
// $NoKeywords: $
//
//=============================================================================//
//=========================================================
// Generic NPC - purely for scripted sequence work.
//=========================================================
#include "cbase.h"
#include "shareddefs.h"
#include "npcevent.h"
#include "ai_basenpc.h"
#include "ai_hull.h"
#include "ai_baseactor.h"
#include "tier1/strtools.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar fuck( "fuck", "5" );

//---------------------------------------------------------
// Sounds
//---------------------------------------------------------


//=========================================================
// NPC's Anim Events Go Here
//=========================================================

class CGenericCrowd : public CAI_BaseActor
{
public:
	DECLARE_CLASS( CGenericCrowd, CAI_BaseActor );

	void	Spawn( void );
	void	Precache( void );
	float	MaxYawSpeed( void );
	Class_T Classify ( void );
	void	HandleAnimEvent( animevent_t *pEvent );
	int		GetSoundInterests ( void );

	
	void	TempGunEffect( void );

	string_t			m_strHullName;

	DECLARE_DATADESC();

protected:
	
	static char *pModels[];
};
LINK_ENTITY_TO_CLASS( generic_crowd, CGenericCrowd );

BEGIN_DATADESC( CGenericCrowd )

	DEFINE_KEYFIELD(m_strHullName,			FIELD_STRING, "hull_name" ),

END_DATADESC()

//=========================================================
// Models array
//=========================================================

char *CGenericCrowd::pModels[] = 
{
	"models/bahl101.mdl",
	"models/combine_soldier.mdl",
	"models/police.mdl",
	"models/childsoldier.mdl",
	"models/cremator_npc.mdl",
	"models/hgang01.mdl",
	"models/monster_metropolice.mdl",
	"models/Tank_Escorter.mdl",
	"models/barngreen.mdl",
	"models/barnyellow.mdl"
};

//=========================================================
// Classify - indicates this NPC's place in the 
// relationship table.
//=========================================================
Class_T	CGenericCrowd::Classify ( void )
{
	return	CLASS_NONE;
}

//=========================================================
// MaxYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
float CGenericCrowd::MaxYawSpeed ( void )
{
	return 90;
}

//=========================================================
// HandleAnimEvent - catches the NPC-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CGenericCrowd::HandleAnimEvent( animevent_t *pEvent )
{
	BaseClass::HandleAnimEvent( pEvent );
}

//=========================================================
// GetSoundInterests - generic NPC can't hear.
//=========================================================
int CGenericCrowd::GetSoundInterests ( void )
{
	return	NULL;
}

//=========================================================
// Spawn
//=========================================================
void CGenericCrowd::Spawn()
{
	Precache();

	char *szModel = (char *)STRING( GetModelName() );
	if (!szModel || !*szModel)
	{
		// himdeez: even if this is unused i'll improve this anyway
		szModel = pModels[ random->RandomInt( 0, ARRAYSIZE( pModels ) - 1 ) ];
		SetModelName( AllocPooledString(szModel) );
	}


	SetModel( szModel );   

	SetModel( STRING( GetModelName() ) );

/*
	if ( FStrEq( STRING( GetModelName() ), "models/player.mdl" ) )
		UTIL_SetSize(this, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
	else
		UTIL_SetSize(this, VEC_HULL_MIN, VEC_HULL_MAX);
*/

	if ( FStrEq( STRING( GetModelName() ), "models/player.mdl" ) || 
		 FStrEq( STRING( GetModelName() ), "models/holo.mdl" ) ||
		 FStrEq( STRING( GetModelName() ), "models/blackout.mdl" ) )
	{
		UTIL_SetSize(this, VEC_HULL_MIN, VEC_HULL_MAX);
	}
	else
	{
		UTIL_SetSize(this, NAI_Hull::Mins(HULL_HUMAN), NAI_Hull::Maxs(HULL_HUMAN));
	}

	if ( !FStrEq( STRING( GetModelName() ), "models/blackout.mdl" ) )
	{
		SetSolid( SOLID_BBOX );
		AddSolidFlags( FSOLID_NOT_STANDABLE );
	}
	else
	{
		SetSolid( SOLID_NONE );
	}

	SetMoveType( MOVETYPE_STEP );
	SetBloodColor( BLOOD_COLOR_RED );
	m_iHealth			= 8;
	m_flFieldOfView		= 0.5;// indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState			= NPC_STATE_NONE;
	
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_OPEN_DOORS );
	
	// remove head turn if no eyes or forward attachment
	if (LookupAttachment( "eyes" ) > 0 && LookupAttachment( "forward" ) > 0) 
	{
		CapabilitiesAdd(  bits_CAP_TURN_HEAD | bits_CAP_ANIMATEDFACE );
	}

	if (m_strHullName != NULL_STRING)
	{
		SetHullType( NAI_Hull::LookupId( STRING( m_strHullName ) ) );
	}
	else
	{
		SetHullType( HULL_HUMAN );
	}
	SetHullSizeNormal( );

	NPCInit();
}

//=========================================================
// Precache - precaches all resources this NPC needs
//=========================================================
void CGenericCrowd::Precache()
{
	PrecacheModel( STRING( GetModelName() ) );
		PrecacheModel( "models/bahl101.mdl" );
		PrecacheModel( "models/combine_soldier.mdl" );
		PrecacheModel( "models/police.mdl" );
		PrecacheModel( "models/childsoldier.mdl" );
		PrecacheModel( "models/cremator_npc.mdl" );
		PrecacheModel( "models/hgang01.mdl" );
		PrecacheModel( "models/monster_metropolice.mdl" );
		PrecacheModel( "models/Tank_Escorter.mdl" );
		PrecacheModel( "models/barngreen.mdl" );
		PrecacheModel( "models/barnyellow.mdl" );
}	

//=========================================================
// AI Schedules Specific to this NPC
//=========================================================






// ----------------------------------------------------------------------