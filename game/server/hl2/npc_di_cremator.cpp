#include	"cbase.h"
#include	"npc_di_cremator.h"
#include	"particle_parse.h"

//Courtesy of Dark Interval's lead developer
DEFINE_CUSTOM_AI;
};

	LINK_ENTITY_TO_CLASS( npc_di_cremator, CNPC_DI_Cremator );

int ACT_FIREINOUT;

	BEGIN_DATADESC( CNPC_DI_Cremator )

	DEFINE_FIELD( m_flNextIdleSoundTime, FIELD_TIME ),
	DEFINE_FIELD( m_flImmoRadius, FIELD_FLOAT ),
	DEFINE_FIELD( m_bHeadshot, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bCanisterShot, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flTimeLastUpdatedRadius, FIELD_TIME ),
//
	END_DATADESC()

void CNPC_DI_Cremator::Precache()
{
	BaseClass::Precache();
	PrecacheModel( "models/cremator_normal.mdl" ); // common low-poly model
	PrecacheModel( "models/cremator_tess.mdl" ); // a model with a bit of a tesselation (tesselated head and collar)
	PrecacheModel( "models/cremator_headprop.mdl" );
	enginesound->PrecacheSound("npc/cremator/amb_loop.wav");
	enginesound->PrecacheSound("npc/cremator/amb1.wav");
	enginesound->PrecacheSound("npc/cremator/amb2.wav");
	enginesound->PrecacheSound("npc/cremator/amb3.wav");
	enginesound->PrecacheSound("npc/cremator/crem_die.wav");
	enginesound->PrecacheSound("npc/cremator/alert_object.wav");
	enginesound->PrecacheSound("npc/cremator/alert_player.wav");
	PrecacheScriptSound( "NPC_DI_Cremator.FootstepLeft" );	
	PrecacheScriptSound( "NPC_DI_Cremator.FootstepRight" );
	PrecacheScriptSound( "DI_Cremator.Pain" );
	PrecacheScriptSound( "DI_Cremator.Alert" );
	PrecacheScriptSound( "DI_Cremator.Evil" );
	PrecacheScriptSound( "Weapon_Immolator.Single" );
	PrecacheScriptSound( "Weapon_Immolator.Stop" );
	UTIL_PrecacheOther( "env_immolatorspray" );
	PrecacheParticleSystem( "flamethrower" );
	engine->PrecacheModel("sprites/crystal_beam1.vmt"); // for the beam attack
}
void CNPC_DI_Cremator::Spawn()
{	
	Precache();
	//if( di_hipoly_character_models.GetBool() == 0) 
	SetModel( "models/cremator_tess.mdl" ); // common low-poly model
	//else
	//SetModel( "models/cremator_tess.mdl" ); // a model with a bit of a tesselation (tesselated head and collar)
	
	BaseClass::Spawn();

	SetHullType(HULL_WIDE_HUMAN); // that's a bit too big but the smaller ones are too small, need to implement the new one
	SetHullSizeNormal();
	
	SetBodygroup( 1, 0 ); // the gun
	SetBodygroup( 2, 0 ); // the head

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	m_bloodColor		= BLOOD_COLOR_RED;//BLOOD_COLOR_YELLOW;
	m_iHealth			= sk_di_cremator_health.GetFloat();
	m_flFieldOfView		= -0.707;// indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState			= NPC_STATE_NONE;
	
	m_flNextIdleSoundTime	= gpGlobals->curtime + random->RandomFloat( 14.0, 28.0 );

	m_MuzzleAttachment	= LookupAttachment( "muzzle" );
	m_HeadAttachment	= LookupAttachment( "1"		 );

	m_nSkin				= DI_CREMATOR_SKIN_ALERT; // original yellow-eyes skin
		
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_INNATE_RANGE_ATTACK1 ); // TODO: Melee?

	NPCInit();
}

void CNPC_DI_Cremator::AlertSound( void )
{
	EmitSound( "DI_Cremator.Alert" );

//	m_flNextIdleSoundTime += random->RandomFloat( 2.0, 4.0 );

}

