//=========================================================
// Opposing Forces Monster Shocktrooper
//
// Made by Demiurge
//
//FGD monster_shocktrooper
//=========================================================


//=========================================================
// Hit groups!	
//=========================================================
/*

  1 - Head
  2 - Stomach
  3 - Gun

*/


#include	"extdll.h"
#include	"plane.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include	"squadmonster.h"
#include	"weapons.h"
#include	"talkmonster.h"
#include	"soundent.h"
#include	"effects.h"
#include	"customentity.h"
#include	"shock.h"

int g_fTrooperQuestion;				// true if an idle Trooper asked a question. Cleared when someone answers.

extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define	TROOPER_CLIP_SIZE					8 // how many bullets in a clip? - NOTE: 3 round burst sound, so keep as 3 * x!
#define TROOPER_VOL						0.35		// volume of grunt sounds
#define TROOPER_ATTN						ATTN_NORM	// attenutation of grunt sentences
#define STROOPER_LIMP_HEALTH				20
#define STROOPER_DMG_HEADSHOT				( DMG_SHOCK | DMG_CLUB )	// damage types that can kill a grunt with a single headshot.
#define STROOPER_NUM_HEADS				2 // how many grunt heads are there? 
#define STROOPER_MINIMUM_HEADSHOT_DAMAGE	15 // must do at least this much damage in one shot to head to score a headshot kill
#define	STROOPER_SENTENCE_VOLUME			(float)0.35 // volume of grunt sentences

#define STROOPER_SHOCK				1
#define STROOPER_SPOREGRENADE		2

#define GUN_SHOCK					1
#define	GUN_NONE					2

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		STROOPER_AE_RELOAD			( 2 )
#define		STROOPER_AE_KICK			( 3 )
#define		STROOPER_AE_BURST1			( 4 )
#define		STROOPER_AE_BURST2			( 5 ) 
#define		STROOPER_AE_BURST3			( 6 ) 
#define		STROOPER_AE_GREN_TOSS		( 7 )
#define		STROOPER_AE_GREN_DROP		( 9 )
#define		STROOPER_AE_CAUGHT_ENEMY	( 10) // grunt established sight with an enemy (player only) that had previously eluded the squad.
#define		STROOPER_AE_DROP_GUN		( 11) // grunt (probably dead) is dropping his mp5.

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_TROOPER_SUPPRESS = LAST_COMMON_SCHEDULE + 1,
	SCHED_TROOPER_ESTABLISH_LINE_OF_FIRE,// move to a location to set up an attack against the enemy. (usually when a friendly is in the way).
	SCHED_TROOPER_COVER_AND_RELOAD,
	SCHED_TROOPER_SWEEP,
	SCHED_TROOPER_FOUND_ENEMY,
	SCHED_TROOPER_WAIT_FACE_ENEMY,
	SCHED_TROOPER_TAKECOVER_FAILED,// special schedule type that forces analysis of conditions and picks the best possible schedule to recover from this type of failure.
	SCHED_TROOPER_ELOF_FAIL,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum 
{
	TASK_TROOPER_FACE_TOSS_DIR = LAST_COMMON_TASK + 1,
	TASK_TROOPER_SPEAK_SENTENCE,
	TASK_TROOPER_CHECK_FIRE,
};

//=========================================================
// monster-specific conditions
//=========================================================
#define bits_COND_TROOPER_NOFIRE	( bits_COND_SPECIAL1 )
#define BIG_CHILDCLASS		"monster_shockroach"

class CSTrooper : public CSquadMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed ( void );
	int  Classify ( void );
	int ISoundMask ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL FCanCheckAttacks ( void );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist );
	void CheckAmmo ( void );
	void SetActivity ( Activity NewActivity );
	void StartTask ( Task_t *pTask );
	void RunTask ( Task_t *pTask );
	void DeathSound( void );
	void PainSound( void );
	void IdleSound ( void );
	Vector GetGunPosition( void );
	void Shoot ( void );
	void PrescheduleThink ( void );
//	void GibMonster( void );
	void Killed( entvars_t *pevAttacker, int iGib );
	void SpeakSentence( void );

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );
	
	CBaseEntity	*Kick( void );
	Schedule_t	*GetSchedule( void );
	Schedule_t  *GetScheduleOfType ( int Type );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	int IRelationship ( CBaseEntity *pTarget );

	BOOL FOkToSpeak( void );
	void JustSpoke( void );

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	// checking the feasibility of a grenade toss is kind of costly, so we do it every couple of seconds,
	// not every server frame.
	float m_flNextGrenadeCheck;
	float m_flNextPainTime;
	float m_flLastEnemySightTime;

	Vector	m_vecTossVelocity;

	BOOL	m_fThrowGrenade;
	BOOL	m_fStanding;
	BOOL	m_fFirstEncounter;// only put on the handsign show in the squad's first encounter.
	int		m_cClipSize;

	int m_voicePitch;
	int m_iVgib;

	int		m_iSentence;

	static const char *pTrooperSentences[];
};

LINK_ENTITY_TO_CLASS( monster_shock_trooper, CSTrooper );

