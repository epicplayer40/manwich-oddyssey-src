//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "ai_basenpc.h"
#include "ai_default.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "ai_motor.h"
#include "ai_memory.h"
#include "ai_route.h"
#include "soundent.h"
#include "game.h"
#include "npcevent.h"
#include "entitylist.h"
#include "ai_task.h"
#include "activitylist.h"
#include "engine/IEngineSound.h"
#include "npc_BaseZombie.h"
#include "movevars_shared.h"
#include "IEffects.h"
#include "props.h"
#include "physics_npc_solver.h"
#include "physics_prop_ragdoll.h"

#include	"hl1/hl1_basegrenade.h"
#include	"hl1/hl1_grenade_mp5.h"

#include "antlion_dust.h"

#ifdef HL2_EPISODIC
#include "episodic/ai_behavior_passenger_zombie.h"
#endif	// HL2_EPISODIC

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define WRETCH_IDLE_PITCH			35
#define WRETCH_MIN_PITCH			70
#define WRETCH_MAX_PITCH			130
#define WRETCH_SOUND_UPDATE_FREQ	0.5

#define		WRETCH_AE_GRENADETHROW	( 5 )
#define		WRETCH_AE_DEPLOYSMOKE	( 10 )

#define WRETCH_MAXLEAP_Z		128

#define WRETCH_EXCITE_DIST 480.0

#define WRETCH_BASE_FREQ 1.5

ConVar	sk_wretch_health( "sk_wretch_health","250");

// If flying at an enemy, and this close or closer, start playing the maul animation!!
#define WRETCH_MAUL_RANGE	300

#ifdef HL2_EPISODIC

int AE_PASSENGER_PHYSICS_PUSH2;
int AE_WRETCH_VEHICLE_LEAP;
int AE_WRETCH_VEHICLE_SS_DIE;	// Killed while doing scripted sequence on vehicle

#endif // HL2_EPISODIC

enum
{
	COND_WRETCH_CLIMB_TOUCH	= LAST_BASE_ZOMBIE_CONDITION,
};

envelopePoint_t envWretchVolumeJump[] =
{
	{	1.0f, 1.0f,
		0.1f, 0.1f,
	},
	{	0.0f, 0.0f,
		1.0f, 1.2f,
	},
};

envelopePoint_t envWretchVolumePain[] =
{
	{	1.0f, 1.0f,
		0.1f, 0.1f,
	},
	{	0.0f, 0.0f,
		1.0f, 1.0f,
	},
};

envelopePoint_t envWretchInverseVolumePain[] =
{
	{	0.0f, 0.0f,
		0.1f, 0.1f,
	},
	{	1.0f, 1.0f,
		1.0f, 1.0f,
	},
};

envelopePoint_t envWretchVolumeJumpPostApex[] =
{
	{	1.0f, 1.0f,
		0.1f, 0.1f,
	},
	{	0.0f, 0.0f,
		1.0f, 1.2f,
	},
};

envelopePoint_t envWretchVolumeClimb[] =
{
	{	1.0f, 1.0f,
		0.1f, 0.1f,
	},
	{	0.0f, 0.0f,
		0.2f, 0.2f,
	},
};

envelopePoint_t envWretchMoanVolumeFast[] =
{
	{	1.0f, 1.0f,
		0.1f, 0.1f,
	},
	{	0.0f, 0.0f,
		0.2f, 0.3f,
	},
};

envelopePoint_t envWretchMoanVolume[] =
{
	{	1.0f, 1.0f,
		0.1f, 0.1f,
	},
	{	1.0f, 1.0f,
		0.2f, 0.2f,
	},
	{	0.0f, 0.0f,
		1.0f, 0.4f,
	},
};

envelopePoint_t envWretchFootstepVolume[] =
{
	{	1.0f, 1.0f,
		0.1f, 0.1f,
	},
	{	0.7f, 0.7f,
		0.2f, 0.2f,
	},
};

envelopePoint_t envWretchVolumeFrenzy[] =
{
	{	1.0f, 1.0f,
		0.1f, 0.1f,
	},
	{	0.0f, 0.0f,
		2.0f, 2.0f,
	},
};


//=========================================================
// animation events
//=========================================================
int AE_WRETCH_LEAP;
int AE_WRETCH_GALLOP_LEFT;
int AE_WRETCH_GALLOP_RIGHT;
int AE_WRETCH_CLIMB_LEFT;
int AE_WRETCH_CLIMB_RIGHT;

//=========================================================
// tasks
//=========================================================
enum 
{
	TASK_WRETCH_DO_ATTACK = LAST_SHARED_TASK + 100,	// again, my !!!HACKHACK
	TASK_WRETCH_LAND_RECOVER,
	TASK_WRETCH_UNSTICK_JUMP,
	TASK_WRETCH_JUMP_BACK,
	TASK_WRETCH_VERIFY_ATTACK,
};

//=========================================================
// activities
//=========================================================
int ACT_WRETCH_LEAP_SOAR;
int ACT_WRETCH_LEAP_STRIKE;
int ACT_WRETCH_LAND_RIGHT;
int ACT_WRETCH_LAND_LEFT;
int ACT_WRETCH_FRENZY;
int ACT_WRETCH_BIG_SLASH;

//=========================================================
// schedules
//=========================================================
enum
{
	SCHED_WRETCH_UNSTICK_JUMP = LAST_SHARED_SCHEDULE, // hack to get past the base zombie's schedules
	SCHED_WRETCH_CLIMBING_UNSTICK_JUMP,
	SCHED_WRETCH_MELEE_ATTACK1,
	SCHED_WRETCH_TORSO_MELEE_ATTACK1,
};



//=========================================================
//=========================================================
class CWretch : public CNPC_BaseZombie
{
	DECLARE_CLASS( CWretch, CNPC_BaseZombie );

public:
	void Spawn( void );
	void Precache( void );

	void SetZombieModel( void );
	bool CanSwatPhysicsObjects( void ) { return false; }

	int	TranslateSchedule( int scheduleType );

	Activity NPC_TranslateActivity( Activity baseAct );

	void LeapAttackTouch( CBaseEntity *pOther );
	void ClimbTouch( CBaseEntity *pOther );

	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );
	int SelectSchedule( void );
	void OnScheduleChange( void );

	void PrescheduleThink( void );

	float InnateRange1MaxRange( void );
	int MeleeAttack1Conditions( float flDot, float flDist );

	virtual float GetClawAttackRange() const { return 50; }

	bool ShouldPlayFootstepMoan( void ) { return false; }

	void HandleAnimEvent( animevent_t *pEvent );

	void PostNPCInit( void );

	void LeapAttack( void );
	void LeapAttackSound( void );

	void BecomeTorso( const Vector &vecTorsoForce, const Vector &vecLegsForce );

	bool IsJumpLegal(const Vector &startPos, const Vector &apex, const Vector &endPos) const;
	bool MovementCost( int moveType, const Vector &vecStart, const Vector &vecEnd, float *pCost );
	bool ShouldFailNav( bool bMovementFailed );

	int	SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );

	const char *GetMoanSound( int nSound );

	void OnChangeActivity( Activity NewActivity );
	void OnStateChange( NPC_STATE OldState, NPC_STATE NewState );
	void Event_Killed( const CTakeDamageInfo &info );
	bool ShouldBecomeTorso( const CTakeDamageInfo &info, float flDamageThreshold );
	HeadcrabRelease_t ShouldReleaseHeadcrab( const CTakeDamageInfo &info, float flDamageThreshold );
	void ReleaseHeadcrab( const Vector &vecOrigin, const Vector &vecVelocity, bool fRemoveHead, bool fRagdollBody, bool fRagdollCrab );

	virtual Vector GetAutoAimCenter() { return WorldSpaceCenter() - Vector( 0, 0, 12.0f ); }

	void PainSound( const CTakeDamageInfo &info );
	void DeathSound( const CTakeDamageInfo &info ); 
	void AlertSound( void );
	void IdleSound( void );
	void AttackSound( void );
	void AttackHitSound( void );
	void AttackMissSound( void );
	void FootstepSound( bool fRightFoot );
	void FootscuffSound( bool fRightFoot ) {}; // fast guy doesn't scuff
	void StopLoopingSounds( void );

	void SoundInit( void );
	void SetIdleSoundState( void );
	void SetAngrySoundState( void );

	void BuildScheduleTestBits( void );

	void BeginNavJump( void );
	void EndNavJump( void );

	float m_flNextGrenadeCheck;
	int		m_iLastGrenadeCondition;
	Vector	m_vecTossVelocity;

	bool IsNavJumping( void ) { return m_fIsNavJumping; }
	void OnNavJumpHitApex( void );

	void BeginAttackJump( void );
	void EndAttackJump( void );

	float		MaxYawSpeed( void );

	int     GetGrenadeConditions ( float flDot, float flDist );
	int     RangeAttack2Conditions ( float flDot, float flDist );

	virtual const char *GetHeadcrabClassname( void );
	virtual const char *GetHeadcrabModel( void );
	virtual const char *GetLegsModel( void );
	virtual const char *GetTorsoModel( void );