void CNPC_DI_Cremator::PainSound( const CTakeDamageInfo &info )
{
	if ( random->RandomInt( 0, 2 ) == 0)
	{
		CPASAttenuationFilter filter( this );

		CSoundParameters params;
		if ( GetParametersForSound( "DI_Cremator.Pain", params, NULL ) )
		{
			EmitSound_t ep( params );
			params.pitch = m_iVoicePitch;

			EmitSound( filter, entindex(), ep );
		}
	}
}

Class_T	CNPC_DI_Cremator::Classify ( void )
{
	return	CLASS_TEAM4;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define CREM_ATTN_IDLE	(float)1.5

/*void CNPC_DI_Cremator::IdleSound( void ) // TODO: replace with modern script-based system
{
	CPASAttenuationFilter filter( this, CREM_ATTN_IDLE );
	switch ( random->RandomInt(0,2) )
	{
	case 0:	
		enginesound->EmitSound( filter, entindex(), CHAN_VOICE, "npc/cremator/amb1.wav", 1, CREM_ATTN_IDLE );	
		break;
	case 1:	
		enginesound->EmitSound( filter, entindex(), CHAN_VOICE, "npc/cremator/amb2.wav", 1, CREM_ATTN_IDLE );	
		break;
	case 2:	
		enginesound->EmitSound( filter, entindex(), CHAN_VOICE, "npc/cremator/amb3.wav", 1, CREM_ATTN_IDLE );	
		break;
	}
	enginesound->EmitSound( filter, entindex(), CHAN_VOICE, "npc/cremator/amb_loop.wav", 1, ATTN_NORM );

	m_flNextIdleSoundTime = gpGlobals->curtime + random->RandomFloat( 14.0, 28.0 );
}
*/

void CNPC_DI_Cremator::DeathSound( const CTakeDamageInfo &info )
{
	CPASAttenuationFilter filter( this );
	int iPitch = random->RandomInt( 90, 105 );
	enginesound->EmitSound( filter, entindex(), CHAN_VOICE, "npc/cremator/crem_die.wav", 1, ATTN_NORM, 0, iPitch );
}

/*void CNPC_DI_Cremator::immosound (void)
{
	CPASAttenuationFilter filter( this );
	int ipitch = random->RandomInt ( 80, 110);
	enginesound->emitsound( filter, entindex(), CHAN_WEAPON, "npc/cremator/plasma_shoot.wav", 1, ATTN_NORM, 0, iPitch );
}
*/
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
float CNPC_DI_Cremator::MaxYawSpeed( void )
{
	float flYS = 0;

	switch ( GetActivity() )
	{
	case	ACT_WALK_HURT:			flYS = 30;	break;
	case	ACT_RUN:			flYS = 90;	break;
	case	ACT_IDLE:			flYS = 90;	break;
	case	ACT_RANGE_ATTACK1:	flYS = 90;	break;
	default:
		flYS = 90;
		break;
	}

	return flYS;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
const char *CNPC_DI_Cremator::GetHeadpropModel( void )
{
	return "models/cremator_headprop.mdl";
}
void CNPC_DI_Cremator::Event_Killed( const CTakeDamageInfo &info  )
{	
	CTakeDamageInfo Info = info;

	if( (Info.GetAmmoType() == GetAmmoDef()->Index( ".50BMG" ))
		|| (Info.GetAmmoType() == GetAmmoDef()->Index( ".50BMG" ))
		|| (Info.GetAmmoType() == GetAmmoDef()->Index( "Buckshot" ))
		|| (Info.GetAmmoType() == GetAmmoDef()->Index( "Gauss" ))
		|| (Info.GetAmmoType() == GetAmmoDef()->Index( "XBowBolt" ))
		|| (Info.GetAmmoType() == GetAmmoDef()->Index( "357" ))
		&& ( m_bHeadshot == 1 ) )
	{
		
		SetBodygroup (2, 1); // turn the head off
		DropHead ( 65 );

	}

	m_nSkin = DI_CREMATOR_SKIN_DEAD; // turn the eyes black

	SetBodygroup (1, 1); // turn the gun off
	BaseClass::Event_Killed( info );
}
void CNPC_DI_Cremator::DropHead( int iVelocity ) 
{
	CPhysicsProp *pGib = assert_cast<CPhysicsProp*>(CreateEntityByName( "prop_physics" ));
		pGib->SetAbsOrigin( GetAbsOrigin() );
		pGib->SetAbsAngles( GetAbsAngles() );
		pGib->SetAbsVelocity( GetAbsVelocity() );
		pGib->SetModel( GetHeadpropModel() );
		pGib->Spawn();
		pGib->SetMoveType( MOVETYPE_VPHYSICS );

			Vector vecVelocity;
			pGib->GetMassCenter( &vecVelocity );
			vecVelocity -= WorldSpaceCenter();
			vecVelocity.z = fabs(vecVelocity.z);
			VectorNormalize( vecVelocity );

			float flRandomVel = random->RandomFloat( 35, 75 );
			vecVelocity *= (iVelocity * flRandomVel) / 15;
			vecVelocity.z += 100.0f;
			AngularImpulse angImpulse = RandomAngularImpulse( -500, 500 );
			
			IPhysicsObject *pObj = pGib->VPhysicsGetObject();
			if ( pObj != NULL )
			{
				pObj->AddVelocity( &vecVelocity, &angImpulse );
			}
			pGib->SetCollisionGroup( COLLISION_GROUP_INTERACTIVE );
}
int CNPC_DI_Cremator::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	CTakeDamageInfo newInfo = info;
	if( newInfo.GetDamageType() & DMG_BURN)
	{
		newInfo.ScaleDamage( 0 );
		DevMsg( "Fire damage; no actual damage is taken\n" );
	}	

	int nDamageTaken = BaseClass::OnTakeDamage_Alive( newInfo );

//	m_bHeadshot = false;
//	m_bCanisterShot = false;

	return nDamageTaken;
}
void CNPC_DI_Cremator::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{	
	CTakeDamageInfo newInfo = info;
	// the head and the abdomen sphere are the only vulnerable parts,
	// the legs, arms and stuff under the cloak is likely just some machinery,
	// and the C. is encountered in City after the lab and in Canals when
	// the player doesn't have weapon apart from crowbar, pistol and SMG
	// TODO: torn clothes with bullets, exposing underlying machinery
	if( (newInfo.GetDamageType() & DMG_BULLET || DMG_CLUB || DMG_BUCKSHOT || DMG_ENERGYBEAM ) ) 
	{	
		
		// the multipliers here are so small because of the innate 
		// headshot multiplayer which makes killing a cremator with headshots
		// ridiculously easy even on Hard. So all of them were divided by 6.
		if( ptr->hitgroup == HITGROUP_HEAD ) 
		{			
			m_bHeadshot = 1;

			if( g_pGameRules->IsSkillLevel(SKILL_EASY) )
			{
				if( newInfo.GetAmmoType() == GetAmmoDef()->Index(".50BMG") ) // base damage 100
				{
					newInfo.ScaleDamage( 0.5 );
					DevMsg( "Sniper, 1.5 HEAD\n" );
				}
				else if( newInfo.GetAmmoType() == GetAmmoDef()->Index("357") ) // base damage 40
				{
					newInfo.ScaleDamage( 0.3 );
					DevMsg( "357, 2.0 HEAD\n" );
				}
				else if( newInfo.GetAmmoType() == GetAmmoDef()->Index("9x19") )
				{
					newInfo.ScaleDamage( 0.142 );
					DevMsg( "9 mm, 0.85 HEAD\n" ); // Pistol, SMG ammo
				}
				else if( newInfo.GetAmmoType() == GetAmmoDef()->Index(".556x45") )
				{
					newInfo.ScaleDamage( 0.192 );
					DevMsg( "5.56, 1.15 HEAD\n" ); // OICW ammo
				}
				else if( newInfo.GetAmmoType() == GetAmmoDef()->Index("Buckshot") )
				{
					newInfo.ScaleDamage( 0.192 );
					DevMsg( "Buchshot, 1.15 HEAD\n" );
				}
				else if( newInfo.GetAmmoType() == GetAmmoDef()->Index("Uranium") )
				{
					newInfo.ScaleDamage( 0.192 );
					DevMsg( "Gauss, 1.15 HEAD\n" );
				}
			}
			else if( g_pGameRules->IsSkillLevel(SKILL_HARD) )
			{
				if( newInfo.GetAmmoType() == GetAmmoDef()->Index(".50BMG") ) // base damage 100
				{
					newInfo.ScaleDamage( 0.384 );
					DevMsg( "Sniper, 1.15 HEAD\n" );
				}
				else if( newInfo.GetAmmoType() == GetAmmoDef()->Index("357") ) // base damage 40
				{
					newInfo.ScaleDamage( 0.292 );
					DevMsg( "357, 1.75 HEAD\n" );
				}
				else if( newInfo.GetAmmoType() == GetAmmoDef()->Index("9x19") )
				{
					newInfo.ScaleDamage( 0.1 );
					DevMsg( "9 mm, 0.6 HEAD\n" );
				}
				else if( newInfo.GetAmmoType() == GetAmmoDef()->Index(".556x45") )
				{
					newInfo.ScaleDamage( 0.146 );
					DevMsg( "5.56, 0.875 HEAD\n" );
				}
				else if( newInfo.GetAmmoType() == GetAmmoDef()->Index("Buckshot") )
				{
					newInfo.ScaleDamage( 0.146 );
					DevMsg( "Buckshot, 0.875 HEAD\n" );
				}
				else if( newInfo.GetAmmoType() == GetAmmoDef()->Index("Uranium") )
				{
					newInfo.ScaleDamage( 0.146 );
					DevMsg( "Gauss, 0.875 HEAD\n" );
				}
			}			
			else
			{
				if( newInfo.GetAmmoType() == GetAmmoDef()->Index(".50BMG") )
				{
					newInfo.ScaleDamage( 0.413  );
					DevMsg( "Sniper, 1.25 HEAD\n" );
				}
				else if( newInfo.GetAmmoType() == GetAmmoDef()->Index("357") )
				{
					newInfo.ScaleDamage( 0.25  );
					DevMsg( "357, 1.5 HEAD\n" );
				}
				else if( newInfo.GetAmmoType() == GetAmmoDef()->Index("9x19") )
				{
					newInfo.ScaleDamage( 0.12  );
					DevMsg( "9mm, 0.7 HEAD\n" );
				}
				else if( newInfo.GetAmmoType() == GetAmmoDef()->Index(".556x45") )
				{
					newInfo.ScaleDamage( 0.16 );
					DevMsg( "5.56, 1.0 HEAD\n" );
				}
				else if( newInfo.GetAmmoType() == GetAmmoDef()->Index("Buckshot") )
				{
					newInfo.ScaleDamage( 0.16 );
					DevMsg( "Buckshot, 1.0 HEAD\n" );
				}
				else if( newInfo.GetAmmoType() == GetAmmoDef()->Index("Uranium") )
				{
					newInfo.ScaleDamage( 0.16 );
					DevMsg( "Gauss, 1.0 HEAD\n" );
				}
			}
		}
		else if( ptr->hitgroup == DI_CREMATOR_CANISTER )
		{
			m_bCanisterShot = 1;

			if( g_pGameRules->IsSkillLevel(SKILL_EASY) )
			{
				if( newInfo.GetAmmoType() == GetAmmoDef()->Index(".50BMG") )
				{
					newInfo.ScaleDamage( 1.5 );
					DevMsg( "Sniper, 1.5 CANISTER\n" );
				}
				else if( newInfo.GetAmmoType() == GetAmmoDef()->Index("357") )
				{
					newInfo.ScaleDamage( 2.0 );
					DevMsg( "357, 2.0 CANISTER\n" );
				}
				else if( newInfo.GetAmmoType() == GetAmmoDef()->Index("9x19") )
				{
					newInfo.ScaleDamage( 0.85 );
					DevMsg( "9 mm, 0.85 CANISTER\n" );
				}
				else if( newInfo.GetAmmoType() == GetAmmoDef()->Index(".556x45") )
				{
					newInfo.ScaleDamage( 1.15 );
					DevMsg( "5.56, 1.15 CANISTER\n" );
				}
				else if( newInfo.GetAmmoType() == GetAmmoDef()->Index("Buckshot") )
				{
					newInfo.ScaleDamage( 1.15 );
					DevMsg( "Buckshot, 1.15 CANISTER\n" );
				}
				else if( newInfo.GetAmmoType() == GetAmmoDef()->Index("Uranium") )
				{
					newInfo.ScaleDamage( 1.15 );
					DevMsg( "Gauss, 1.15 CANISTER\n" );
				}
			}
			else if( g_pGameRules->IsSkillLevel(SKILL_HARD) )
			{
				if( newInfo.GetAmmoType() == GetAmmoDef()->Index(".50BMG") )
				{
					newInfo.ScaleDamage( 1.15 );
					DevMsg( "Sniper, 1.15 CANISTER\n" );
				}
				else if( newInfo.GetAmmoType() == GetAmmoDef()->Index("357") )
				{
					newInfo.ScaleDamage( 1.75 );
					DevMsg( "357, 1.75 CANISTER\n" );
				}
				else if( newInfo.GetAmmoType() == GetAmmoDef()->Index("9x19") )
				{
					newInfo.ScaleDamage( 0.6 );
					DevMsg( "9 mm, 0.6 CANISTER\n" );
				}
				else if( newInfo.GetAmmoType() == GetAmmoDef()->Index(".556x45") )
				{
					newInfo.ScaleDamage( 0.875 );
					DevMsg( "5.56, 0.875 CANISTER\n" );
				}
				else if( newInfo.GetAmmoType() == GetAmmoDef()->Index("Buckshot") )
				{
					newInfo.ScaleDamage( 0.875 );
					DevMsg( "Buckshot, 0.875 CANISTER\n" );
				}
				else if( newInfo.GetAmmoType() == GetAmmoDef()->Index("Uranium") )
				{
					newInfo.ScaleDamage( 0.875 );
					DevMsg( "Gauss, 0.875 CANISTER\n" );
				}
			}			
			else
			{
				if( newInfo.GetAmmoType() == GetAmmoDef()->Index(".50BMG") )
				{
					newInfo.ScaleDamage( 1.25  );
					DevMsg( "Sniper, 1.25 CANISTER\n" );
				}
				else if( newInfo.GetAmmoType() == GetAmmoDef()->Index("357") )
				{
					newInfo.ScaleDamage( 1.5  );
					DevMsg( "357, 1.5 CANISTER\n" );
				}
				else if( newInfo.GetAmmoType() == GetAmmoDef()->Index("9x19") )
				{
					newInfo.ScaleDamage( 0.7  );
					DevMsg( "9mm, 0.7 CANISTER\n" );
				}
				else if( newInfo.GetAmmoType() == GetAmmoDef()->Index(".556x45") )
				{
					newInfo.ScaleDamage( 1.0 );
					DevMsg( "5.56, 1.0 CANISTER\n" );
				}
				else if( newInfo.GetAmmoType() == GetAmmoDef()->Index("Buckshot") )
				{
					newInfo.ScaleDamage( 1.0 );
					DevMsg( "Buckshot, 1.0 CANISTER\n" );
				}
				else if( newInfo.GetAmmoType() == GetAmmoDef()->Index("Uranium") )
				{
					newInfo.ScaleDamage( 1.0 );
					DevMsg( "Gauss, 1.0 CANISTER\n" );
				}
			}
		}
		// however, if the sprayer is damaged, something unpleasant may happen
		else if (ptr->hitgroup == DI_CREMATOR_GUN )
		{
		// well, right now I can't think of any specific effect
			DevMsg( "DI_Cremator: gear is damaged\n");
		}

		else
		{
			newInfo.ScaleDamage( 0.25 );
			DevMsg( "Generic 0.25 Non-specified\n" );
		}
	}

	BaseClass::TraceAttack( newInfo, vecDir, ptr, 0 );
}
void CNPC_DI_Cremator::Ignite( float flFlameLifetime, bool bNPCOnly, float flSize, bool bCalledByLevelDesigner )
{
	BaseClass::Ignite( DI_CREMATOR_BURN_TIME, bNPCOnly, flSize, bCalledByLevelDesigner );
	//if( IsOnFire() && di_dynamic_lighting_npconfire.GetBool() == 1 )
	// NOT NEEDED ANYMORE SINCE ANY BURNING NPC WILL 
	// EMIT EF_DIMLIGHT
	//{
	//	GetEffectEntity()->AddEffects( EF_DIMLIGHT );
	//}
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_DI_Cremator::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_DI_CREMATOR_IDLE:
		SetActivity( ACT_IDLE );
		TaskComplete();
		break;
	case TASK_DI_CREMATOR_RANGE_ATTACK1:
		{
			Vector flEnemyLKP = GetEnemyLKP();
			GetMotor()->SetIdealYawToTarget( flEnemyLKP );
		}
		return;
		default:
		BaseClass::StartTask( pTask );
		break;
	}
}
void CNPC_DI_Cremator::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_DI_CREMATOR_RANGE_ATTACK1:
		{
			Vector flEnemyLKP = GetEnemyLKP();
			GetMotor()->SetIdealYawToTargetAndUpdate( flEnemyLKP );
			if ( IsActivityFinished() || IsActivityMovementPhased( ACT_WALK ) || IsActivityMovementPhased( ACT_RUN ) )
			{				
				StopParticleEffects( this );
				TaskComplete();
				return;
			}
		}
		return;
	}
	BaseClass::RunTask( pTask );
}
void CNPC_DI_Cremator::OnChangeActivity( Activity eNewActivity ) // implemented so that a cremator chasing moving target won't have his immolator emitting the fre
{
	if (eNewActivity == ACT_WALK || ACT_RUN)
	{
		StopParticleEffects( this );
		StopSound( "Weapon_Immolator.Single" );
		EmitSound( "Weapon_Immolator.Stop" );
	}
}
void CNPC_DI_Cremator::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
		case NPC_EVENT_LEFTFOOT:
			{
				EmitSound( "NPC_DI_Cremator.FootstepLeft", pEvent->eventtime );
			}
			break;
		case NPC_EVENT_RIGHTFOOT:
			{
				EmitSound( "NPC_DI_Cremator.FootstepRight", pEvent->eventtime );
			}
			break;			
		case DI_CREMATOR_AE_IMMO_START:
			{
				DispatchSpray();
				Vector flEnemyLKP = GetEnemyLKP();
				GetMotor()->SetIdealYawToTargetAndUpdate( flEnemyLKP );
			}
			break;
		case DI_CREMATOR_AE_IMMO_PARTICLE:
			{				
				DispatchParticleEffect( "flamethrower", PATTACH_POINT_FOLLOW, this, "muzzle" );
				EmitSound( "Weapon_Immolator.Single" );
			}
			break;
		case DI_CREMATOR_AE_IMMO_PARTICLEOFF:
			{				
				StopParticleEffects( this );
				StopSound( "Weapon_Immolator.Single" );
				EmitSound( "Weapon_Immolator.Stop" );
			}
			break;
		default:
			BaseClass::HandleAnimEvent( pEvent );
			break;
	}
}
void CNPC_DI_Cremator::DispatchSpray()
{ 
	Vector vecSrc, vecAim;
	trace_t tr;
	CBaseEntity *pEntity;

	Vector forward, right, up;
	AngleVectors( GetAbsAngles(), &forward, &right, &up );

	vecSrc = GetAbsOrigin() + up * 36;
	vecAim = GetShootEnemyDir( vecSrc );
	float deflection = 0.01;	
	vecAim = vecAim + 1 * right * random->RandomFloat( 0, deflection ) + up * random->RandomFloat( -deflection, deflection );
	UTIL_TraceLine ( vecSrc, vecSrc + vecAim * 512, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr);
			
	if ( tr.DidHitWorld() ) // spawn flames on solid surfaces. 
							//Is not very important since it is extremely rarely for a cremator to
							// hit brush geometry but might be a nice feature for a close-space combat
							// it also works fine but again is EXTREMELY hard to get in-game
	{
		Vector	ofsDir = ( tr.endpos - GetAbsOrigin() );
		float	offset = VectorNormalize( ofsDir );

		if ( offset > 128 )
			offset = 128;

		float scale	 = 0.1f + ( 0.75f * ( 1.0f - ( offset / 128.0f ) ) );
		float growth = 0.1f + ( 0.75f * (offset / 128.0f ) );

		if ( tr.surface.flags & CONTENTS_GRATE ) // get smaller flames on grates since they have a smaller burning area
		{
			scale = 0.1f + ( 0.15f * ( 1.0f - ( offset / 128.0f ) ) );
		}
		else
		{
			scale = 0.1f + ( 0.75f * ( 1.0f - ( offset / 128.0f ) ) );
		}
		FireSystem_StartFire( tr.endpos, scale, growth, 10.0f, (SF_FIRE_START_ON|SF_FIRE_START_FULL|SF_FIRE_SMOKELESS), (CBaseEntity*) this, FIRE_NATURAL );
	}	

	pEntity = tr.m_pEnt;

	if ( pEntity != NULL && m_takedamage )
	{
		CTakeDamageInfo firedamage( this, this, sk_di_cremator_firedamage.GetFloat(), DMG_BURN );
		CTakeDamageInfo radiusdamage( this, this, sk_di_cremator_radiusdamage.GetFloat(), DMG_BURN ); //DMG_PLASMA by default
		CalculateMeleeDamageForce( &firedamage, vecAim, tr.endpos );
		RadiusDamage ( CTakeDamageInfo ( this, this, 2, DMG_BURN ), // AOE; this stuff makes cremators absurdly powerfull sometimes btw DMG_PLASMA BY DEFAULT
			tr.endpos,
			64.0f,
			CLASS_NONE,
			NULL );

		pEntity->DispatchTraceAttack( ( firedamage ), vecAim, &tr );

		if( pEntity->IsPlayer() )
		{
			color32 flamescreen = {0,255,30,26};
			UTIL_ScreenFade( pEntity, flamescreen, 0.5f, 0.1f, FFADE_IN ); // add a greenish color to the screen when hurting a player
		}
//-----------------------------------------------------------------------------
// If the C. has ignited a zombie/headcrab, the C. should turn its attention
// on other zombies that have not been ignited since the burning zombie
// will die anyway and there's no point in wasting time killing it
// while other zombies get a chance to come closer and attack
//-----------------------------------------------------------------------------
// have to move it to DispatchSpray() since the ignition is no longer controlled in cremator code
//		else if ( pEntity->GetClassname() == "npc_zombie" )
//		{
//			AddEntityRelationship( pEntity, IRelationType(pEntity), (IRelationPriority(pEntity) - 90) );
//		}
	}	
}
void CNPC_DI_Cremator::SelectSkin( void ) // doesn't quite work, it can turn eyes red ("combat" skin) when combating an enemy, but will not return to calm/alert skin when an enemy is dead
{
/*	if(m_NPCState == NPC_STATE_COMBAT)
	{
		m_nSkin = 3;
		if ( HasCondition( COND_ENEMY_DEAD ) )
		{
			m_nSkin = 0;
		}
		if ( GetEnemy() == NULL )
		{
			m_nSkin = 1;
		}
	}
	if(m_NPCState == NPC_STATE_ALERT)
	{
		m_nSkin = 3;
		if ( GetEnemy() == NULL )
		{
			m_nSkin = 1;
		}
	}
	if(m_NPCState == NPC_STATE_NONE)
	{
		m_nSkin = 1;
	}*/
}