TYPEDESCRIPTION	CSTrooper::m_SaveData[] = 
{
	DEFINE_FIELD( CSTrooper, m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( CSTrooper, m_flNextPainTime, FIELD_TIME ),
	DEFINE_FIELD( CSTrooper, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CSTrooper, m_fThrowGrenade, FIELD_BOOLEAN ),
	DEFINE_FIELD( CSTrooper, m_fStanding, FIELD_BOOLEAN ),
	DEFINE_FIELD( CSTrooper, m_fFirstEncounter, FIELD_BOOLEAN ),
	DEFINE_FIELD( CSTrooper, m_cClipSize, FIELD_INTEGER ),
	DEFINE_FIELD( CSTrooper, m_voicePitch, FIELD_INTEGER ),
	DEFINE_FIELD( CSTrooper, m_iSentence, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CSTrooper, CSquadMonster );

const char *CSTrooper::pTrooperSentences[] = 
{
	"ST_GREN", // grenade scared grunt
	"ST_ALERT", // sees player
	"ST_MONSTER", // sees monster
	"ST_COVER", // running to cover
	"ST_THROW", // about to throw grenade
	"ST_CHARGE",  // running out to get the enemy
	"ST_TAUNT", // say rude things
};

enum
{
	STROOPER_SENT_NONE = -1,
	STROOPER_SENT_GREN = 0,
	STROOPER_SENT_ALERT,
	STROOPER_SENT_MONSTER,
	STROOPER_SENT_COVER,
	STROOPER_SENT_THROW,
	STROOPER_SENT_CHARGE,
	STROOPER_SENT_TAUNT,
} STROOPER_SENTENCE_TYPES;

//=========================================================
// Speak Sentence - say your cued up sentence.
//
// Some grunt sentences (take cover and charge) rely on actually
// being able to execute the intended action. It's really lame
// when a grunt says 'COVER ME' and then doesn't move. The problem
// is that the sentences were played when the decision to TRY
// to move to cover was made. Now the sentence is played after 
// we know for sure that there is a valid path. The schedule
// may still fail but in most cases, well after the grunt has 
// started moving.
//=========================================================
void CSTrooper :: SpeakSentence( void )
{
	if ( m_iSentence == STROOPER_SENT_NONE )
	{
		// no sentence cued up.
		return; 
	}

	if (FOkToSpeak())
	{
		SENTENCEG_PlayRndSz( ENT(pev), pTrooperSentences[ m_iSentence ], STROOPER_SENTENCE_VOLUME, TROOPER_ATTN, 0, m_voicePitch);
		JustSpoke();
	}
}

//=========================================================
// IRelationship - overridden because Alien Troopers are 
// Human Trooper's nemesis.
//=========================================================
int CSTrooper::IRelationship ( CBaseEntity *pTarget )
{
	if ( FClassnameIs( pTarget->pev, "monster_alien_grunt" ) || ( FClassnameIs( pTarget->pev,  "monster_gargantua" ) ) || ( FClassnameIs( pTarget->pev,  "monster_gargantua_friend" ) ) || ( FClassnameIs( pTarget->pev,  "monster_babygarg" ) ) || ( FClassnameIs( pTarget->pev,  "monster_babygarg_friend" ) ) )
	{
		return R_NM;
	}

	return CSquadMonster::IRelationship( pTarget );
}

//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
/*void CSTrooper :: GibMonster ( void )
{
	Vector	vecGunPos;
	Vector	vecGunAngles;

	if ( GetBodygroup( 2 ) != 2 )
	{// throw a gun if the grunt has one
		GetAttachment( 0, vecGunPos, vecGunAngles );
		
			//DropItem( "weapon_9mmAR", vecGunPos, vecGunAngles );
	CBaseEntity *pChild = CBaseEntity::Create( "monster_shockroach", pev->origin, pev->angles, edict() );
	}

	Vector vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;		

		// gibs
		vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
			WRITE_BYTE( TE_BREAKMODEL);

			// position
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z );

			// size
			WRITE_COORD( 100 );
			WRITE_COORD( 100 );
			WRITE_COORD( 32 );

			// velocity
			WRITE_COORD( 0 ); 
			WRITE_COORD( 0 );
			WRITE_COORD( 200 );

			// randomization
			WRITE_BYTE( 8 ); 

			// Model
			WRITE_SHORT( m_iVgib );	//model id#

			// # of shards
			WRITE_BYTE( 8 );

			// duration
			WRITE_BYTE( 200 );// 10.0 seconds

			// flags

			WRITE_BYTE( BREAK_FLESH );
		MESSAGE_END();

		SetThink( SUB_Remove );
		pev->nextthink = gpGlobals->time + 0.1;


	CBaseMonster :: GibMonster();
}*/

//=========================================================
// ISoundMask - Overidden for human grunts because they 
// hear the DANGER sound that is made by hand grenades and
// other dangerous items.
//=========================================================
int CSTrooper :: ISoundMask ( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_PLAYER	|
			bits_SOUND_DANGER;
}

//=========================================================
// someone else is talking - don't speak
//=========================================================
BOOL CSTrooper :: FOkToSpeak( void )
{
// if someone else is talking, don't speak
	if (gpGlobals->time <= CTalkMonster::g_talkWaitTime)
		return FALSE;

	if ( pev->spawnflags & SF_MONSTER_GAG )
	{
		if ( m_MonsterState != MONSTERSTATE_COMBAT )
		{
			// no talking outside of combat if gagged.
			return FALSE;
		}
	}

	// if player is not in pvs, don't speak
//	if (FNullEnt(FIND_CLIENT_IN_PVS(edict())))
//		return FALSE;
	
	return TRUE;
}

//=========================================================
//=========================================================
void CSTrooper :: JustSpoke( void )
{
	CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(1.5, 2.0);
	m_iSentence = STROOPER_SENT_NONE;
}

//=========================================================
// PrescheduleThink - this function runs after conditions
// are collected and before scheduling code is run.
//=========================================================
void CSTrooper :: PrescheduleThink ( void )
{
	if ( InSquad() && m_hEnemy != NULL )
	{
		if ( HasConditions ( bits_COND_SEE_ENEMY ) )
		{
			// update the squad's last enemy sighting time.
			MySquadLeader()->m_flLastEnemySightTime = gpGlobals->time;
		}
		else
		{
			if ( gpGlobals->time - MySquadLeader()->m_flLastEnemySightTime > 5 )
			{
				// been a while since we've seen the enemy
				MySquadLeader()->m_fEnemyEluded = TRUE;
			}
		}
	}
}

//=========================================================
// FCanCheckAttacks - this is overridden for human grunts
// because they can throw/shoot grenades when they can't see their
// target and the base class doesn't check attacks if the monster
// cannot see its enemy.
//
// !!!BUGBUG - this gets called before a 3-round burst is fired
// which means that a friendly can still be hit with up to 2 rounds. 
// ALSO, grenades will not be tossed if there is a friendly in front,
// this is a bad bug. Friendly machine gun fire avoidance
// will unecessarily prevent the throwing of a grenade as well.
//=========================================================
BOOL CSTrooper :: FCanCheckAttacks ( void )
{
	if ( !HasConditions( bits_COND_ENEMY_TOOFAR ) )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


//=========================================================
// CheckMeleeAttack1
//=========================================================
BOOL CSTrooper :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	CBaseMonster *pEnemy;

	if ( m_hEnemy != NULL )
	{
		pEnemy = m_hEnemy->MyMonsterPointer();

		if ( !pEnemy )
		{
			return FALSE;
		}
	}

	if ( flDist <= 64 && flDot >= 0.7	&& 
		 pEnemy->Classify() != CLASS_ALIEN_BIOWEAPON &&
		 pEnemy->Classify() != CLASS_PLAYER_BIOWEAPON )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack1 - overridden for STrooper, cause 
// FCanCheckAttacks() doesn't disqualify all attacks based
// on whether or not the enemy is occluded because unlike
// the base class, the STrooper can attack when the enemy is
// occluded (throw grenade over wall, etc). We must 
// disqualify the machine gun attack if the enemy is occluded.
//=========================================================
BOOL CSTrooper :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= 2048 && flDot >= 0.5 && NoFriendlyFire() )
	{
		TraceResult	tr;

		if ( !m_hEnemy->IsPlayer() && flDist <= 64 )
		{
			// kick nonclients, but don't shoot at them.
			return FALSE;
		}

		Vector vecSrc = GetGunPosition();

		// verify that a bullet fired from the gun will hit the enemy before the world.
		UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget(vecSrc), ignore_monsters, ignore_glass, ENT(pev), &tr);

		if ( tr.flFraction == 1.0 )
		{
			return TRUE;
		}
	}

	return FALSE;
}