//=============================================================================
#ifdef HL2_EPISODIC

public:
	virtual bool	CreateBehaviors( void );
	virtual void	VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	virtual	void	UpdateEfficiency( bool bInPVS );
	virtual bool	IsInAVehicle( void );
	void			InputAttachToVehicle( inputdata_t &inputdata );
	void			VehicleLeapAttackTouch( CBaseEntity *pOther );

private:
	void			VehicleLeapAttack( void );
	bool			CanEnterVehicle( CPropJeepEpisodic *pVehicle );

	CAI_PassengerBehaviorZombie		m_PassengerBehavior;

#endif	// HL2_EPISODIC
//=============================================================================

protected:

	static const char *pMoanSounds[];

	// Sound stuff
	float			m_flDistFactor; 
	unsigned char	m_iClimbCount; // counts rungs climbed (for sound)
	bool			m_fIsNavJumping;
	bool			m_fIsAttackJumping;
	bool			m_fHitApex;
	mutable float	m_flJumpDist;

	bool			m_fHasScreamed;

private:
	float	m_flNextMeleeAttack;
	bool	m_fJustJumped;
	float	m_flJumpStartAltitude;
	float	m_flTimeUpdateSound;

	CSoundPatch	*m_pLayer2; // used for climbing ladders, and when jumping (pre apex)

public:
	DEFINE_CUSTOM_AI;
	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( npc_wretch, CWretch );
LINK_ENTITY_TO_CLASS( npc_wretch_torso, CWretch );


BEGIN_DATADESC( CWretch )

	DEFINE_FIELD( m_flDistFactor, FIELD_FLOAT ),
	DEFINE_FIELD( m_iClimbCount, FIELD_CHARACTER ),
	DEFINE_FIELD( m_fIsNavJumping, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_fIsAttackJumping, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_fHitApex, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flJumpDist, FIELD_FLOAT ),
	DEFINE_FIELD( m_fHasScreamed, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flNextMeleeAttack, FIELD_TIME ),
	DEFINE_FIELD( m_fJustJumped, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flJumpStartAltitude, FIELD_FLOAT ),
	DEFINE_FIELD( m_flTimeUpdateSound, FIELD_TIME ),
	DEFINE_FIELD(m_iLastGrenadeCondition, FIELD_INTEGER),
	DEFINE_FIELD(m_flNextGrenadeCheck, FIELD_TIME),

	// Function Pointers
	DEFINE_ENTITYFUNC( LeapAttackTouch ),
	DEFINE_ENTITYFUNC( ClimbTouch ),
	DEFINE_SOUNDPATCH( m_pLayer2 ),



#ifdef HL2_EPISODIC
	DEFINE_ENTITYFUNC( VehicleLeapAttackTouch ),
	DEFINE_INPUTFUNC( FIELD_STRING, "AttachToVehicle", InputAttachToVehicle ),
#endif	// HL2_EPISODIC

END_DATADESC()


const char *CWretch::pMoanSounds[] =
{
	"NPC_Wretch.Moan1",
};

//-----------------------------------------------------------------------------
// The model we use for our legs when we get blowed up.
//-----------------------------------------------------------------------------
static const char *s_pLegsModel = "models/gibs/fast_zombie_legs.mdl";


//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CWretch::Precache( void )
{
	PrecacheModel("models/addon/wretch.mdl");

	PrecacheScriptSound( "NPC_Wretch.LeapAttack" );
	PrecacheScriptSound( "NPC_Wretch.FootstepRight" );
	PrecacheScriptSound( "NPC_Wretch.FootstepLeft" );
	PrecacheScriptSound( "NPC_Wretch.AttackHit" );
	PrecacheScriptSound( "NPC_Wretch.AttackMiss" );
	PrecacheScriptSound( "NPC_Wretch.LeapAttack" );
	PrecacheScriptSound( "NPC_Wretch.Attack" );
	PrecacheScriptSound( "NPC_Wretch.Idle" );
	PrecacheScriptSound( "NPC_Wretch.AlertFar" );
	PrecacheScriptSound( "NPC_Wretch.AlertNear" );
	PrecacheScriptSound( "Crem.Footstep" );
	PrecacheScriptSound( "NPC_Wretch.Scream" );
	PrecacheScriptSound( "NPC_Wretch.RangeAttack" );
	PrecacheScriptSound( "NPC_Wretch.Frenzy" );
	PrecacheScriptSound( "NPC_Wretch.NoSound" );
	PrecacheScriptSound( "NPC_Wretch.Die" );

	PrecacheScriptSound( "NPC_Wretch.Gurgle" );

	PrecacheScriptSound( "NPC_Wretch.Moan1" );

	UTIL_PrecacheOther("grenade_hand");
		UTIL_PrecacheOther("grenade_molotov");


	BaseClass::Precache();
}

//---------------------------------------------------------
//---------------------------------------------------------
void CWretch::OnScheduleChange( void )
{
	if ( m_flNextMeleeAttack > gpGlobals->curtime + 1 )
	{
		// Allow melee attacks again.
		m_flNextMeleeAttack = gpGlobals->curtime + 0.5;
	}

	BaseClass::OnScheduleChange();
}

