//******************************************\\
//			CrazyArts 2009					\\
//	Made only by CrazyArts in 2009 year		\\
//		http://crazyarts.do.am/				\\
//	Code is free for compiling and editing	\\
//******************************************\\
/////////////////////////////////////////////////////////////
#include	"cbase.h"
#include	"ai_default.h"
#include	"ai_task.h"
#include	"ai_schedule.h"
#include	"ai_node.h"
#include	"ai_hull.h"
#include	"ai_hint.h"
#include	"ai_memory.h"
#include	"ai_route.h"
#include	"ai_motor.h"
#include	"hl1_npc_barney.h"
#include	"soundent.h"
#include	"game.h"
#include	"npcevent.h"
#include	"entitylist.h"
#include	"activitylist.h"
#include	"animation.h"
#include	"basecombatweapon.h"
#include	"IEffects.h"
#include	"vstdlib/random.h"
#include	"engine/IEngineSound.h"
#include	"ammodef.h"
#include	"ai_behavior_follow.h"
#include	"AI_Criteria.h"
#include	"SoundEmitterSystem/isoundemittersystembase.h"

#include 	<time.h>

#include "effect_dispatch_data.h"
#include "te_effect_dispatch.h"
////////////////////////////////////////////////////////////
#define		HAS_AE_SHOOT (3)
////////////////////////////////////////////////////////////
class CAssault: public CHL1BaseNPC
{
		DECLARE_CLASS( CAssault, CHL1NPCTalker );
public:
	void Spawn( void );
	void Precache( void );	
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void SetYawSpeed( void );
	void PainSound( void );
	void DeathSound( void );
	void ShootMG( void );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	int Classify( void );
	int GetVoicePitch( void );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );

	float	m_checkAttackTime;
	BOOL	m_lastAttackCheck;
};
/////////////////////////////////////////////////////////////
LINK_ENTITY_TO_CLASS( monster_hassault, CAssault );
/////////////////////////////////////////////////////////////


void CAssault :: Spawn()
{
	Precache();
	//So, setting entity vars
	SET_MODEL(ENT(pev), "models/hassault.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->health			= gSkillData.hgruntHealth;
	pev->view_ofs		= Vector ( 0, 0, 50 );
	m_flFieldOfView		= VIEW_FIELD_WIDE; 
	m_MonsterState		= MONSTERSTATE_NONE;
	pev->body			= 0; 
	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;
	MonsterInit();
}

void CAssault :: Precache()
{
//Monster precache
	PRECACHE_MODEL("models/hassault.mdl");
	PRECACHE_SOUND("hass/ha_pain.wav");
	PRECACHE_SOUND("hass/ha_die.wav");
	PRECACHE_SOUND("hass/ha_idle.wav");
	PRECACHE_SOUND("hass/minigun_shoot.wav");
//Base precache
	CBaseMonster :: Precache();
}

void CAssault::HandleAnimEvent( animevent_t *pEvent )
{
	switch(pEvent->event)
	{
	case HAS_AE_SHOOT:
		ShootMG();ShootMG();ShootMG();
		break;
	default:
		CBaseMonster::HandleAnimEvent(pEvent);
	}
}

void CAssault :: SetYawSpeed()
{
	int ys;
	ys = 0;
	switch ( m_Activity )
	{
	case ACT_IDLE:		
		ys = 65;
		break;
	case ACT_WALK:
		ys = 50;
		break;
	case ACT_RUN:
		ys = 52;
		break;
	default:
		ys = 50;
		break;
	}
	pev->yaw_speed = ys;
}

void CAssault :: PainSound()
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "hass/ha_pain.wav", 1, ATTN_NORM, 0, GetVoicePitch()); 
}

void CAssault :: DeathSound()
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "hass/ha_die.wav", 1, ATTN_NORM, 0, GetVoicePitch()); 
}

void CAssault :: ShootMG()
{
	Vector vecShootOrigin;
	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = pev->origin + Vector( 0, 0, 55 );
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );
	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
	pev->effects = EF_MUZZLEFLASH;
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 1024, BULLET_PLAYER_9MM );
	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "hass/minigun_shoot.wav", 1, ATTN_NORM, 0, 100 );
}

void CAssault::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	switch( ptr->iHitgroup)
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_BLAST))
		{
			flDamage = flDamage / 1.5;
		}
		break;
	case 10:
		if (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_CLUB))
		{
			flDamage -= 12;
			if (flDamage <= 0)
			{
				UTIL_Ricochet( ptr->vecEndPos, 1.0 );
				flDamage = 0.01;
			}
		}
		ptr->iHitgroup = HITGROUP_HEAD;
		break;
	}
	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

int CAssault :: GetVoicePitch()
{
	return 102;
}

int CAssault :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
/*	pev->health -= flDamage;
	if (flGamage<pev->health) 
	{PainSound();} 
	else 
	{
		DeathSound();
		Killed();
	}*/
return CBaseMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

int CAssault :: Classify()
{
	return CLASS_HUMAN_MILITARY;
}

BOOL CAssault :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( flDist <= 1024 && flDot >= 0.5 )
	{
		if ( gpGlobals->time > m_checkAttackTime )
		{
			TraceResult tr;
			
			Vector shootOrigin = pev->origin + Vector( 0, 0, 55 );
			CBaseEntity *pEnemy = m_hEnemy;
			Vector shootTarget = ( (pEnemy->BodyTarget( shootOrigin ) - pEnemy->pev->origin) + m_vecEnemyLKP );
			UTIL_TraceLine( shootOrigin, shootTarget, dont_ignore_monsters, ENT(pev), &tr );
			m_checkAttackTime = gpGlobals->time + 0.03;
			if ( tr.flFraction == 1.0 || (tr.pHit != NULL && CBaseEntity::Instance(tr.pHit) == pEnemy) )
				m_lastAttackCheck = TRUE;
			else
				m_lastAttackCheck = FALSE;
			m_checkAttackTime = gpGlobals->time + 0.01;
		}
		return m_lastAttackCheck;
	}
	return FALSE;
}