//=========================================================
// CheckRangeAttack2 - this checks the Trooper's grenade
// attack. 
//=========================================================
BOOL CSTrooper :: CheckRangeAttack2 ( float flDot, float flDist )
{
	
	// if the grunt isn't moving, it's ok to check.
	if ( m_flGroundSpeed != 0 )
	{
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

	// assume things haven't changed too much since last time
	if (gpGlobals->time < m_flNextGrenadeCheck )
	{
		return m_fThrowGrenade;
	}

	if ( !FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND ) && m_hEnemy->pev->waterlevel == 0 && m_vecEnemyLKP.z > pev->absmax.z  )
	{
		//!!!BUGBUG - we should make this check movetype and make sure it isn't FLY? Players who jump a lot are unlikely to 
		// be grenaded.
		// don't throw grenades at anything that isn't on the ground!
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}
	
	Vector vecTarget;

	if (FBitSet( pev->weapons, STROOPER_SPOREGRENADE))
	{
		// find feet
		if (RANDOM_LONG(0,1))
		{
			// magically know where they are
			vecTarget = Vector( m_hEnemy->pev->origin.x, m_hEnemy->pev->origin.y, m_hEnemy->pev->absmin.z );
		}
		else
		{
			// toss it to where you last saw them
			vecTarget = m_vecEnemyLKP;
		}
		// vecTarget = m_vecEnemyLKP + (m_hEnemy->BodyTarget( pev->origin ) - m_hEnemy->pev->origin);
		// estimate position
		// vecTarget = vecTarget + m_hEnemy->pev->velocity * 2;
	}
	else
	{
		// find target
		// vecTarget = m_hEnemy->BodyTarget( pev->origin );
		vecTarget = m_vecEnemyLKP + (m_hEnemy->BodyTarget( pev->origin ) - m_hEnemy->pev->origin);
		// estimate position
		if (HasConditions( bits_COND_SEE_ENEMY))
			vecTarget = vecTarget + ((vecTarget - pev->origin).Length() / gSkillData.shocktrooperGrenadeSpeed) * m_hEnemy->pev->velocity;
	}

	// are any of my squad members near the intended grenade impact area?
	if ( InSquad() )
	{
		if (SquadMemberInRange( vecTarget, 256 ))
		{
			// crap, I might blow my own guy up. Don't throw a grenade and don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
			m_fThrowGrenade = FALSE;
		}
	}
	
	if ( ( vecTarget - pev->origin ).Length2D() <= 256 )
	{
		// crap, I don't want to blow myself up
		m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

		
	if (FBitSet( pev->weapons, STROOPER_SPOREGRENADE))
	{
		Vector vecToss = VecCheckToss( pev, GetGunPosition(), vecTarget, 0.5 );

		if ( vecToss != g_vecZero )
		{
			m_vecTossVelocity = vecToss;

			// throw a hand grenade
			m_fThrowGrenade = TRUE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time; // 1/3 second.
		}
		else
		{
			// don't throw
			m_fThrowGrenade = FALSE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		}
	}
	else
	{
		Vector vecToss = VecCheckThrow( pev, GetGunPosition(), vecTarget, gSkillData.shocktrooperGrenadeSpeed, 0.5 );

		if ( vecToss != g_vecZero )
		{
			m_vecTossVelocity = vecToss;

			// throw a hand grenade
			m_fThrowGrenade = TRUE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 0.3; // 1/3 second.
		}
		else
		{
			// don't throw
			m_fThrowGrenade = FALSE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		}
	}

	

	return m_fThrowGrenade;
}


//=========================================================
// TraceAttack - make sure we're not taking it in the helmet
//=========================================================
void CSTrooper :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	// check for helmet shot
	if (ptr->iHitgroup == 11)
	{
		// make sure we're wearing one
		if (GetBodygroup( 1 ) == (bitsDamageType & (DMG_SHOCK | DMG_SLASH | DMG_BLAST | DMG_CLUB)))
		{
			// absorb damage
			flDamage -= 20;
			if (flDamage <= 0)
			{
				UTIL_Ricochet( ptr->vecEndPos, 1.0 );
				flDamage = 0.01;
			}
		}
		// it's head shot anyways
		ptr->iHitgroup = HITGROUP_HEAD;
	}
	CSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


//=========================================================
// TakeDamage - overridden for the grunt because the grunt
// needs to forget that he is in cover if he's hurt. (Obviously
// not in a safe place anymore).
//=========================================================
int CSTrooper :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	Forget( bits_MEMORY_INCOVER );

	return CSquadMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CSTrooper :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_IDLE:	
		ys = 150;		
		break;
	case ACT_RUN:	
		ys = 150;	
		break;
	case ACT_WALK:	
		ys = 180;		
		break;
	case ACT_RANGE_ATTACK1:	
		ys = 120;	
		break;
	case ACT_RANGE_ATTACK2:	
		ys = 120;	
		break;
	case ACT_MELEE_ATTACK1:	
		ys = 120;	
		break;
	case ACT_MELEE_ATTACK2:	
		ys = 120;	
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:	
		ys = 180;
		break;
	case ACT_GLIDE:
	case ACT_FLY:
		ys = 30;
		break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}