//---------------------------------------------------------
//---------------------------------------------------------
int CWretch::SelectSchedule ( void )
{

// ========================================================
#ifdef HL2_EPISODIC

	// Defer all decisions to the behavior if it's running
	if ( m_PassengerBehavior.CanSelectSchedule() )
	{
		DeferSchedulingToBehavior( &m_PassengerBehavior );
		return BaseClass::SelectSchedule();
	}

#endif //HL2_EPISODIC
// ========================================================

	if ( HasCondition( COND_ZOMBIE_RELEASECRAB ) )
	{
		// Death waits for no man. Or zombie. Or something.
//		return SCHED_ZOMBIE_RELEASECRAB;
	}

	if ( HasCondition( COND_WRETCH_CLIMB_TOUCH ) )
	{
		return SCHED_WRETCH_UNSTICK_JUMP;
	}

	switch ( m_NPCState )
	{
	case NPC_STATE_COMBAT:

		if ( HasCondition( COND_HEAVY_DAMAGE ) )
		{
			return SCHED_TAKE_COVER_FROM_ENEMY;
		}
		if ( HasCondition( COND_LOST_ENEMY ) || ( HasCondition( COND_ENEMY_UNREACHABLE ) && MustCloseToAttack() ) )
		{
			// Set state to alert and recurse!
			SetState( NPC_STATE_ALERT );
			return SelectSchedule();
		}
		

	case NPC_STATE_ALERT:
		if ( HasCondition( COND_LOST_ENEMY ) || ( HasCondition( COND_ENEMY_UNREACHABLE ) && MustCloseToAttack() ) )
		{
			ClearCondition( COND_LOST_ENEMY );
			ClearCondition( COND_ENEMY_UNREACHABLE );
			SetEnemy( NULL );

#ifdef DEBUG_ZOMBIES
			DevMsg("Wandering\n");
#endif

			// Just lost track of our enemy. 
			// Wander around a bit so we don't look like a dingus.
			return SCHED_ZOMBIE_WANDER_MEDIUM;
		}
		break;
	}

	return BaseClass::SelectSchedule();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CWretch::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();

	if( GetGroundEntity() && GetGroundEntity()->Classify() == CLASS_HEADCRAB )
	{
		// Kill!
		CTakeDamageInfo info;
		info.SetDamage( GetGroundEntity()->GetHealth() );
		info.SetAttacker( this );
		info.SetInflictor( this );
		info.SetDamageType( DMG_GENERIC );
		GetGroundEntity()->TakeDamage( info );
	}

 	if( m_pMoanSound && gpGlobals->curtime > m_flTimeUpdateSound )
	{
		// Manage the snorting sound, pitch up for closer.
		float flDistNoBBox;

		if( GetEnemy() && m_NPCState == NPC_STATE_COMBAT )
		{
			flDistNoBBox = ( GetEnemy()->WorldSpaceCenter() - WorldSpaceCenter() ).Length();
			flDistNoBBox -= WorldAlignSize().x;
		}
		else
		{
			// Calm down!
			flDistNoBBox = WRETCH_EXCITE_DIST;
			m_flTimeUpdateSound += 1.0;
		}

		if( flDistNoBBox >= WRETCH_EXCITE_DIST && m_flDistFactor != 1.0 )
		{
			// Go back to normal pitch.
			m_flDistFactor = 1.0;

			ENVELOPE_CONTROLLER.SoundChangePitch( m_pMoanSound, WRETCH_IDLE_PITCH, WRETCH_SOUND_UPDATE_FREQ );
		}
		else if( flDistNoBBox < WRETCH_EXCITE_DIST )
		{
			// Zombie is close! Recalculate pitch.
			int iPitch;

			m_flDistFactor = MIN( 1.0, 1 - flDistNoBBox / WRETCH_EXCITE_DIST ); 
			iPitch = WRETCH_MIN_PITCH + ( ( WRETCH_MAX_PITCH - WRETCH_MIN_PITCH ) * m_flDistFactor); 
			ENVELOPE_CONTROLLER.SoundChangePitch( m_pMoanSound, iPitch, WRETCH_SOUND_UPDATE_FREQ );
		}

		m_flTimeUpdateSound = gpGlobals->curtime + WRETCH_SOUND_UPDATE_FREQ;
	}

	// Crudely detect the apex of our jump
	if( IsNavJumping() && !m_fHitApex && GetAbsVelocity().z <= 0.0 )
	{
		OnNavJumpHitApex();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Startup all of the sound patches that the fast zombie uses.
//
//
//-----------------------------------------------------------------------------
void CWretch::SoundInit( void )
{
	if( !m_pMoanSound )
	{
		// !!!HACKHACK - kickstart the moan sound. (sjb)
		MoanSound( envWretchMoanVolume, ARRAYSIZE( envWretchMoanVolume ) );

		// Clear the commands that the base class gave the moaning sound channel.
		ENVELOPE_CONTROLLER.CommandClear( m_pMoanSound );
	}

	CPASAttenuationFilter filter( this );

	if( !m_pLayer2 )
	{
		// Set up layer2
		m_pLayer2 = ENVELOPE_CONTROLLER.SoundCreate( filter, entindex(), CHAN_VOICE, "NPC_Wretch.Gurgle", ATTN_NORM );

		// Start silent.
		ENVELOPE_CONTROLLER.Play( m_pLayer2, 0.0, 100 );
	}

	SetIdleSoundState();
}

//-----------------------------------------------------------------------------
// Purpose: Make the zombie sound calm.
//-----------------------------------------------------------------------------
void CWretch::SetIdleSoundState( void )
{
	// Main looping sound
	if ( m_pMoanSound )
	{
		ENVELOPE_CONTROLLER.SoundChangePitch( m_pMoanSound, WRETCH_IDLE_PITCH, 1.0 );
		ENVELOPE_CONTROLLER.SoundChangeVolume( m_pMoanSound, 0.75, 1.0 );
	}

	// Second Layer
	if ( m_pLayer2 )
	{
		ENVELOPE_CONTROLLER.SoundChangePitch( m_pLayer2, 100, 1.0 );
		ENVELOPE_CONTROLLER.SoundChangeVolume( m_pLayer2, 0.0, 1.0 );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Make the zombie sound pizzled
//-----------------------------------------------------------------------------
void CWretch::SetAngrySoundState( void )
{
	if (( !m_pMoanSound ) || ( !m_pLayer2 ))
	{
		return;
	}

	EmitSound( "NPC_Wretch.LeapAttack" );

	// Main looping sound
	ENVELOPE_CONTROLLER.SoundChangePitch( m_pMoanSound, WRETCH_MIN_PITCH, 0.5 );
	ENVELOPE_CONTROLLER.SoundChangeVolume( m_pMoanSound, 1.0, 0.5 );

	// Second Layer
	ENVELOPE_CONTROLLER.SoundChangePitch( m_pLayer2, 100, 1.0 );
	ENVELOPE_CONTROLLER.SoundChangeVolume( m_pLayer2, 0.0, 1.0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CWretch::Spawn( void )
{
	Precache();

	m_fJustJumped = false;

	m_fIsTorso = m_fIsHeadless = false;

	if( FClassnameIs( this, "npc_wretch" ) )
	{
		m_fIsTorso = false;
	}
	else
	{
		// This was placed as an npc_wretch_torso
		m_fIsTorso = true;
	}

	m_flNextGrenadeCheck = gpGlobals->curtime + 2;

#ifdef HL2_EPISODIC
	SetBloodColor( BLOOD_COLOR_ZOMBIE );
#else
	SetBloodColor( BLOOD_COLOR_YELLOW );
#endif // HL2_EPISODIC

	m_iHealth			= sk_wretch_health.GetFloat();;
	m_flFieldOfView		= 0.2;

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_MOVE_CLIMB | bits_CAP_MOVE_JUMP | bits_CAP_MOVE_GROUND | bits_CAP_USE_WEAPONS | bits_CAP_INNATE_RANGE_ATTACK2 );
	AddSpawnFlags( SF_NPC_LONG_RANGE | SF_NPC_ALWAYSTHINK );

	if ( m_fIsTorso == true )
	{
		CapabilitiesRemove( bits_CAP_MOVE_JUMP );
	}

	if( Weapon_OwnsThisType( "0" ) )
	{
		CapabilitiesRemove( bits_CAP_USE_WEAPONS );
	}

	m_flNextAttack = gpGlobals->curtime;

	m_pLayer2 = NULL;
	m_iClimbCount = 0;

	EndNavJump();

	m_flDistFactor = 1.0;

	BaseClass::Spawn();
}

/////////////////////////////////



int CWretch::RangeAttack2Conditions(float flDot, float flDist)
{
	m_iLastGrenadeCondition = GetGrenadeConditions(flDot, flDist);
	return m_iLastGrenadeCondition;
}

int CWretch::GetGrenadeConditions(float flDot, float flDist)
{

	// assume things haven't changed too much since last time
	if (gpGlobals->curtime < m_flNextGrenadeCheck)
		return m_iLastGrenadeCondition;

	if (m_flGroundSpeed != 0)
		return COND_NONE;

	CBaseEntity *pEnemy = GetEnemy();

	if (!pEnemy)
		return COND_NONE;

	Vector flEnemyLKP = GetEnemyLKP();
	if (!(pEnemy->GetFlags() & FL_ONGROUND) && pEnemy->GetWaterLevel() == 0 && flEnemyLKP.z > (GetAbsOrigin().z + WorldAlignMaxs().z))
	{
		//!!!BUGBUG - we should make this check movetype and make sure it isn't FLY? Players who jump a lot are unlikely to 
		// be grenaded.
		// don't throw grenades at anything that isn't on the ground!
		return COND_NONE;
	}

	Vector vecTarget;


		// find feet
		if (random->RandomInt(0, 1))
		{
			// magically know where they are
			pEnemy->CollisionProp()->NormalizedToWorldSpace(Vector(0.5f, 0.5f, 0.0f), &vecTarget);
		}
		else
		{
			// toss it to where you last saw them
			vecTarget = flEnemyLKP;
		}


	if ((vecTarget - GetAbsOrigin()).Length2D() <= 256)
	{
		// crap, I don't want to blow myself up
		m_flNextGrenadeCheck = gpGlobals->curtime + 1; // one full second.
		return COND_NONE;
	}



		Vector vGunPos;
		QAngle angGunAngles;
		GetAttachment("0", vGunPos, angGunAngles);


		Vector vecToss = VecCheckToss(this, vGunPos, vecTarget, -1, 0.5, false);

		if (vecToss != vec3_origin)
		{
			m_vecTossVelocity = vecToss;

			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->curtime + 0.3; // 1/3 second.

			int iRand = random->RandomInt( 1, 5 );

			if (iRand == 1)
			{
				return COND_CAN_RANGE_ATTACK2;
			}
			else if (iRand > 1)
			{
				return COND_NONE;
			}
		}

		{
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->curtime + 1; // one full second.

			return COND_NONE;
		}
	}



////////////////////////////////

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CWretch::PostNPCInit( void )
{
	SoundInit();

	m_flTimeUpdateSound = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the classname (ie "npc_headcrab") to spawn when our headcrab bails.
//-----------------------------------------------------------------------------
const char *CWretch::GetHeadcrabClassname( void )
{
	return NULL;
}

const char *CWretch::GetHeadcrabModel( void )
{
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CWretch::MaxYawSpeed( void )
{
	switch( GetActivity() )
	{
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		return 120;
		break;

	case ACT_RUN:
		return 160;
		break;

	case ACT_WALK:
	case ACT_IDLE:
		return 25;
		break;
		
	default:
		return 20;
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CWretch::SetZombieModel( void )
{
	Hull_t lastHull = GetHullType();
	{
		SetModel( "models/addon/wretch.mdl" );
		SetHullType(HULL_HUMAN);
	}

	SetHullSizeNormal( true );
	SetDefaultEyeOffset();
	SetActivity( ACT_IDLE );

	// hull changed size, notify vphysics
	// UNDONE: Solve this generally, systematically so other
	// NPCs can change size
	if ( lastHull != GetHullType() )
	{
		if ( VPhysicsGetObject() )
		{
			SetupVPhysicsHull();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns the model to use for our legs ragdoll when we are blown in twain.
//-----------------------------------------------------------------------------
const char *CWretch::GetLegsModel( void )
{
	return NULL;
}

const char *CWretch::GetTorsoModel( void )
{
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: See if I can swat the player
//
//
//-----------------------------------------------------------------------------
int CWretch::MeleeAttack1Conditions( float flDot, float flDist )
{
	if ( !GetEnemy() )
	{
		return COND_NONE;
	}

	if( !(GetFlags() & FL_ONGROUND) )
	{
		// Have to be on the ground!
		return COND_NONE;
	}

	if( gpGlobals->curtime < m_flNextMeleeAttack )
	{
		return COND_NONE;
	}
	
	int baseResult = BaseClass::MeleeAttack1Conditions( flDot, flDist );

	// @TODO (toml 07-21-04): follow up with Steve to find out why fz was explicitly not using these conditions
	if ( baseResult == COND_TOO_FAR_TO_ATTACK || baseResult == COND_NOT_FACING_ATTACK )
	{
		return COND_NONE;
	}

	return baseResult;
}

//-----------------------------------------------------------------------------
// Purpose: Returns a moan sound for this class of zombie.
//-----------------------------------------------------------------------------
const char *CWretch::GetMoanSound( int nSound )
{
	return pMoanSounds[ nSound % ARRAYSIZE( pMoanSounds ) ];
}

//-----------------------------------------------------------------------------
// Purpose: Sound of a footstep
//-----------------------------------------------------------------------------
void CWretch::FootstepSound( bool fRightFoot )
{
	if( fRightFoot )
	{
		EmitSound( "NPC_Wretch.FootstepRight" );
	}
	else
	{
		EmitSound( "NPC_Wretch.FootstepLeft" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Play a random attack hit sound
//-----------------------------------------------------------------------------
void CWretch::AttackHitSound( void )
{
	EmitSound( "NPC_Wretch.AttackHit" );
}

//-----------------------------------------------------------------------------
// Purpose: Play a random attack miss sound
//-----------------------------------------------------------------------------
void CWretch::AttackMissSound( void )
{
	// Play a random attack miss sound
	EmitSound( "NPC_Wretch.AttackMiss" );
}

//-----------------------------------------------------------------------------
// Purpose: Play a random attack sound.
//-----------------------------------------------------------------------------
void CWretch::LeapAttackSound( void )
{
	EmitSound( "NPC_Wretch.LeapAttack" );
}

//-----------------------------------------------------------------------------
// Purpose: Play a random attack sound.
//-----------------------------------------------------------------------------
void CWretch::AttackSound( void )
{
	EmitSound( "NPC_Wretch.Attack" );
}

//-----------------------------------------------------------------------------
// Purpose: Play a random idle sound.
//-----------------------------------------------------------------------------
void CWretch::IdleSound( void )
{
	EmitSound( "NPC_Wretch.Idle" );
	MakeAISpookySound( 360.0f );
}

//-----------------------------------------------------------------------------
// Purpose: Play a random pain sound.
//-----------------------------------------------------------------------------
void CWretch::PainSound( const CTakeDamageInfo &info )
{
	if ( m_pLayer2 )
		ENVELOPE_CONTROLLER.SoundPlayEnvelope( m_pLayer2, SOUNDCTRL_CHANGE_VOLUME, envWretchVolumePain, ARRAYSIZE(envWretchVolumePain) );
	if ( m_pMoanSound )
		ENVELOPE_CONTROLLER.SoundPlayEnvelope( m_pMoanSound, SOUNDCTRL_CHANGE_VOLUME, envWretchInverseVolumePain, ARRAYSIZE(envWretchInverseVolumePain) );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CWretch::DeathSound( const CTakeDamageInfo &info ) 
{
	EmitSound( "NPC_Wretch.Die" );
}

//-----------------------------------------------------------------------------
// Purpose: Play a random alert sound.
//-----------------------------------------------------------------------------
void CWretch::AlertSound( void )
{
	CBaseEntity *pPlayer = AI_GetSinglePlayer();

	if( pPlayer )
	{
		// Measure how far the player is, and play the appropriate type of alert sound. 
		// Doesn't matter if I'm getting mad at a different character, the player is the
		// one that hears the sound.
		float flDist;

		flDist = ( GetAbsOrigin() - pPlayer->GetAbsOrigin() ).Length();

		if( flDist > 512 )
		{
			EmitSound( "NPC_Wretch.AlertFar" );
		}
		else
		{
			EmitSound( "NPC_Wretch.AlertNear" );
		}
	}

}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define WRETCH_MINLEAP			200
#define WRETCH_MAXLEAP			300
float CWretch::InnateRange1MaxRange( void ) 
{ 
	return WRETCH_MAXLEAP; 
}

//-----------------------------------------------------------------------------
// Purpose:
//
//
//-----------------------------------------------------------------------------
void CWretch::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
		case WRETCH_AE_GRENADETHROW:
		{
			CHandGrenade *pGrenade = (CHandGrenade*)Create("grenade_molotov", GetAbsOrigin() + Vector(0, 0, 60), vec3_angle);
			if (pGrenade)
			{
				pGrenade->ShootTimed(this, m_vecTossVelocity, 3.5);
			}

			m_iLastGrenadeCondition = COND_NONE;
			m_flNextGrenadeCheck = gpGlobals->curtime + 2;// wait six seconds before even looking again to see if a grenade can be thrown.

			//Msg("MUTANT THREW GRENADE! IT WORKS!\n");
		}
		break;

		case WRETCH_AE_DEPLOYSMOKE:
			{
						UTIL_CreateAntlionDust( GetAbsOrigin() + Vector(0,0,24), GetLocalAngles() );
			}
			break;
	}


	if ( pEvent->event == AE_WRETCH_CLIMB_LEFT || pEvent->event == AE_WRETCH_CLIMB_RIGHT )
	{
		if( ++m_iClimbCount % 3 == 0 )
		{
			ENVELOPE_CONTROLLER.SoundChangePitch( m_pLayer2, random->RandomFloat( 100, 150 ), 0.0 );
			ENVELOPE_CONTROLLER.SoundPlayEnvelope( m_pLayer2, SOUNDCTRL_CHANGE_VOLUME, envWretchVolumeClimb, ARRAYSIZE(envWretchVolumeClimb) );
		}

		return;
	}

	if ( pEvent->event == AE_WRETCH_LEAP )
	{
		LeapAttack();
		return;
	}
	
	if ( pEvent->event == AE_WRETCH_GALLOP_LEFT )
	{
		EmitSound( "Crem.Footstep" );
		return;
	}

	if ( pEvent->event == AE_WRETCH_GALLOP_RIGHT )
	{
		EmitSound( "Crem.Footstep" );
		return;
	}
	
	if ( pEvent->event == AE_ZOMBIE_ATTACK_RIGHT )
	{
		Vector right;
		AngleVectors( GetLocalAngles(), NULL, &right, NULL );
		right = right * -50;

		QAngle angle( -3, -5, -3  );
		ClawAttack( GetClawAttackRange(), 3, angle, right, ZOMBIE_BLOOD_RIGHT_HAND );
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_ATTACK_LEFT )
	{
		Vector right;
		AngleVectors( GetLocalAngles(), NULL, &right, NULL );
		right = right * 50;
		QAngle angle( -3, 5, -3 );
		ClawAttack( GetClawAttackRange(), 3, angle, right, ZOMBIE_BLOOD_LEFT_HAND );
		return;
	}

//=============================================================================
#ifdef HL2_EPISODIC

	// Do the leap attack
	if ( pEvent->event == AE_WRETCH_VEHICLE_LEAP )
	{
		VehicleLeapAttack();
		return;
	}

	// Die while doing an SS in a vehicle
	if ( pEvent->event == AE_WRETCH_VEHICLE_SS_DIE )
	{
		if ( IsInAVehicle() )
		{
			// Get the vehicle's present speed as a baseline
			Vector vecVelocity = vec3_origin;
			CBaseEntity *pVehicle = m_PassengerBehavior.GetTargetVehicle();
			if ( pVehicle )
			{
				pVehicle->GetVelocity( &vecVelocity, NULL );
			}

			// TODO: We need to make this content driven -- jdw
			Vector vecForward, vecRight, vecUp;
			GetVectors( &vecForward, &vecRight, &vecUp );

			vecVelocity += ( vecForward * -2500.0f ) + ( vecRight * 200.0f ) + ( vecUp * 300 );
			
			// Always kill
			float flDamage = GetMaxHealth() + 10;

			// Take the damage and die
			CTakeDamageInfo info( this, this, vecVelocity * 25.0f, WorldSpaceCenter(), flDamage, (DMG_CRUSH|DMG_VEHICLE) );
			TakeDamage( info );
		}
		return;
	}

#endif // HL2_EPISODIC
//=============================================================================

	BaseClass::HandleAnimEvent( pEvent );
}


//-----------------------------------------------------------------------------
// Purpose: Jump at the enemy!! (stole this from the headcrab)
//
//
//-----------------------------------------------------------------------------
void CWretch::LeapAttack( void )
{
	SetGroundEntity( NULL );

	BeginAttackJump();

	LeapAttackSound();

	//
	// Take him off ground so engine doesn't instantly reset FL_ONGROUND.
	//
	UTIL_SetOrigin( this, GetLocalOrigin() + Vector( 0 , 0 , 1 ));

	Vector vecJumpDir;
	CBaseEntity *pEnemy = GetEnemy();

	if ( pEnemy )
	{
		Vector vecEnemyPos = pEnemy->WorldSpaceCenter();

		float gravity = GetCurrentGravity();
		if ( gravity <= 1 )
		{
			gravity = 1;
		}

		//
		// How fast does the zombie need to travel to reach my enemy's eyes given gravity?
		//
		float height = ( vecEnemyPos.z - GetAbsOrigin().z );

		if ( height < 16 )
		{
			height = 16;
		}
		else if ( height > 120 )
		{
			height = 120;
		}
		float speed = sqrt( 2 * gravity * height );
		float time = speed / gravity;

		//
		// Scale the sideways velocity to get there at the right time
		//
		vecJumpDir = vecEnemyPos - GetAbsOrigin();
		vecJumpDir = vecJumpDir / time;

		//
		// Speed to offset gravity at the desired height.
		//
		vecJumpDir.z = speed;

		//
		// Don't jump too far/fast.
		//
#define CLAMP 1000.0
		float distance = vecJumpDir.Length();
		if ( distance > CLAMP )
		{
			vecJumpDir = vecJumpDir * ( CLAMP / distance );
		}

		// try speeding up a bit.
		SetAbsVelocity( vecJumpDir );
		m_flNextAttack = gpGlobals->curtime + 2;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWretch::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_WRETCH_VERIFY_ATTACK:
		// Simply ensure that the zombie still has a valid melee attack
		if( HasCondition( COND_CAN_MELEE_ATTACK1 ) )
		{
			TaskComplete();
		}
		else
		{
			TaskFail("");
		}
		break;

	case TASK_WRETCH_JUMP_BACK:
		{
			SetActivity( ACT_IDLE );

			SetGroundEntity( NULL );

			BeginAttackJump();

			Vector forward;
			AngleVectors( GetLocalAngles(), &forward );

			//
			// Take him off ground so engine doesn't instantly reset FL_ONGROUND.
			//
			UTIL_SetOrigin( this, GetLocalOrigin() + Vector( 0 , 0 , 1 ));

			ApplyAbsVelocityImpulse( forward * -200 + Vector( 0, 0, 200 ) );
		}
		break;

	case TASK_WRETCH_UNSTICK_JUMP:
		{
			SetGroundEntity( NULL );

			// Call begin attack jump. A little bit later if we fail to pathfind, we check
			// this value to see if we just jumped. If so, we assume we've jumped 
			// to someplace that's not pathing friendly, and so must jump again to get out.
			BeginAttackJump();

			//
			// Take him off ground so engine doesn't instantly reset FL_ONGROUND.
			//
			UTIL_SetOrigin( this, GetLocalOrigin() + Vector( 0 , 0 , 1 ));

			CBaseEntity *pEnemy = GetEnemy();
			Vector vecJumpDir;

			if ( GetActivity() == ACT_CLIMB_UP || GetActivity() == ACT_CLIMB_DOWN )
			{
				// Jump off the pipe backwards!
				Vector forward;

				GetVectors( &forward, NULL, NULL );

				ApplyAbsVelocityImpulse( forward * -200 );
			}
			else if( pEnemy )
			{
				vecJumpDir = pEnemy->GetLocalOrigin() - GetLocalOrigin();
				VectorNormalize( vecJumpDir );
				vecJumpDir.z = 0;

				ApplyAbsVelocityImpulse( vecJumpDir * 300 + Vector( 0, 0, 200 ) );
			}
			else
			{
				DevMsg("UNHANDLED CASE! Stuck Fast Zombie with no enemy!\n");
			}
		}
		break;

	case TASK_WAIT_FOR_MOVEMENT:
		// If we're waiting for movement, that means that pathfinding succeeded, and
		// we're about to be moving. So we aren't stuck. So clear this flag. 
		m_fJustJumped = false;

		BaseClass::StartTask( pTask );
		break;

	case TASK_FACE_ENEMY:
		{
			// We don't use the base class implementation of this, because GetTurnActivity
			// stomps our landing scrabble animations (sjb)
			Vector flEnemyLKP = GetEnemyLKP();
			GetMotor()->SetIdealYawToTarget( flEnemyLKP );
		}
		break;

	case TASK_WRETCH_LAND_RECOVER:
		{
			// Set the ideal yaw
			Vector flEnemyLKP = GetEnemyLKP();
			GetMotor()->SetIdealYawToTarget( flEnemyLKP );

			// figure out which way to turn.
			float flDeltaYaw = GetMotor()->DeltaIdealYaw();

			if( flDeltaYaw < 0 )
			{
				SetIdealActivity( (Activity)ACT_WRETCH_LAND_RIGHT );
			}
			else
			{
				SetIdealActivity( (Activity)ACT_WRETCH_LAND_LEFT );
			}


			TaskComplete();
		}
		break;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWretch::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_WRETCH_JUMP_BACK:
	case TASK_WRETCH_UNSTICK_JUMP:
		if( GetFlags() & FL_ONGROUND )
		{
			TaskComplete();
		}
		break;

	default:
		BaseClass::RunTask( pTask );
		break;
	}
}


//---------------------------------------------------------
//---------------------------------------------------------
int CWretch::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{

	case SCHED_WRETCH_UNSTICK_JUMP:
		if ( GetActivity() == ACT_CLIMB_UP || GetActivity() == ACT_CLIMB_DOWN || GetActivity() == ACT_CLIMB_DISMOUNT )
		{
			return SCHED_WRETCH_CLIMBING_UNSTICK_JUMP;
		}
		else
		{
			return SCHED_WRETCH_UNSTICK_JUMP;
		}
		break;
	case SCHED_MOVE_TO_WEAPON_RANGE:
		{
			float flZDist = fabs( GetEnemy()->GetLocalOrigin().z - GetLocalOrigin().z );
			if ( flZDist > WRETCH_MAXLEAP_Z )
				return SCHED_CHASE_ENEMY;
			else // fall through to default
				return BaseClass::TranslateSchedule( scheduleType );
			break;
		}
	default:
		return BaseClass::TranslateSchedule( scheduleType );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
Activity CWretch::NPC_TranslateActivity( Activity baseAct )
{
	if ( baseAct == ACT_CLIMB_DOWN )
		return ACT_CLIMB_UP;
	
	return BaseClass::NPC_TranslateActivity( baseAct );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CWretch::LeapAttackTouch( CBaseEntity *pOther )
{
	if ( !pOther->IsSolid() )
	{
		// Touching a trigger or something.
		return;
	}

	// Stop the zombie and knock the player back
	Vector vecNewVelocity( 0, 0, GetAbsVelocity().z );
	SetAbsVelocity( vecNewVelocity );

	Vector forward;
	AngleVectors( GetLocalAngles(), &forward );
	forward *= 500;
	QAngle qaPunch( 15, random->RandomInt(-5,5), random->RandomInt(-5,5) );
	
	ClawAttack( GetClawAttackRange(), 5, qaPunch, forward, ZOMBIE_BLOOD_BOTH_HANDS );

	SetTouch( NULL );
}

//-----------------------------------------------------------------------------
// Purpose: Lets us know if we touch the player while we're climbing.
//-----------------------------------------------------------------------------
void CWretch::ClimbTouch( CBaseEntity *pOther )
{
	if ( pOther->IsPlayer() )
	{
		// If I hit the player, shove him aside.
		Vector vecDir = pOther->WorldSpaceCenter() - WorldSpaceCenter();
		vecDir.z = 0.0; // planar
		VectorNormalize( vecDir );

		if( IsXbox() )
		{
			vecDir *= 400.0f;
		}
		else
		{
			vecDir *= 200.0f;
		}

		pOther->VelocityPunch( vecDir );

		if ( GetActivity() != ACT_CLIMB_DISMOUNT || 
			 ( pOther->GetGroundEntity() == NULL &&
			   GetNavigator()->IsGoalActive() &&
			   pOther->GetAbsOrigin().z - GetNavigator()->GetCurWaypointPos().z < -1.0 ) )
		{
			SetCondition( COND_WRETCH_CLIMB_TOUCH );
		}

		SetTouch( NULL );
	}
	else if ( dynamic_cast<CPhysicsProp *>(pOther) )
	{
		NPCPhysics_CreateSolver( this, pOther, true, 5.0 );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Shuts down our looping sounds.
//-----------------------------------------------------------------------------
void CWretch::StopLoopingSounds( void )
{
	if ( m_pMoanSound )
	{
		ENVELOPE_CONTROLLER.SoundDestroy( m_pMoanSound );
		m_pMoanSound = NULL;
	}

	if ( m_pLayer2 )
	{
		ENVELOPE_CONTROLLER.SoundDestroy( m_pLayer2 );
		m_pLayer2 = NULL;
	}

	BaseClass::StopLoopingSounds();
}


//-----------------------------------------------------------------------------
// Purpose: Fast zombie cannot range attack when he's a torso!
//-----------------------------------------------------------------------------
void CWretch::BecomeTorso( const Vector &vecTorsoForce, const Vector &vecLegsForce )
{
	return;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if a reasonable jumping distance
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CWretch::IsJumpLegal(const Vector &startPos, const Vector &apex, const Vector &endPos) const
{
	const float MAX_JUMP_RISE		= 440.0f; //wretch has enhanced jump capabilities
	const float MAX_JUMP_DISTANCE	= 1024.0f;
	const float MAX_JUMP_DROP		= 768.0f;

	if ( BaseClass::IsJumpLegal( startPos, apex, endPos, MAX_JUMP_RISE, MAX_JUMP_DROP, MAX_JUMP_DISTANCE ) )
	{
		// Hang onto the jump distance. The AI is going to want it.
		m_flJumpDist = (startPos - endPos).Length();

		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool CWretch::MovementCost( int moveType, const Vector &vecStart, const Vector &vecEnd, float *pCost )
{
	float delta = vecEnd.z - vecStart.z;

	float multiplier = 1;
	if ( moveType == bits_CAP_MOVE_JUMP )
	{
		multiplier = ( delta < 0 ) ? 0.5 : 1.5;
	}
	else if ( moveType == bits_CAP_MOVE_CLIMB )
	{
		multiplier = ( delta > 0 ) ? 0.5 : 4.0;
	}

	*pCost *= multiplier;

	return ( multiplier != 1 );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool CWretch::ShouldFailNav( bool bMovementFailed )
{
	if ( !BaseClass::ShouldFailNav( bMovementFailed ) )
	{
		DevMsg( 2, "Fast zombie in scripted sequence probably hit bad node configuration at %s\n", VecToString( GetAbsOrigin() ) );
		
		if ( GetNavigator()->GetPath()->CurWaypointNavType() == NAV_JUMP && GetNavigator()->RefindPathToGoal( false ) )
		{
			return false;
		}
		DevMsg( 2, "Fast zombie failed to get to scripted sequence\n" );
	}

	return true;
}


//---------------------------------------------------------
// Purpose: Notifier that lets us know when the fast
//			zombie has hit the apex of a navigational jump.
//---------------------------------------------------------
void CWretch::OnNavJumpHitApex( void )
{
	m_fHitApex = true;	// stop subsequent notifications
}

//---------------------------------------------------------
// Purpose: Overridden to detect when the zombie goes into
//			and out of his climb state and his navigation
//			jump state.
//---------------------------------------------------------
void CWretch::OnChangeActivity( Activity NewActivity )
{
	if ( NewActivity == ACT_WRETCH_FRENZY )
	{
		// Scream!!!!
		EmitSound( "NPC_Wretch.Frenzy" );
		SetPlaybackRate( random->RandomFloat( .9, 1.1 ) );	
	}

	if( NewActivity == ACT_JUMP )
	{
		BeginNavJump();
	}
	else if( GetActivity() == ACT_JUMP )
	{
		EndNavJump();
	}

	if ( NewActivity == ACT_LAND )
	{
		m_flNextAttack = gpGlobals->curtime + 1.0;
	}

	if ( NewActivity == ACT_GLIDE )
	{
		// Started a jump.
		BeginNavJump();
	}
	else if ( GetActivity() == ACT_GLIDE )
	{
		// Landed a jump
		EndNavJump();

		if ( m_pMoanSound )
			ENVELOPE_CONTROLLER.SoundChangePitch( m_pMoanSound, WRETCH_MIN_PITCH, 0.3 );
	}

	if ( NewActivity == ACT_CLIMB_UP )
	{
		// Started a climb!
		if ( m_pMoanSound )
			ENVELOPE_CONTROLLER.SoundChangeVolume( m_pMoanSound, 0.0, 0.2 );

		SetTouch( &CWretch::ClimbTouch );
	}
	else if ( GetActivity() == ACT_CLIMB_DISMOUNT || ( GetActivity() == ACT_CLIMB_UP && NewActivity != ACT_CLIMB_DISMOUNT ) )
	{
		// Ended a climb
		if ( m_pMoanSound )
			ENVELOPE_CONTROLLER.SoundChangeVolume( m_pMoanSound, 1.0, 0.2 );

		SetTouch( NULL );
	}

	BaseClass::OnChangeActivity( NewActivity );
}


//=========================================================
// 
//=========================================================
int CWretch::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	if ( m_fJustJumped )
	{
		// Assume we failed cause we jumped to a bad place.
		m_fJustJumped = false;
		return SCHED_WRETCH_UNSTICK_JUMP;
	}

	return BaseClass::SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
}

//=========================================================
// Purpose: Do some record keeping for jumps made for 
//			navigational purposes (i.e., not attack jumps)
//=========================================================
void CWretch::BeginNavJump( void )
{
	m_fIsNavJumping = true;
	m_fHitApex = false;

	ENVELOPE_CONTROLLER.SoundPlayEnvelope( m_pLayer2, SOUNDCTRL_CHANGE_VOLUME, envWretchVolumeJump, ARRAYSIZE(envWretchVolumeJump) );
}

//=========================================================
// 
//=========================================================
void CWretch::EndNavJump( void )
{
	m_fIsNavJumping = false;
	m_fHitApex = false;
}

//=========================================================
// 
//=========================================================
void CWretch::BeginAttackJump( void )
{
	// Set this to true. A little bit later if we fail to pathfind, we check
	// this value to see if we just jumped. If so, we assume we've jumped 
	// to someplace that's not pathing friendly, and so must jump again to get out.
	m_fJustJumped = true;

	m_flJumpStartAltitude = GetLocalOrigin().z;
}

//=========================================================
// 
//=========================================================
void CWretch::EndAttackJump( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWretch::BuildScheduleTestBits( void )
{
	// FIXME: This is probably the desired call to make, but it opts into an untested base class path, we'll need to
	//		  revisit this and figure out if we want that. -- jdw
	// BaseClass::BuildScheduleTestBits();
	//
	// For now, make sure our active behavior gets a chance to add its own bits
	if ( GetRunningBehavior() )
		GetRunningBehavior()->BridgeBuildScheduleTestBits(); 

#ifdef HL2_EPISODIC
	SetCustomInterruptCondition( COND_PROVOKED );
#endif	// HL2_EPISODIC

	// Any schedule that makes us climb should break if we touch player
	if ( GetActivity() == ACT_CLIMB_UP || GetActivity() == ACT_CLIMB_DOWN || GetActivity() == ACT_CLIMB_DISMOUNT)
	{
		SetCustomInterruptCondition( COND_WRETCH_CLIMB_TOUCH );
	}
	else
	{
		ClearCustomInterruptCondition( COND_WRETCH_CLIMB_TOUCH );
	}
}

//=========================================================
// 
//=========================================================
void CWretch::OnStateChange( NPC_STATE OldState, NPC_STATE NewState )
{
	if( NewState == NPC_STATE_COMBAT )
	{
		SetAngrySoundState();
	}
	else if( (m_pMoanSound) && ( NewState == NPC_STATE_IDLE || NewState == NPC_STATE_ALERT ) ) ///!!!HACKHACK - sjb
	{
		// Don't make this sound while we're slumped
		if ( IsSlumped() == false )
		{
			// Set it up so that if the zombie goes into combat state sometime down the road
			// that he'll be able to scream.
			m_fHasScreamed = false;

			SetIdleSoundState();
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CWretch::Event_Killed( const CTakeDamageInfo &info )
{
	// Shut up my screaming sounds.
	CPASAttenuationFilter filter( this );
	EmitSound( filter, entindex(), "NPC_Wretch.NoSound" );

	CTakeDamageInfo dInfo = info;

#if 0

	// Become a server-side ragdoll and create a constraint at the hand
	if ( m_PassengerBehavior.GetPassengerState() == PASSENGER_STATE_INSIDE )
	{
		IPhysicsObject *pVehiclePhys = m_PassengerBehavior.GetTargetVehicle()->GetServerVehicle()->GetVehicleEnt()->VPhysicsGetObject();
		CBaseAnimating *pVehicleAnimating = m_PassengerBehavior.GetTargetVehicle()->GetServerVehicle()->GetVehicleEnt()->GetBaseAnimating();
		int nRightHandBone = 31;//GetBaseAnimating()->LookupBone( "ValveBiped.Bip01_R_Finger2" );
		Vector vecRightHandPos;
		QAngle vecRightHandAngle;
		GetAttachment( LookupAttachment( "Blood_Right" ), vecRightHandPos, vecRightHandAngle );
		//CTakeDamageInfo dInfo( GetEnemy(), GetEnemy(), RandomVector( -200, 200 ), WorldSpaceCenter(), 50.0f, DMG_CRUSH );
		dInfo.SetDamageType( info.GetDamageType() | DMG_REMOVENORAGDOLL );
		dInfo.ScaleDamageForce( 10.0f );
		CBaseEntity *pRagdoll = CreateServerRagdoll( GetBaseAnimating(), 0, info, COLLISION_GROUP_DEBRIS );

		/*
		GetBaseAnimating()->GetBonePosition( nRightHandBone, vecRightHandPos, vecRightHandAngle );

		CBaseEntity *pRagdoll = CreateServerRagdollAttached(	GetBaseAnimating(), 
																vec3_origin, 
																-1, 
																COLLISION_GROUP_DEBRIS, 
																pVehiclePhys,
																pVehicleAnimating, 
																0, 
																vecRightHandPos,
																nRightHandBone,	
																vec3_origin );*/

	}
#endif

	BaseClass::Event_Killed( dInfo );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CWretch::ShouldBecomeTorso( const CTakeDamageInfo &info, float flDamageThreshold )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: A zombie has taken damage. Determine whether he release his headcrab.
// Output : YES, IMMEDIATE, or SCHEDULED (see HeadcrabRelease_t)
//-----------------------------------------------------------------------------
HeadcrabRelease_t CWretch::ShouldReleaseHeadcrab(const CTakeDamageInfo &info, float flDamageThreshold)
{
	return RELEASE_NO;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecOrigin - 
//			&vecVelocity - 
//			fRemoveHead - 
//			fRagdollBody - 
//-----------------------------------------------------------------------------
void CWretch::ReleaseHeadcrab( const Vector &vecOrigin, const Vector &vecVelocity, bool fRemoveHead, bool fRagdollBody, bool fRagdollCrab )
{
	return;
}

//=============================================================================
#ifdef HL2_EPISODIC

//-----------------------------------------------------------------------------
// Purpose: Add the passenger behavior to our repertoire
//-----------------------------------------------------------------------------
bool CWretch::CreateBehaviors( void )
{
	AddBehavior( &m_PassengerBehavior );

	return BaseClass::CreateBehaviors();
}

//-----------------------------------------------------------------------------
// Purpose: Get on the vehicle!
//-----------------------------------------------------------------------------
void CWretch::InputAttachToVehicle( inputdata_t &inputdata )
{
	// Interrupt us
	SetCondition( COND_PROVOKED );

	// Find the target vehicle
	CBaseEntity *pEntity = FindNamedEntity( inputdata.value.String() );
	CPropJeepEpisodic *pVehicle = dynamic_cast<CPropJeepEpisodic *>(pEntity);

	// Get in the car if it's valid
	if ( pVehicle && CanEnterVehicle( pVehicle ) )
	{
		// Set her into a "passenger" behavior
		m_PassengerBehavior.Enable( pVehicle );
		m_PassengerBehavior.AttachToVehicle();
	}

	RemoveSpawnFlags( SF_NPC_GAG );
}

//-----------------------------------------------------------------------------
// Purpose: Passed along from the vehicle's callback list
//-----------------------------------------------------------------------------
void CWretch::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	// Only do the override while riding on a vehicle
	if ( m_PassengerBehavior.CanSelectSchedule() && m_PassengerBehavior.GetPassengerState() != PASSENGER_STATE_OUTSIDE )
	{
		int damageType = 0;
		float flDamage = CalculatePhysicsImpactDamage( index, pEvent, gZombiePassengerImpactDamageTable, 1.0, true, damageType );

		if ( flDamage > 0  )
		{
			Vector damagePos;
			pEvent->pInternalData->GetContactPoint( damagePos );
			Vector damageForce = pEvent->postVelocity[index] * pEvent->pObjects[index]->GetMass();
			CTakeDamageInfo info( this, this, damageForce, damagePos, flDamage, (damageType|DMG_VEHICLE) );
			TakeDamage( info );
		}
		return;
	}

	BaseClass::VPhysicsCollision( index, pEvent );
}

//-----------------------------------------------------------------------------
// Purpose: FIXME: Fold this into LeapAttack using different jump targets!
//-----------------------------------------------------------------------------
void CWretch::VehicleLeapAttack( void )
{
	CBaseEntity *pEnemy = GetEnemy();
	if ( pEnemy == NULL )
		return;

	Vector vecEnemyPos;
	UTIL_PredictedPosition( pEnemy, 1.0f, &vecEnemyPos );

	// Move
	SetGroundEntity( NULL );
	BeginAttackJump();
	LeapAttackSound();

	// Take him off ground so engine doesn't instantly reset FL_ONGROUND.
	UTIL_SetOrigin( this, GetLocalOrigin() + Vector( 0 , 0 , 1 ));

	// FIXME: This should be the exact position we'll enter at, but this approximates it generally
	//vecEnemyPos[2] += 16;

	Vector vecMins = GetHullMins();
	Vector vecMaxs = GetHullMaxs();
	Vector vecJumpDir = VecCheckToss( this, GetAbsOrigin(), vecEnemyPos, 0.1f, 1.0f, false, &vecMins, &vecMaxs );

	SetAbsVelocity( vecJumpDir );
	m_flNextAttack = gpGlobals->curtime + 2.0f;
	SetTouch( &CWretch::VehicleLeapAttackTouch );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWretch::CanEnterVehicle( CPropJeepEpisodic *pVehicle )
{
	if ( pVehicle == NULL )
		return false;

	return pVehicle->NPC_CanEnterVehicle( this, false );
}

//-----------------------------------------------------------------------------
// Purpose: FIXME: Move into behavior?
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CWretch::VehicleLeapAttackTouch( CBaseEntity *pOther )
{
	if ( pOther->GetServerVehicle() )
	{
		m_PassengerBehavior.AttachToVehicle();

		// HACK: Stop us cold
		SetLocalVelocity( vec3_origin );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Determine whether we're in a vehicle or not
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWretch::IsInAVehicle( void )
{
	// Must be active and getting in/out of vehicle
	if ( m_PassengerBehavior.IsEnabled() && m_PassengerBehavior.GetPassengerState() != PASSENGER_STATE_OUTSIDE )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Override our efficiency so that we don't jitter when we're in the middle
//			of our enter/exit animations.
// Input  : bInPVS - Whether we're in the PVS or not
//-----------------------------------------------------------------------------
void CWretch::UpdateEfficiency( bool bInPVS )
{ 
	// If we're transitioning and in the PVS, we override our efficiency
	if ( IsInAVehicle() && bInPVS )
	{
		PassengerState_e nState = m_PassengerBehavior.GetPassengerState();
		if ( nState == PASSENGER_STATE_ENTERING || nState == PASSENGER_STATE_EXITING )
		{
			SetEfficiency( AIE_NORMAL );
			return;
		}
	}

	// Do the default behavior
	BaseClass::UpdateEfficiency( bInPVS );
}

#endif	// HL2_EPISODIC
//=============================================================================

//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_wretch, CWretch )

	DECLARE_ACTIVITY( ACT_WRETCH_LEAP_SOAR )
	DECLARE_ACTIVITY( ACT_WRETCH_LEAP_STRIKE )
	DECLARE_ACTIVITY( ACT_WRETCH_LAND_RIGHT )
	DECLARE_ACTIVITY( ACT_WRETCH_LAND_LEFT )
	DECLARE_ACTIVITY( ACT_WRETCH_FRENZY )
	DECLARE_ACTIVITY( ACT_WRETCH_BIG_SLASH )
	
	DECLARE_TASK( TASK_WRETCH_DO_ATTACK )
	DECLARE_TASK( TASK_WRETCH_LAND_RECOVER )
	DECLARE_TASK( TASK_WRETCH_UNSTICK_JUMP )
	DECLARE_TASK( TASK_WRETCH_JUMP_BACK )
	DECLARE_TASK( TASK_WRETCH_VERIFY_ATTACK )

	DECLARE_CONDITION( COND_WRETCH_CLIMB_TOUCH )

	//Adrian: events go here
	DECLARE_ANIMEVENT( AE_WRETCH_LEAP )
	DECLARE_ANIMEVENT( AE_WRETCH_GALLOP_LEFT )
	DECLARE_ANIMEVENT( AE_WRETCH_GALLOP_RIGHT )
	DECLARE_ANIMEVENT( AE_WRETCH_CLIMB_LEFT )
	DECLARE_ANIMEVENT( AE_WRETCH_CLIMB_RIGHT )

#ifdef HL2_EPISODIC
	// FIXME: Move!
	DECLARE_ANIMEVENT( AE_PASSENGER_PHYSICS_PUSH2 )
	DECLARE_ANIMEVENT( AE_WRETCH_VEHICLE_LEAP )
	DECLARE_ANIMEVENT( AE_WRETCH_VEHICLE_SS_DIE )
#endif	// HL2_EPISODIC

	//=========================================================
	// 
	//=========================================================

	//=========================================================
	// I have landed somewhere that's pathfinding-unfriendly
	// just try to jump out.
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_WRETCH_UNSTICK_JUMP,

		"	Tasks"
		"		TASK_WRETCH_UNSTICK_JUMP	0"
		"	"
		"	Interrupts"
	)

	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_WRETCH_CLIMBING_UNSTICK_JUMP,

		"	Tasks"
		"		TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE"
		"		TASK_WRETCH_UNSTICK_JUMP	0"
		"	"
		"	Interrupts"
	)


AI_END_CUSTOM_NPC()