NPC_STATE CNPC_DI_Cremator::SelectIdealState( void )
{
	switch( m_NPCState )
	{
	case NPC_STATE_COMBAT:
		{
			if ( GetEnemy() == NULL )
			{
				if ( !HasCondition( COND_ENEMY_DEAD ) )
				{
					SetCondition( COND_ENEMY_DEAD ); // TODO: patrolling

				}
				return NPC_STATE_ALERT;
			}
			else if ( HasCondition( COND_ENEMY_DEAD ) )
			{
				//AnnounceEnemyKill(GetEnemy());
				EmitSound( "DI_Cremator.Evil" );
			//	m_flNextIdleSoundTime += random->RandomFloat( 2.0, 4.0 );
			}
		}
	default:
		{
			return BaseClass::SelectIdealState();
		}
	}

	return GetIdealState();
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
Activity CNPC_DI_Cremator::NPC_TranslateActivity( Activity baseAct )
{
	if ( baseAct == ACT_WALK || ACT_RUN )
	{
		if ( m_iHealth < ( sk_di_cremator_health.GetBool() * 0.5 ) )
		{
			//return (Activity)ACT_WALK_HURT; // the animaton is broken, don't use it
		}
	}
	//return BaseClass::NPC_TranslateActivity( baseAct );
	return baseAct;
}
int CNPC_DI_Cremator::RangeAttack1Conditions( float flDot, float flDist )
{
	if ( flDist > DI_CREMATOR_MAX_RANGE )
		return COND_TOO_FAR_TO_ATTACK;	
	if ( flDist < DI_CREMATOR_MIN_RANGE )
		return COND_TOO_CLOSE_TO_ATTACK;
	if ( flDot < DI_CREMATOR_RANGE1_CONE )
		return COND_NOT_FACING_ATTACK;
	return COND_CAN_RANGE_ATTACK1;
}
int CNPC_DI_Cremator::SelectSchedule( void )
{
	if( HasCondition ( COND_CAN_RANGE_ATTACK1 ) )
	{
		return SCHED_DI_CREMATOR_RANGE_ATTACK1;
	}
	if( HasCondition( COND_SEE_ENEMY ) )
	{
		if( HasCondition( COND_TOO_FAR_TO_ATTACK ) )
		{
			return SCHED_CHASE_ENEMY;
			StopParticleEffects( this );
		}
		return SCHED_DI_CREMATOR_RANGE_ATTACK1;
	}
	return BaseClass::SelectSchedule();
}
int CNPC_DI_Cremator::TranslateSchedule( int type ) 
{
	switch( type )
	{
	case SCHED_RANGE_ATTACK1:
		return SCHED_DI_CREMATOR_RANGE_ATTACK1;
		break;
	case SCHED_CHASE_ENEMY:
		return SCHED_DI_CREMATOR_CHASE;
		break;
	}
	return BaseClass::TranslateSchedule( type );
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_DI_Cremator::GetSoundInterests( void )
{
	return	SOUND_WORLD	|
			SOUND_COMBAT	|
			SOUND_BULLET_IMPACT |
		    SOUND_CARCASS	|
			SOUND_MEAT		|
			SOUND_GARBAGE	|
			SOUND_PLAYER	|
			SOUND_MOVE_AWAY;
}

AI_BEGIN_CUSTOM_NPC( npc_cremator, CNPC_DI_Cremator )

	DECLARE_TASK( TASK_DI_CREMATOR_RANGE_ATTACK1 )
	DECLARE_TASK( TASK_DI_CREMATOR_IDLE )
	DECLARE_ACTIVITY( ACT_FIREINOUT )

	DEFINE_SCHEDULE
	(
		SCHED_DI_CREMATOR_RANGE_ATTACK1,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_FACE_ENEMY				0"
		"		TASK_ANNOUNCE_ATTACK		1"
		//"		TASK_DI_CREMATOR_RANGE_ATTACK1	0"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_RANGE_ATTACK1"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_NO_PRIMARY_AMMO"
		"		COND_TOO_FAR_TO_ATTACK"
	)
	DEFINE_SCHEDULE
	(
		SCHED_DI_CREMATOR_CHASE,

		"	Tasks"
		"		TASK_SET_TOLERANCE_DISTANCE		320"
		"		 TASK_GET_CHASE_PATH_TO_ENEMY	1000"
		"		 TASK_RUN_PATH					0"
		"		 TASK_WAIT_FOR_MOVEMENT			0"
		"		 TASK_FACE_ENEMY				0"
		"	"
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
		"		COND_CAN_RANGE_ATTACK1"
	)
	DEFINE_SCHEDULE
	(
		SCHED_DI_CREMATOR_IDLE,

		"	Tasks"
		//"		TASK_DI_CREMATOR_RANGE_ATTACK1	0"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_IDLE"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
	)
	
AI_END_CUSTOM_NPC()