void CSTrooper :: IdleSound( void )
{
	if (FOkToSpeak() && (g_fTrooperQuestion || RANDOM_LONG(0,1)))
	{
		if (!g_fTrooperQuestion)
		{
			// ask question or make statement
			switch (RANDOM_LONG(0,2))
			{
			case 0: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "ST_CHECK", STROOPER_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				g_fTrooperQuestion = 1;
				break;
			case 1: // question
				SENTENCEG_PlayRndSz(ENT(pev), "ST_QUEST", STROOPER_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				g_fTrooperQuestion = 2;
				break;
			case 2: // statement
				SENTENCEG_PlayRndSz(ENT(pev), "ST_IDLE", STROOPER_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			}
		}
		else
		{
			switch (g_fTrooperQuestion)
			{
			case 1: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "ST_CLEAR", STROOPER_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			case 2: // question 
				SENTENCEG_PlayRndSz(ENT(pev), "ST_ANSWER", STROOPER_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			}
			g_fTrooperQuestion = 0;
		}
		JustSpoke();
	}
}

//=========================================================
// CheckAmmo - overridden for the grunt because he actually
// uses ammo! (base class doesn't)
//=========================================================
void CSTrooper :: CheckAmmo ( void )
{
	if ( m_cAmmoLoaded <= 0 )
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CSTrooper :: Classify ( void )
{
	return	CLASS_ALIEN_MILITARY;
}

//=========================================================
//=========================================================
CBaseEntity *CSTrooper :: Kick( void )
{
	TraceResult tr;

	UTIL_MakeVectors( pev->angles );
	Vector vecStart = pev->origin;
	vecStart.z += pev->size.z * 0.5;
	Vector vecEnd = vecStart + (gpGlobals->v_forward * 70);

	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr );
	
	if ( tr.pHit )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		return pEntity;
	}

	return NULL;
}

//=========================================================
// GetGunPosition	return the end of the barrel
//=========================================================

Vector CSTrooper :: GetGunPosition( )
{
	if (m_fStanding )
	{
		return pev->origin + Vector( 0, 0, 60 );
	}
	else
	{
		return pev->origin + Vector( 0, 0, 48 );
	}
}

//=========================================================
// Shoot
//=========================================================
void CSTrooper :: Shoot ( void )
{
	if (m_hEnemy == NULL)
	{
		return;
	}
			Vector vecGunPos, vecGunDir;
			Vector vecDirToEnemy;
			Vector angDir;

	UTIL_MakeAimVectors ( pev->angles );

				m_vecEnemyLKP = m_hEnemy->BodyTarget( pev->origin );
				vecDirToEnemy = ( (( m_vecEnemyLKP ) - pev->origin) + gpGlobals->v_up * -25 +gpGlobals->v_right * -20 ).Normalize();

		if (m_fStanding )
		{
				vecDirToEnemy = ( (( m_vecEnemyLKP ) - pev->origin) + gpGlobals->v_up * -50 +gpGlobals->v_right * -10 ).Normalize();
		}
				vecDirToEnemy.z = vecDirToEnemy.z * -1;
	pev->effects |= EF_MUZZLEFLASH;
	
	m_cAmmoLoaded--;// take away a bullet!

	GetAttachment( 1, vecGunPos, vecGunDir );

			CBaseEntity *pShock = CBaseEntity::Create( "shock", vecGunPos, UTIL_VecToAngles( vecDirToEnemy ), edict() );
			UTIL_MakeVectors ( pShock->pev->angles );
			pShock->pev->velocity = gpGlobals->v_forward * 1300;

		CBaseMonster *pShockMonster = pShock->MyMonsterPointer();

		if ( pShockMonster )
		{
			pShockMonster->m_hEnemy = m_hEnemy;
		}

}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CSTrooper :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch( pEvent->event )
	{
		case STROOPER_AE_DROP_GUN:
			{
			Vector	vecGunPos;
			Vector	vecGunAngles;

			GetAttachment( 0, vecGunPos, vecGunAngles );

			// switch to body group with no gun.
			//SetBodygroup( GUN_NONE, GUN_NONE );
			pev->body = 1;

			// now spawn a gun.
		
				 //DropItem( "weapon_9mmAR", vecGunPos, vecGunAngles );
				//CBaseEntity *pChild = CBaseEntity::Create( "monster_shockroach", pev->origin, pev->angles, edict() );
			}
			break;

		case STROOPER_AE_RELOAD:
			m_cAmmoLoaded = m_cClipSize;
			ClearConditions(bits_COND_NO_AMMO_LOADED);
			break;

		case STROOPER_AE_GREN_TOSS:
		{
			UTIL_MakeVectors( pev->angles );
			// CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 34 + Vector (0, 0, 32), m_vecTossVelocity, 3.5 );
			CSpore::Spore( pev, GetGunPosition(), m_vecTossVelocity, 3.5 );

			m_fThrowGrenade = FALSE;
			m_flNextGrenadeCheck = gpGlobals->time + 6;// wait six seconds before even looking again to see if a grenade can be thrown.
			// !!!LATER - when in a group, only try to throw grenade if ordered.
		}
		break;

		case STROOPER_AE_GREN_DROP:
		{
			UTIL_MakeVectors( pev->angles );
			CSpore::Spore( pev, pev->origin + gpGlobals->v_forward * 17 - gpGlobals->v_right * 27 + gpGlobals->v_up * 6, g_vecZero, 3 );
		}
		break;

		case STROOPER_AE_BURST1:
		{
				Shoot();

				// the first round of the three round burst plays the sound and puts a sound in the world sound list.
					EMIT_SOUND( ENT(pev), CHAN_WEAPON, "shocktrooper/shock_trooper_attack.wav", 1, ATTN_NORM );
		
			CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
		}
		break;

		case STROOPER_AE_BURST2:
		case STROOPER_AE_BURST3:
			Shoot();
			break;

		case STROOPER_AE_KICK:
		{
			CBaseEntity *pHurt = Kick();

			if ( pHurt )
			{
				// SOUND HERE!
				UTIL_MakeVectors( pev->angles );
				pHurt->pev->punchangle.x = 15;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
				pHurt->TakeDamage( pev, pev, gSkillData.shocktrooperDmgKick, DMG_CLUB );
			}
		}
		break;

		case STROOPER_AE_CAUGHT_ENEMY:
		{
			if ( FOkToSpeak() )
			{
				SENTENCEG_PlayRndSz(ENT(pev), "ST_ALERT", STROOPER_SENTENCE_VOLUME, TROOPER_ATTN, 0, m_voicePitch);
				 JustSpoke();
			}

		}

		default:
			CSquadMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CSTrooper :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/strooper.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_YELLOW;
	pev->effects		= 0;
	pev->health			= gSkillData.shocktrooperHealth;
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime	= gpGlobals->time;
	m_iSentence			= STROOPER_SENT_NONE;

	m_afCapability		= bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_fEnemyEluded		= FALSE;
	m_fFirstEncounter	= TRUE;// this is true when the grunt spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector ( 0, 0, 55 );

		pev->weapons = STROOPER_SHOCK | STROOPER_SPOREGRENADE;

		m_cClipSize		= TROOPER_CLIP_SIZE;
	m_cAmmoLoaded		= m_cClipSize;


	CTalkMonster::g_talkWaitTime = 0;

	MonsterInit();
}

void CSTrooper::Killed( entvars_t *pevAttacker, int iGib )
{
	CBaseEntity *pChild = CBaseEntity::Create( "monster_shockroach", pev->origin, pev->angles, edict() );
	Vector vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;		

		// gibs
		vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
			WRITE_BYTE( TE_BREAKMODEL);

			// position
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z );

			// size
			WRITE_COORD( 100 );
			WRITE_COORD( 100 );
			WRITE_COORD( 32 );

			// velocity
			WRITE_COORD( 0 ); 
			WRITE_COORD( 0 );
			WRITE_COORD( 200 );

			// randomization
			WRITE_BYTE( 8 ); 

			// Model
			WRITE_SHORT( m_iVgib );	//model id#

			// # of shards
			WRITE_BYTE( 8 );

			// duration
			WRITE_BYTE( 50 );// 10.0 seconds

			// flags

			WRITE_BYTE( BREAK_FLESH );
		MESSAGE_END();

		SetThink( SUB_Remove );
		pev->nextthink = gpGlobals->time + 0.1;

	CBaseMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CSTrooper :: Precache()
{
	PRECACHE_MODEL("models/strooper.mdl");
	m_iVgib = PRECACHE_MODEL("models/strooper_gibs.mdl");

	PRECACHE_SOUND( "shocktrooper/shock_trooper_attack.wav" );
	
	PRECACHE_SOUND( "shocktrooper/shock_trooper_die1.wav" );
	PRECACHE_SOUND( "shocktrooper/shock_trooper_die2.wav" );
	PRECACHE_SOUND( "shocktrooper/shock_trooper_die3.wav" );
	PRECACHE_SOUND( "shocktrooper/shock_trooper_die4.wav" );

	PRECACHE_SOUND( "shocktrooper/shock_trooper_pain1.wav" );
	PRECACHE_SOUND( "shocktrooper/shock_trooper_pain2.wav" );
	PRECACHE_SOUND( "shocktrooper/shock_trooper_pain3.wav" );
	PRECACHE_SOUND( "shocktrooper/shock_trooper_pain4.wav" );
	PRECACHE_SOUND( "shocktrooper/shock_trooper_pain5.wav" );

	PRECACHE_SOUND( "weapons/sbarrel1.wav" );

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	UTIL_PrecacheOther( "shock" );
	UTIL_PrecacheOther( "monster_shockroach" );
	// get voice pitch
	if (RANDOM_LONG(0,1))
		m_voicePitch = 109 + RANDOM_LONG(0,7);
	else
		m_voicePitch = 100;

}	

//=========================================================
// start task
//=========================================================
void CSTrooper :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_TROOPER_CHECK_FIRE:
		if ( !NoFriendlyFire() )
		{
			SetConditions( bits_COND_TROOPER_NOFIRE );
		}
		TaskComplete();
		break;

	case TASK_TROOPER_SPEAK_SENTENCE:
		SpeakSentence();
		TaskComplete();
		break;
	
	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		// grunt no longer assumes he is covered if he moves
		Forget( bits_MEMORY_INCOVER );
		CSquadMonster ::StartTask( pTask );
		break;

	case TASK_RELOAD:
		m_IdealActivity = ACT_RELOAD;
		break;

	case TASK_TROOPER_FACE_TOSS_DIR:
		break;

	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		CSquadMonster :: StartTask( pTask );
		if (pev->movetype == MOVETYPE_FLY)
		{
			m_IdealActivity = ACT_GLIDE;
		}
		break;

	default: 
		CSquadMonster :: StartTask( pTask );
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CSTrooper :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_TROOPER_FACE_TOSS_DIR:
		{
			// project a point along the toss vector and turn to face that point.
			MakeIdealYaw( pev->origin + m_vecTossVelocity * 64 );
			ChangeYaw( pev->yaw_speed );

			if ( FacingIdeal() )
			{
				m_iTaskStatus = TASKSTATUS_COMPLETE;
			}
			break;
		}
	default:
		{
			CSquadMonster :: RunTask( pTask );
			break;
		}
	}
}

//=========================================================
// PainSound
//=========================================================
void CSTrooper :: PainSound ( void )
{
	if ( gpGlobals->time > m_flNextPainTime )
	{
#if 0
		if ( RANDOM_LONG(0,99) < 5 )
		{
			// pain sentences are rare
			if (FOkToSpeak())
			{
				SENTENCEG_PlayRndSz(ENT(pev), "ST_PAIN", STROOPER_SENTENCE_VOLUME, ATTN_NORM, 0, PITCH_NORM);
				JustSpoke();
				return;
			}
		}
#endif 
		switch ( RANDOM_LONG(0,6) )
		{
		case 0:	
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "shocktrooper/shock_trooper_pain3.wav", 1, ATTN_NORM );	
			break;
		case 1:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "shocktrooper/shock_trooper_pain4.wav", 1, ATTN_NORM );	
			break;
		case 2:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "shocktrooper/shock_trooper_pain5.wav", 1, ATTN_NORM );	
			break;
		case 3:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "shocktrooper/shock_trooper_pain1.wav", 1, ATTN_NORM );	
			break;
		case 4:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "shocktrooper/shock_trooper_pain2.wav", 1, ATTN_NORM );	
			break;
		}

		m_flNextPainTime = gpGlobals->time + 1;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CSTrooper :: DeathSound ( void )
{
	switch ( RANDOM_LONG(0,3) )
	{
	case 0:	
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "shocktrooper/shock_trooper_die1.wav", 1, ATTN_IDLE );	
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "shocktrooper/shock_trooper_die2.wav", 1, ATTN_IDLE );	
		break;
	case 2:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "shocktrooper/shock_trooper_die3.wav", 1, ATTN_IDLE );	
		break;
	case 3:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "shocktrooper/shock_trooper_die4.wav", 1, ATTN_IDLE );	
		break;
	}
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// TrooperFail
//=========================================================
Task_t	tlTrooperFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slTrooperFail[] =
{
	{
		tlTrooperFail,
		ARRAYSIZE ( tlTrooperFail ),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2,
		0,
		"Trooper Fail"
	},
};

//=========================================================
// Trooper Combat Fail
//=========================================================
Task_t	tlTrooperCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slTrooperCombatFail[] =
{
	{
		tlTrooperCombatFail,
		ARRAYSIZE ( tlTrooperCombatFail ),
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"Trooper Combat Fail"
	},
};

//=========================================================
// Victory dance!
//=========================================================
Task_t	tlTrooperVictoryDance[] =
{
	{ TASK_STOP_MOVING,						(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_WAIT,							(float)1.5					},
	{ TASK_GET_PATH_TO_ENEMY_CORPSE,		(float)0					},
	{ TASK_WALK_PATH,						(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
};

Schedule_t	slTrooperVictoryDance[] =
{
	{ 
		tlTrooperVictoryDance,
		ARRAYSIZE ( tlTrooperVictoryDance ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"TrooperVictoryDance"
	},
};

//=========================================================
// Establish line of fire - move to a position that allows
// the grunt to attack.
//=========================================================
Task_t tlTrooperEstablishLineOfFire[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_TROOPER_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_TROOPER_SPEAK_SENTENCE,(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slTrooperEstablishLineOfFire[] =
{
	{ 
		tlTrooperEstablishLineOfFire,
		ARRAYSIZE ( tlTrooperEstablishLineOfFire ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"TrooperEstablishLineOfFire"
	},
};

//=========================================================
// TrooperFoundEnemy - grunt established sight with an enemy
// that was hiding from the squad.
//=========================================================
Task_t	tlTrooperFoundEnemy[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_SIGNAL1			},
};

Schedule_t	slTrooperFoundEnemy[] =
{
	{ 
		tlTrooperFoundEnemy,
		ARRAYSIZE ( tlTrooperFoundEnemy ), 
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"TrooperFoundEnemy"
	},
};

//=========================================================
// TrooperCombatFace Schedule
//=========================================================
Task_t	tlTrooperCombatFace1[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_WAIT,					(float)1.5					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_TROOPER_SWEEP	},
};

Schedule_t	slTrooperCombatFace[] =
{
	{ 
		tlTrooperCombatFace1,
		ARRAYSIZE ( tlTrooperCombatFace1 ), 
		bits_COND_NEW_ENEMY				|
		bits_COND_ENEMY_DEAD			|
		bits_COND_CAN_RANGE_ATTACK1		|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"Combat Face"
	},
};

//=========================================================
// Suppressing fire - don't stop shooting until the clip is
// empty or grunt gets hurt.
//=========================================================
Task_t	tlTrooperSignalSuppress[] =
{
	{ TASK_STOP_MOVING,					0						},
	{ TASK_FACE_IDEAL,					(float)0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float)ACT_SIGNAL2		},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_TROOPER_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_TROOPER_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_TROOPER_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_TROOPER_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_TROOPER_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
};

Schedule_t	slTrooperSignalSuppress[] =
{
	{ 
		tlTrooperSignalSuppress,
		ARRAYSIZE ( tlTrooperSignalSuppress ), 
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_TROOPER_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"SignalSuppress"
	},
};

Task_t	tlTrooperSuppress[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_TROOPER_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_TROOPER_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_TROOPER_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_TROOPER_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_TROOPER_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
};

Schedule_t	slTrooperSuppress[] =
{
	{ 
		tlTrooperSuppress,
		ARRAYSIZE ( tlTrooperSuppress ), 
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_TROOPER_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"Suppress"
	},
};


//=========================================================
// grunt wait in cover - we don't allow danger or the ability
// to attack to break a grunt's run to cover schedule, but
// when a grunt is in cover, we do want them to attack if they can.
//=========================================================
Task_t	tlTrooperWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slTrooperWaitInCover[] =
{
	{ 
		tlTrooperWaitInCover,
		ARRAYSIZE ( tlTrooperWaitInCover ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAR_SOUND		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK2,

		bits_SOUND_DANGER,
		"TrooperWaitInCover"
	},
};

//=========================================================
// run to cover.
// !!!BUGBUG - set a decent fail schedule here.
//=========================================================
Task_t	tlTrooperTakeCover1[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_TROOPER_TAKECOVER_FAILED	},
	{ TASK_WAIT,					(float)0.2							},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0							},
	{ TASK_TROOPER_SPEAK_SENTENCE,	(float)0							},
	{ TASK_RUN_PATH,				(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0							},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER			},
	{ TASK_SET_SCHEDULE,			(float)SCHED_TROOPER_WAIT_FACE_ENEMY	},
};

Schedule_t	slTrooperTakeCover[] =
{
	{ 
		tlTrooperTakeCover1,
		ARRAYSIZE ( tlTrooperTakeCover1 ), 
		0,
		0,
		"TakeCover"
	},
};

//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlTrooperGrenadeCover1[] =
{
	{ TASK_STOP_MOVING,						(float)0							},
	{ TASK_FIND_COVER_FROM_ENEMY,			(float)99							},
	{ TASK_FIND_FAR_NODE_COVER_FROM_ENEMY,	(float)384							},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_SPECIAL_ATTACK1			},
	{ TASK_CLEAR_MOVE_WAIT,					(float)0							},
	{ TASK_RUN_PATH,						(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_TROOPER_WAIT_FACE_ENEMY	},
};

Schedule_t	slTrooperGrenadeCover[] =
{
	{ 
		tlTrooperGrenadeCover1,
		ARRAYSIZE ( tlTrooperGrenadeCover1 ), 
		0,
		0,
		"GrenadeCover"
	},
};


//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlTrooperTossGrenadeCover1[] =
{
	{ TASK_FACE_ENEMY,						(float)0							},
	{ TASK_RANGE_ATTACK2, 					(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_TAKE_COVER_FROM_ENEMY	},
};

Schedule_t	slTrooperTossGrenadeCover[] =
{
	{ 
		tlTrooperTossGrenadeCover1,
		ARRAYSIZE ( tlTrooperTossGrenadeCover1 ), 
		0,
		0,
		"TossGrenadeCover"
	},
};

//=========================================================
// hide from the loudest sound source (to run from grenade)
//=========================================================
Task_t	tlTrooperTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_COWER			},// duck and cover if cannot move from explosion
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_FIND_COVER_FROM_BEST_SOUND,	(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER	},
	{ TASK_TURN_LEFT,					(float)179					},
};

Schedule_t	slTrooperTakeCoverFromBestSound[] =
{
	{ 
		tlTrooperTakeCoverFromBestSound,
		ARRAYSIZE ( tlTrooperTakeCoverFromBestSound ), 
		0,
		0,
		"TrooperTakeCoverFromBestSound"
	},
};

//=========================================================
// Trooper reload schedule
//=========================================================
Task_t	tlTrooperHideReload[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RELOAD			},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RELOAD			},
};

Schedule_t slTrooperHideReload[] = 
{
	{
		tlTrooperHideReload,
		ARRAYSIZE ( tlTrooperHideReload ),
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"TrooperHideReload"
	}
};

//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t	tlTrooperSweep[] =
{
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
};

Schedule_t	slTrooperSweep[] =
{
	{ 
		tlTrooperSweep,
		ARRAYSIZE ( tlTrooperSweep ), 
		
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_WORLD		|// sound flags
		bits_SOUND_DANGER		|
		bits_SOUND_PLAYER,

		"Trooper Sweep"
	},
};

//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlTrooperRangeAttack1A[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,		(float)ACT_CROUCH },
	{ TASK_TROOPER_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_TROOPER_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_TROOPER_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_TROOPER_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slTrooperRangeAttack1A[] =
{
	{ 
		tlTrooperRangeAttack1A,
		ARRAYSIZE ( tlTrooperRangeAttack1A ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_HEAR_SOUND		|
		bits_COND_TROOPER_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,
		
		bits_SOUND_DANGER,
		"Range Attack1A"
	},
};


//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlTrooperRangeAttack1B[] =
{
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_IDLE_ANGRY  },
	{ TASK_TROOPER_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_TROOPER_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_TROOPER_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_TROOPER_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slTrooperRangeAttack1B[] =
{
	{ 
		tlTrooperRangeAttack1B,
		ARRAYSIZE ( tlTrooperRangeAttack1B ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED	|
		bits_COND_TROOPER_NOFIRE		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"Range Attack1B"
	},
};

//=========================================================
// secondary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlTrooperRangeAttack2[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_TROOPER_FACE_TOSS_DIR,		(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RANGE_ATTACK2	},
	{ TASK_SET_SCHEDULE,			(float)SCHED_TROOPER_WAIT_FACE_ENEMY	},// don't run immediately after throwing grenade.
};

Schedule_t	slTrooperRangeAttack2[] =
{
	{ 
		tlTrooperRangeAttack2,
		ARRAYSIZE ( tlTrooperRangeAttack2 ), 
		0,
		0,
		"RangeAttack2"
	},
};



DEFINE_CUSTOM_SCHEDULES( CSTrooper )
{
	slTrooperFail,
	slTrooperCombatFail,
	slTrooperVictoryDance,
	slTrooperEstablishLineOfFire,
	slTrooperFoundEnemy,
	slTrooperCombatFace,
	slTrooperSignalSuppress,
	slTrooperSuppress,
	slTrooperWaitInCover,
	slTrooperTakeCover,
	slTrooperGrenadeCover,
	slTrooperTossGrenadeCover,
	slTrooperTakeCoverFromBestSound,
	slTrooperHideReload,
	slTrooperSweep,
	slTrooperRangeAttack1A,
	slTrooperRangeAttack1B,
	slTrooperRangeAttack2,
};

IMPLEMENT_CUSTOM_SCHEDULES( CSTrooper, CSquadMonster );

//=========================================================
// SetActivity 
//=========================================================
void CSTrooper :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_RANGE_ATTACK1:
		// grunt is either shooting standing or shooting crouched
			if ( m_fStanding )
			{
				// get aimable sequence
				iSequence = LookupSequence( "standing_mp5" );
			}
			else
			{
				// get crouching shoot
				iSequence = LookupSequence( "crouching_mp5" );
			}
		break;
	case ACT_RANGE_ATTACK2:
		// grunt is going to a secondary long range attack. This may be a thrown 
		// grenade or fired grenade, we must determine which and pick proper sequence
			// get toss anim
			iSequence = LookupSequence( "throwgrenade" );
		break;
	case ACT_RUN:
		if ( pev->health <= STROOPER_LIMP_HEALTH )
		{
			// limp!
			iSequence = LookupActivity ( ACT_RUN_HURT );
		}
		else
		{
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_WALK:
		if ( pev->health <= STROOPER_LIMP_HEALTH )
		{
			// limp!
			iSequence = LookupActivity ( ACT_WALK_HURT );
		}
		else
		{
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_IDLE:
		if ( m_MonsterState == MONSTERSTATE_COMBAT )
		{
			NewActivity = ACT_IDLE_ANGRY;
		}
		iSequence = LookupActivity ( NewActivity );
		break;
	default:
		iSequence = LookupActivity ( NewActivity );
		break;
	}
	
	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if ( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}

		pev->sequence		= iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo( );
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT ( at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity );
		pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}
}

//=========================================================
// Get Schedule!
//=========================================================
Schedule_t *CSTrooper :: GetSchedule( void )
{

	// clear old sentence
	m_iSentence = STROOPER_SENT_NONE;

	// grunts place HIGH priority on running away from danger sounds.
	if ( HasConditions(bits_COND_HEAR_SOUND) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if ( pSound)
		{
			if (pSound->m_iType & bits_SOUND_DANGER)
			{
				// dangerous sound nearby!
				
				//!!!KELLY - currently, this is the grunt's signal that a grenade has landed nearby,
				// and the grunt should find cover from the blast
				// good place for "SHIT!" or some other colorful verbal indicator of dismay.
				// It's not safe to play a verbal order here "Scatter", etc cause 
				// this may only affect a single individual in a squad. 
				
				if (FOkToSpeak())
				{
					SENTENCEG_PlayRndSz( ENT(pev), "ST_GREN", STROOPER_SENTENCE_VOLUME, TROOPER_ATTN, 0, m_voicePitch);
					JustSpoke();
				}
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
			}
			/*
			if (!HasConditions( bits_COND_SEE_ENEMY ) && ( pSound->m_iType & (bits_SOUND_PLAYER | bits_SOUND_COMBAT) ))
			{
				MakeIdealYaw( pSound->m_vecOrigin );
			}
			*/
		}
	}
	switch	( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

// new enemy
			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{
				if ( InSquad() )
				{
					MySquadLeader()->m_fEnemyEluded = FALSE;

					if ( !IsLeader() )
					{
						return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
					}
					else 
					{
						//!!!KELLY - the leader of a squad of grunts has just seen the player or a 
						// monster and has made it the squad's enemy. You
						// can check pev->flags for FL_CLIENT to determine whether this is the player
						// or a monster. He's going to immediately start
						// firing, though. If you'd like, we can make an alternate "first sight" 
						// schedule where the leader plays a handsign anim
						// that gives us enough time to hear a short sentence or spoken command
						// before he starts pluggin away.
						if (FOkToSpeak())// && RANDOM_LONG(0,1))
						{
							if ((m_hEnemy != NULL) && m_hEnemy->IsPlayer())
								// player
								SENTENCEG_PlayRndSz( ENT(pev), "ST_ALERT", STROOPER_SENTENCE_VOLUME, TROOPER_ATTN, 0, m_voicePitch);
							else if ((m_hEnemy != NULL) &&
									(m_hEnemy->Classify() != CLASS_PLAYER_ALLY) && 
									(m_hEnemy->Classify() != CLASS_HUMAN_PASSIVE) && 
									(m_hEnemy->Classify() != CLASS_MACHINE))
								// monster
								SENTENCEG_PlayRndSz( ENT(pev), "ST_MONST", STROOPER_SENTENCE_VOLUME, TROOPER_ATTN, 0, m_voicePitch);

							JustSpoke();
						}
						
						if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
						{
							return GetScheduleOfType ( SCHED_TROOPER_SUPPRESS );
						}
						else
						{
							return GetScheduleOfType ( SCHED_TROOPER_ESTABLISH_LINE_OF_FIRE );
						}
					}
				}
			}
// no ammo
			else if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
			{
				//!!!KELLY - this individual just realized he's out of bullet ammo. 
				// He's going to try to find cover to run to and reload, but rarely, if 
				// none is available, he'll drop and reload in the open here. 
				return GetScheduleOfType ( SCHED_TROOPER_COVER_AND_RELOAD );
			}
			
// damaged just a little
			else if ( HasConditions( bits_COND_LIGHT_DAMAGE ) )
			{
				// if hurt:
				// 90% chance of taking cover
				// 10% chance of flinch.
				int iPercent = RANDOM_LONG(0,99);

				if ( iPercent <= 90 && m_hEnemy != NULL )
				{
					// only try to take cover if we actually have an enemy!

					//!!!KELLY - this grunt was hit and is going to run to cover.
					if (FOkToSpeak()) // && RANDOM_LONG(0,1))
					{
						//SENTENCEG_PlayRndSz( ENT(pev), "ST_COVER", STROOPER_SENTENCE_VOLUME, TROOPER_ATTN, 0, m_voicePitch);
						m_iSentence = STROOPER_SENT_COVER;
						//JustSpoke();
					}
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
				else
				{
					return GetScheduleOfType( SCHED_SMALL_FLINCH );
				}
			}
// can kick
			else if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
			}
// can shoot
			else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				if ( InSquad() )
				{
					// if the enemy has eluded the squad and a squad member has just located the enemy
					// and the enemy does not see the squad member, issue a call to the squad to waste a 
					// little time and give the player a chance to turn.
					if ( MySquadLeader()->m_fEnemyEluded && !HasConditions ( bits_COND_ENEMY_FACING_ME ) )
					{
						MySquadLeader()->m_fEnemyEluded = FALSE;
						return GetScheduleOfType ( SCHED_TROOPER_FOUND_ENEMY );
					}
				}

				if ( OccupySlot ( bits_SLOTS_STROOPER_ENGAGE ) )
				{
					// try to take an available ENGAGE slot
					return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
				}
				else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_STROOPER_GRENADE ) )
				{
					// throw a grenade if can and no engage slots are available
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
				}
				else
				{
					// hide!
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
			}
// can't see enemy
			else if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
			{
				if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_STROOPER_GRENADE ) )
				{
					//!!!KELLY - this grunt is about to throw or fire a grenade at the player. Great place for "fire in the hole"  "frag out" etc
					if (FOkToSpeak())
					{
						SENTENCEG_PlayRndSz( ENT(pev), "ST_THROW", STROOPER_SENTENCE_VOLUME, TROOPER_ATTN, 0, m_voicePitch);
						JustSpoke();
					}
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
				}
				else if ( OccupySlot( bits_SLOTS_STROOPER_ENGAGE ) )
				{
					//!!!KELLY - grunt cannot see the enemy and has just decided to 
					// charge the enemy's position. 
					if (FOkToSpeak())// && RANDOM_LONG(0,1))
					{
						//SENTENCEG_PlayRndSz( ENT(pev), "ST_CHARGE", STROOPER_SENTENCE_VOLUME, TROOPER_ATTN, 0, m_voicePitch);
						m_iSentence = STROOPER_SENT_CHARGE;
						//JustSpoke();
					}

					return GetScheduleOfType( SCHED_TROOPER_ESTABLISH_LINE_OF_FIRE );
				}
				else
				{
					//!!!KELLY - grunt is going to stay put for a couple seconds to see if
					// the enemy wanders back out into the open, or approaches the
					// grunt's covered position. Good place for a taunt, I guess?
					if (FOkToSpeak() && RANDOM_LONG(0,1))
					{
						SENTENCEG_PlayRndSz( ENT(pev), "ST_TAUNT", STROOPER_SENTENCE_VOLUME, TROOPER_ATTN, 0, m_voicePitch);
						JustSpoke();
					}
					return GetScheduleOfType( SCHED_STANDOFF );
				}
			}
			
			if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_TROOPER_ESTABLISH_LINE_OF_FIRE );
			}
		}
	}
	
	// no special cases here, call the base class
	return CSquadMonster :: GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* CSTrooper :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			if ( InSquad() )
			{
				if ( g_iSkillLevel == SKILL_HARD && HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_STROOPER_GRENADE ) )
				{
					if (FOkToSpeak())
					{
						SENTENCEG_PlayRndSz( ENT(pev), "ST_THROW", STROOPER_SENTENCE_VOLUME, TROOPER_ATTN, 0, m_voicePitch);
						JustSpoke();
					}
					return slTrooperTossGrenadeCover;
				}
				else
				{
					return &slTrooperTakeCover[ 0 ];
				}
			}
			else
			{
				if ( RANDOM_LONG(0,1) )
				{
					return &slTrooperTakeCover[ 0 ];
				}
				else
				{
					return &slTrooperGrenadeCover[ 0 ];
				}
			}
		}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		{
			return &slTrooperTakeCoverFromBestSound[ 0 ];
		}
	case SCHED_TROOPER_TAKECOVER_FAILED:
		{
			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && OccupySlot( bits_SLOTS_STROOPER_ENGAGE ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}

			return GetScheduleOfType ( SCHED_FAIL );
		}
		break;
	case SCHED_TROOPER_ELOF_FAIL:
		{
			// human grunt is unable to move to a position that allows him to attack the enemy.
			return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		break;
	case SCHED_TROOPER_ESTABLISH_LINE_OF_FIRE:
		{
			return &slTrooperEstablishLineOfFire[ 0 ];
		}
		break;
	case SCHED_RANGE_ATTACK1:
		{
			// randomly stand or crouch
			if (RANDOM_LONG(0,9) == 0)
				m_fStanding = RANDOM_LONG(0,1);
		 
			if (m_fStanding)
				return &slTrooperRangeAttack1B[ 0 ];
			else
				return &slTrooperRangeAttack1A[ 0 ];
		}
	case SCHED_COMBAT_FACE:
		{
			return &slTrooperCombatFace[ 0 ];
		}
	case SCHED_TROOPER_WAIT_FACE_ENEMY:
		{
			return &slTrooperWaitInCover[ 0 ];
		}
	case SCHED_TROOPER_SWEEP:
		{
			return &slTrooperSweep[ 0 ];
		}
	case SCHED_TROOPER_COVER_AND_RELOAD:
		{
			return &slTrooperHideReload[ 0 ];
		}
	case SCHED_TROOPER_FOUND_ENEMY:
		{
			return &slTrooperFoundEnemy[ 0 ];
		}
	case SCHED_VICTORY_DANCE:
		{
			if ( InSquad() )
			{
				if ( !IsLeader() )
				{
					return &slTrooperFail[ 0 ];
				}
			}

			return &slTrooperVictoryDance[ 0 ];
		}
	case SCHED_TROOPER_SUPPRESS:
		{
			if ( m_hEnemy->IsPlayer() && m_fFirstEncounter )
			{
				m_fFirstEncounter = FALSE;// after first encounter, leader won't issue handsigns anymore when he has a new enemy
				return &slTrooperSignalSuppress[ 0 ];
			}
			else
			{
				return &slTrooperSuppress[ 0 ];
			}
		}
	case SCHED_FAIL:
		{
			if ( m_hEnemy != NULL )
			{
				// grunt has an enemy, so pick a different default fail schedule most likely to help recover.
				return &slTrooperCombatFail[ 0 ];
			}

			return &slTrooperFail[ 0 ];
		}
	default:
		{
			return CSquadMonster :: GetScheduleOfType ( Type );
		}
	}
}

