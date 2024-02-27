//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "iviewrender.h"
#include "clienteffectprecachesystem.h"
#include "studio.h"
#include "bone_setup.h"
#include "engine/ivmodelinfo.h"
#include "c_fire_smoke.h"
#include "engine/IEngineSound.h"
#include "iefx.h"
#include "dlight.h"
#include "tier0/icommandline.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


CLIENTEFFECT_REGISTER_BEGIN( SmokeStackMaterials )
	CLIENTEFFECT_MATERIAL( "particle/SmokeStack" )
	CLIENTEFFECT_MATERIAL("particle/fire")
	CLIENTEFFECT_MATERIAL("sprites/flamelet1")
	CLIENTEFFECT_MATERIAL("sprites/flamelet2")
	CLIENTEFFECT_MATERIAL("sprites/flamelet3")
	CLIENTEFFECT_MATERIAL("sprites/flamelet4")
	CLIENTEFFECT_MATERIAL("sprites/flamelet5")
	CLIENTEFFECT_MATERIAL("sprites/fire1")
	CLIENTEFFECT_MATERIAL("sprites/plasma1")
CLIENTEFFECT_REGISTER_END()

float g_flGlowObstructionDecayPerSecond = .2;
//
// Used for sorting hitboxes by volume.
//
struct HitboxVolume_t
{
	int nIndex;			// The index of the hitbox in the model.
	float flVolume;		// The volume of the hitbox in cubic inches.
};

//-----------------------------------------------------------------------------
// Purpose: Callback function to sort hitboxes by decreasing volume.
//			To mix up the sort results a little we pick a random result for
//			boxes within 50 cubic inches of another.
//-----------------------------------------------------------------------------
int __cdecl SortHitboxVolumes(HitboxVolume_t* elem1, HitboxVolume_t* elem2)
{
	if (elem1->flVolume > elem2->flVolume + 50)
	{
		return -1;
	}

	if (elem1->flVolume < elem2->flVolume + 50)
	{
		return 1;
	}

	if (elem1->flVolume != elem2->flVolume)
	{
		return random->RandomInt(-1, 1);
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the volume of the given box in cubic inches.
//-----------------------------------------------------------------------------
inline float CalcBoxVolume(const Vector& mins, const Vector& maxs)
{
	return (maxs.x - mins.x) * (maxs.y - mins.y) * (maxs.z - mins.z);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pRecvProp - 
//			*pStruct - 
//			*pVarData - 
//			*pIn - 
//			objectID - 
//-----------------------------------------------------------------------------
void RecvProxy_Scale( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_FireSmoke	*pFireSmoke	= (C_FireSmoke	*) pStruct;
	float scale				= pData->m_Value.m_Float;

	//If changed, update our internal information
	if ( ( pFireSmoke->m_flScale != scale ) && ( pFireSmoke->m_flScaleEnd != scale ) )
	{
		pFireSmoke->m_flScaleStart		= pFireSmoke->m_flScaleRegister;
		pFireSmoke->m_flScaleEnd		= scale;			
	}

	pFireSmoke->m_flScale = scale;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pRecvProp - 
//			*pStruct - 
//			*pVarData - 
//			*pIn - 
//			objectID - 
//-----------------------------------------------------------------------------
void RecvProxy_ScaleTime( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_FireSmoke	*pFireSmoke	= (C_FireSmoke	*) pStruct;
	float time				= pData->m_Value.m_Float;

	//If changed, update our internal information
	//if ( pFireSmoke->m_flScaleTime != time )
	{
		if ( time == -1.0f )
		{
			pFireSmoke->m_flScaleTimeStart	= Helper_GetTime()-1.0f;
			pFireSmoke->m_flScaleTimeEnd	= pFireSmoke->m_flScaleTimeStart;
		}
		else
		{
			pFireSmoke->m_flScaleTimeStart	= Helper_GetTime();
			pFireSmoke->m_flScaleTimeEnd	= Helper_GetTime() + time;
		}
	}

	pFireSmoke->m_flScaleTime = time;
}

//Receive datatable
IMPLEMENT_CLIENTCLASS_DT( C_FireSmoke, DT_FireSmoke, CFireSmoke )
	RecvPropFloat( RECVINFO( m_flStartScale )),
	RecvPropFloat( RECVINFO( m_flScale ), 0, RecvProxy_Scale ),
	RecvPropFloat( RECVINFO( m_flScaleTime ), 0, RecvProxy_ScaleTime ),
	RecvPropInt( RECVINFO( m_nFlags ) ),
	RecvPropInt( RECVINFO( m_nFlameModelIndex ) ),
	RecvPropInt( RECVINFO( m_nFlameFromAboveModelIndex ) ),
END_RECV_TABLE()

ConVar manod_fire_new("manod_fire_new","0",FCVAR_REPLICATED | FCVAR_ARCHIVE);

//==================================================
// C_FireSmoke
//==================================================

C_FireSmoke::C_FireSmoke()
{
	if (manod_fire_new.GetInt() == FIRE_2003)
	{
		//Server-side
		m_flStartScale = 0.0f;
		m_flScale = 0.0f;
		m_flScaleTime = 0.0f;
		m_nFlags = bitsFIRESMOKE_NONE;
		m_nFlameModelIndex = 0;
		m_nFlameFromAboveModelIndex = 0;

		//Client-side
		m_flScaleRegister = 0.0f;
		m_flScaleStart = 0.0f;
		m_flScaleEnd = 0.0f;
		m_flScaleTimeStart = 0.0f;
		m_flScaleTimeEnd = 0.0f;
		m_bClipTested = false;

		m_flChildFlameSpread = FLAME_CHILD_SPREAD;

		//m_pEmitter = NULL;

		//Clear all child flames
		for (int i = 0; i < NUM_CHILD_FLAMES; i++)
		{
			m_entFlames[i].Clear();
			m_entFlamesFromAbove[i].Clear();
		}

		m_pEmberEmitter = NULL;
		m_pSmokeEmitter = NULL;
	}
	m_eFireStyle = (FireStyles_e)manod_fire_new.GetInt();
}

C_FireSmoke::~C_FireSmoke()
{

	// Shut down our effect if we have it
	if ( m_hEffect )
	{
		m_hEffect->StopEmission(false, false , true);
		m_hEffect = NULL;
	}

	if (m_pFireOverlay != NULL)
	{
		delete m_pFireOverlay;
		m_pFireOverlay = NULL;
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_FireSmoke::Simulate( void )
{
	if (m_eFireStyle != FIRE_ORANGEBOX)
	{
		if (ShouldDraw() == false)
		{
			for (int i = 0; i < NUM_CHILD_FLAMES; i++)
			{
				m_entFlames[i].SetRenderColor(0, 0, 0, 0);
				m_entFlames[i].SetBrightness(0);
			}

			if (m_nFlags & bitsFIRESMOKE_VISIBLE_FROM_ABOVE)
			{
				for (int i = 0; i < NUM_CHILD_FLAMES; i++)
				{
					m_entFlamesFromAbove[i].SetRenderColor(0, 0, 0, 0);
					m_entFlamesFromAbove[i].SetBrightness(0);
				}
			}

			m_nFlags &= ~bitsFIRESMOKE_SMOKE;
		}

		//Only do this if we're active
		if ((m_nFlags & bitsFIRESMOKE_ACTIVE) == false)
			return;

		Update();
		AddFlames();
	}
}

#define	FLAME_ALPHA_START	0.9f
#define FLAME_ALPHA_END		1.0f

#define	FLAME_TRANS_START	0.75f

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_FireSmoke::AddFlames( void )
{
	float alpha = 1.0f;

	//Update the child flame alpha
	for (int i = 0; i < NUM_CHILD_FLAMES; i++)
	{
		if (m_entFlames[i].GetScale() > 1e-3f)
		{
			m_entFlames[i].SetRenderColor((255.0f * alpha), (255.0f * alpha), (255.0f * alpha));
			m_entFlames[i].SetBrightness(255.0f * alpha);

			Assert(m_entFlames[i].GetRenderHandle() != INVALID_CLIENT_RENDER_HANDLE);
			m_entFlames[i].AddToLeafSystem();
		}
	}

	if (m_nFlags & bitsFIRESMOKE_VISIBLE_FROM_ABOVE)
	{
		for (int i = 0; i < NUM_CHILD_FLAMES; i++)
		{
			if (m_entFlamesFromAbove[i].GetScale() > 1e-3f)
			{
				m_entFlamesFromAbove[i].SetRenderColor((255.0f * alpha), (255.0f * alpha), (255.0f * alpha));
				m_entFlamesFromAbove[i].SetBrightness(255.0f * alpha);

				Assert(m_entFlamesFromAbove[i].GetRenderHandle() != INVALID_CLIENT_RENDER_HANDLE);
				m_entFlamesFromAbove[i].AddToLeafSystem();
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bnewentity - 
//-----------------------------------------------------------------------------
void C_FireSmoke::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if (m_eFireStyle != FIRE_ORANGEBOX)
		UpdateEffects();

	if ( updateType == DATA_UPDATE_CREATED )
	{
		Start();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_FireSmoke::UpdateEffects( void )
{
	if (m_pEmberEmitter.IsValid() && m_eFireStyle == FIRE_2003)
	{
		m_pEmberEmitter->SetSortOrigin(GetAbsOrigin());
	}


	if (m_pSmokeEmitter.IsValid())
	{
		m_pSmokeEmitter->SetSortOrigin(GetAbsOrigin());
	}

	if (m_pFireOverlay != NULL)
	{
		m_pFireOverlay->m_vPos = GetAbsOrigin();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_FireSmoke::ShouldDraw()
{
	if (m_eFireStyle != FIRE_ORANGEBOX)
		if (GetOwnerEntity() && GetOwnerEntity()->GetRenderColor().a == 0)
			return false;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_FireSmoke::Start(void)
{
	if (m_eFireStyle != FIRE_ORANGEBOX)
	{
		bool bTools = CommandLine()->CheckParm("-tools") != NULL;

		// Setup the render handles for stuff we want in the client leaf list.
		int i;
		for (i = 0; i < NUM_CHILD_FLAMES; i++)
		{
			if (bTools)
			{
				ClientEntityList().AddNonNetworkableEntity(&m_entFlames[i]);
			}
			m_entFlames[i].AddToLeafSystem(RENDER_GROUP_TRANSLUCENT_ENTITY);
		}

		if (m_nFlags & bitsFIRESMOKE_VISIBLE_FROM_ABOVE)
		{
			for (int i = 0; i < NUM_CHILD_FLAMES; i++)
			{
				if (bTools)
				{
					ClientEntityList().AddNonNetworkableEntity(&m_entFlamesFromAbove[i]);
				}
				m_entFlamesFromAbove[i].AddToLeafSystem(RENDER_GROUP_TRANSLUCENT_ENTITY);
			}
		}

		float rate = 2.0f;

		if (m_eFireStyle == FIRE_2003)
		{
			rate = 10.0f;
		}
		//Various setup info
		m_tParticleSpawn.Init(rate);

		QAngle	offset;
		model_t* pModel;
		int		maxFrames;

		//Setup the child flames
		for (i = 0; i < NUM_CHILD_FLAMES; i++)
		{
			//Setup our offset angles
			offset[0] = 0.0f;
			offset[1] = Helper_RandomFloat(0, 360);
			offset[2] = 0.0f;

			AngleVectors(offset, &m_entFlames[i].m_vecMoveDir);
			m_entFlames[i].m_bFadeFromAbove = (m_nFlags & bitsFIRESMOKE_VISIBLE_FROM_ABOVE);

			pModel = (model_t*)modelinfo->GetModel(m_nFlameModelIndex);
			maxFrames = modelinfo->GetModelFrameCount(pModel);

			//Setup all the information for the client entity
			m_entFlames[i].SetModelByIndex(m_nFlameModelIndex);
			m_entFlames[i].SetLocalOrigin(GetLocalOrigin());
			m_entFlames[i].m_flFrame = Helper_RandomInt(0.0f, maxFrames - 1);
			m_entFlames[i].m_flSpriteFramerate = Helper_RandomInt(15, 30);
			m_entFlames[i].SetScale(m_flStartScale);
			m_entFlames[i].SetRenderMode(kRenderTransAddFrameBlend);
			m_entFlames[i].m_nRenderFX = kRenderFxNone;
			m_entFlames[i].SetRenderColor(255, 255, 255, 255);
			m_entFlames[i].SetBrightness(255);
			m_entFlames[i].AddEffects(EF_NORECEIVESHADOW | EF_NOSHADOW);

			m_entFlames[i].index = -1;

			if (i == 0)
			{
				m_entFlameScales[i] = 1.0f;
			}
			else
			{
				//Keep a scale offset
				m_entFlameScales[i] = random->RandomFloat(0.5f, 1.0f);
			}
		}

		if (m_nFlags & bitsFIRESMOKE_VISIBLE_FROM_ABOVE)
		{
			for (int i = 0; i < NUM_CHILD_FLAMES; i++)
			{
				pModel = (model_t*)modelinfo->GetModel(m_nFlameFromAboveModelIndex);
				maxFrames = modelinfo->GetModelFrameCount(pModel);

				//Setup all the information for the client entity
				m_entFlamesFromAbove[i].SetModelByIndex(m_nFlameFromAboveModelIndex);
				m_entFlamesFromAbove[i].SetLocalOrigin(GetLocalOrigin());
				m_entFlamesFromAbove[i].m_flFrame = Helper_RandomInt(0.0f, maxFrames - 1);
				m_entFlamesFromAbove[i].m_flSpriteFramerate = Helper_RandomInt(15, 30);
				m_entFlamesFromAbove[i].SetScale(m_flStartScale);
				m_entFlamesFromAbove[i].SetRenderMode(kRenderTransAddFrameBlend);
				m_entFlamesFromAbove[i].m_nRenderFX = kRenderFxNone;
				m_entFlamesFromAbove[i].SetRenderColor(255, 255, 255, 255);
				m_entFlamesFromAbove[i].SetBrightness(255);
				m_entFlamesFromAbove[i].AddEffects(EF_NORECEIVESHADOW | EF_NOSHADOW);

				m_entFlamesFromAbove[i].index = -1;

				if (i == 0)
				{
					m_entFlameScales[i] = 1.0f;
				}
				else
				{
					//Keep a scale offset
					m_entFlameScales[i] = random->RandomFloat(0.5f, 1.0f);
				}
			}
		}

		// Start up the smoke
		if (m_nFlags & bitsFIRESMOKE_SMOKE)
		{
			if (m_eFireStyle == FIRE_2003)
				m_pEmberEmitter = CEmberEffect::Create("C_FireSmoke::m_pEmberEmitter");
			m_pSmokeEmitter = CLitSmokeEmitter::Create("C_FireSmoke::m_pSmokeEmitter");
			m_pSmokeEmitter->Init("particle/SmokeStack", GetAbsOrigin());
		}

		//Only make the glow if we've requested it
		if (m_nFlags & bitsFIRESMOKE_GLOW)
		{
#if 1
			if (m_eFireStyle == FIRE_2003)
				//Create the fire overlay
				if (m_pFireOverlay = new CFireOverlay(this))
				{
					m_pFireOverlay->m_vPos = GetAbsOrigin();
					m_pFireOverlay->m_nSprites = 1;

					m_pFireOverlay->m_vBaseColors[0].Init(0.4f, 0.2f, 0.05f);
					m_pFireOverlay->Activate();
				}
#endif
		}
	}
	else
	{
		const char* lpszEffectName;
		int nSize = (int)floor(m_flStartScale / 36.0f);
		switch (nSize)
		{
		case 0:
			lpszEffectName = (m_nFlags & bitsFIRESMOKE_SMOKE) ? "env_fire_tiny_smoke" : "env_fire_tiny";
			break;

		case 1:
			lpszEffectName = (m_nFlags & bitsFIRESMOKE_SMOKE) ? "env_fire_small_smoke" : "env_fire_small";
			break;

		case 2:
			lpszEffectName = (m_nFlags & bitsFIRESMOKE_SMOKE) ? "env_fire_medium_smoke" : "env_fire_medium";
			break;

		case 3:
		default:
			lpszEffectName = (m_nFlags & bitsFIRESMOKE_SMOKE) ? "env_fire_large_smoke" : "env_fire_large";
			break;
		}

		// Create the effect of the correct size
		m_hEffect = ParticleProp()->Create(lpszEffectName, PATTACH_ABSORIGIN);
	}
}


//-----------------------------------------------------------------------------
// Purpose: FIXME: what's the right way to do this?
//-----------------------------------------------------------------------------
void C_FireSmoke::StartClientOnly( void )
{
	Start();

	ClientEntityList().AddNonNetworkableEntity(	this );
	CollisionProp()->CreatePartitionHandle();
	AddEffects( EF_NORECEIVESHADOW | EF_NOSHADOW );
	AddToLeafSystem();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_FireSmoke::RemoveClientOnly(void)
{
	ClientThinkList()->RemoveThinkable( GetClientHandle() );

	// Remove from the client entity list.
	ClientEntityList().RemoveEntity( GetClientHandle() );

	partition->Remove( PARTITION_CLIENT_SOLID_EDICTS | PARTITION_CLIENT_RESPONSIVE_EDICTS | PARTITION_CLIENT_NON_STATIC_EDICTS, CollisionProp()->GetPartitionHandle() );

	RemoveFromLeafSystem();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_FireSmoke::UpdateAnimation( void )
{
	int		numFrames;
	float	frametime = Helper_GetFrameTime();

	for (int i = 0; i < NUM_CHILD_FLAMES; i++)
	{
		m_entFlames[i].m_flFrame += m_entFlames[i].m_flSpriteFramerate * frametime;

		numFrames = modelinfo->GetModelFrameCount(m_entFlames[i].GetModel());

		if (m_entFlames[i].m_flFrame >= numFrames)
		{
			m_entFlames[i].m_flFrame = m_entFlames[i].m_flFrame - (int)(m_entFlames[i].m_flFrame);
		}
	}

	if (m_nFlags & bitsFIRESMOKE_VISIBLE_FROM_ABOVE)
	{
		for (int i = 0; i < NUM_CHILD_FLAMES; i++)
		{
			m_entFlamesFromAbove[i].m_flFrame += m_entFlamesFromAbove[i].m_flSpriteFramerate * frametime;

			numFrames = modelinfo->GetModelFrameCount(m_entFlamesFromAbove[i].GetModel());

			if (m_entFlamesFromAbove[i].m_flFrame >= numFrames)
			{
				m_entFlamesFromAbove[i].m_flFrame = m_entFlamesFromAbove[i].m_flFrame - (int)(m_entFlamesFromAbove[i].m_flFrame);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_FireSmoke::UpdateFlames( void )
{
	for (int i = 0; i < NUM_CHILD_FLAMES; i++)
	{
		float	newScale = m_flScaleRegister * m_entFlameScales[i];
		Vector	dir;

		dir[2] = 0.0f;
		VectorNormalize(dir);
		dir[2] = 0.0f;

		Vector offset = GetAbsOrigin();
		offset[2] += FLAME_SOURCE_HEIGHT * m_entFlames[i].GetScale();

		//NOTENOTE: Sprite renderer assumes a scale of 0.0 means 1.0
		if (m_bFadingOut == false)
		{
			m_entFlames[i].SetScale(max(0.000001, newScale));
		}
		else
		{
			m_entFlames[i].SetScale(newScale);
		}

		Assert(!m_entFlames[i].GetMoveParent());
		if (i != 0)
		{
			m_entFlames[i].SetLocalOrigin(offset + (m_entFlames[i].m_vecMoveDir * (m_entFlames[i].GetScale() * m_flChildFlameSpread)));
		}
		else
		{
			m_entFlames[i].SetLocalOrigin(offset);
		}
	}

	if (m_nFlags & bitsFIRESMOKE_VISIBLE_FROM_ABOVE)
	{
		for (int i = 0; i < NUM_CHILD_FLAMES; i++)
		{
			float	newScale = m_flScaleRegister * m_entFlameScales[i];
			Vector	dir;

			dir[2] = 0.0f;
			VectorNormalize(dir);
			dir[2] = 0.0f;

			Vector offset = GetAbsOrigin();
			offset[2] += FLAME_FROM_ABOVE_SOURCE_HEIGHT * m_entFlamesFromAbove[i].GetScale();

			//NOTENOTE: Sprite renderer assumes a scale of 0.0 means 1.0
			if (m_bFadingOut == false)
			{
				m_entFlamesFromAbove[i].SetScale(max(0.000001, newScale));
			}
			else
			{
				m_entFlamesFromAbove[i].SetScale(newScale);
			}

			Assert(!m_entFlamesFromAbove[i].GetMoveParent());
			if (i != 0)
			{
				m_entFlamesFromAbove[i].SetLocalOrigin(offset + (m_entFlames[i].m_vecMoveDir * (m_entFlamesFromAbove[i].GetScale() * m_flChildFlameSpread)));
			}
			else
			{
				m_entFlamesFromAbove[i].SetLocalOrigin(offset);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_FireSmoke::UpdateScale( void )
{
	float	time = Helper_GetTime();

	if (m_flScaleRegister != m_flScaleEnd)
	{
		//See if we're done scaling
		if (time > m_flScaleTimeEnd)
		{
			m_flScaleRegister = m_flStartScale = m_flScaleEnd;
		}
		else
		{
			//Lerp the scale and set it 
			float	timeFraction = 1.0f - (m_flScaleTimeEnd - time) / (m_flScaleTimeEnd - m_flScaleTimeStart);
			float	newScale = 0.0f;

			if (m_bFadingOut == false)
				newScale = m_flScaleStart + ((m_flScaleEnd - m_flScaleStart) * timeFraction);
			else
				newScale = m_flScaleStart - ((m_flScaleStart - m_flScaleEnd) * timeFraction);

			m_flScaleRegister = m_flStartScale = newScale;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_FireSmoke::Update( void )
{
	//If we haven't already, find the clip plane for smoke effects
	if ((m_nFlags & bitsFIRESMOKE_SMOKE) && (m_bClipTested == false))
	{
		FindClipPlane();
	}

	//Update all our parts
	UpdateEffects();
	UpdateScale();
	UpdateAnimation();
	UpdateFlames();

	//See if we should emit smoke
	if (m_nFlags & bitsFIRESMOKE_SMOKE)
	{
		float tempDelta = Helper_GetFrameTime();

		while (m_tParticleSpawn.NextEvent(tempDelta))
		{
			SpawnSmoke();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_FireSmoke::FindClipPlane( void )
{
	m_bClipTested = true;
	m_flClipPerc = 1.0f;

	trace_t	tr;
	Vector	end(0.0f, 0.0f, SMOKE_RISE_RATE * SMOKE_LIFETIME);

	UTIL_TraceLine(GetAbsOrigin(), GetAbsOrigin() + end, MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr);

	//If the ceiling is too close, reject smoke
	if (tr.fraction < 0.5f)
	{
		m_nFlags &= ~bitsFIRESMOKE_SMOKE;
		return;
	}

	if (tr.fraction < 1.0f)
	{
		m_planeClip.Init(tr.plane.normal, tr.plane.dist);
		m_nFlags |= bitsFIRESMOKE_SMOKE_COLLISION;
		m_flClipPerc = tr.fraction * 0.5f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Spawn smoke (...duh)
//-----------------------------------------------------------------------------

void C_FireSmoke::SpawnSmoke( void )
{
	if (m_pEmberEmitter.IsValid() == false && m_eFireStyle == FIRE_2003)
		return;

	if (m_pSmokeEmitter.IsValid() == false)
		return;

	float			scalar;
	Vector			offset;

	scalar = 32.0f * m_flScaleRegister;
	offset[0] = Helper_RandomFloat(-scalar, scalar);
	offset[1] = Helper_RandomFloat(-scalar, scalar);
	offset[2] = scalar + (Helper_RandomFloat(-scalar * 0.25f, scalar * 0.25f));

	//
	// Embers
	//

	if (m_eFireStyle == FIRE_2003)
	{
		//FIXME: These aren't visible enough to justify their cost, currently -- jdw

		SimpleParticle* sParticle;
		sParticle = (SimpleParticle*)m_pEmberEmitter->AddParticle(sizeof(SimpleParticle), m_pEmberEmitter->GetPMaterial("particle/fire"), GetAbsOrigin() + offset);

		if (sParticle)
		{
			sParticle->m_flLifetime = 0.0f;
			sParticle->m_flDieTime = EMBER_LIFETIME;

			sParticle->m_flRoll = 0;
			sParticle->m_flRollDelta = 0;

			scalar = Helper_RandomFloat(0.5f, 2.0f);

			sParticle->m_uchColor[0] = min(255, Helper_RandomFloat(185.0f, 190.0f) * scalar);
			sParticle->m_uchColor[1] = min(255, Helper_RandomFloat(140.0f, 165.0f) * scalar);
			sParticle->m_uchColor[2] = min(255, 65.0f * scalar);
			sParticle->m_uchStartAlpha = 255;
			sParticle->m_uchEndAlpha = 0;
			sParticle->m_uchStartSize = 2;
			sParticle->m_uchEndSize = 0;

			sParticle->m_vecVelocity = Vector(Helper_RandomFloat(-16.0f, 16.0f), Helper_RandomFloat(-16.0f, 16.0f), Helper_RandomFloat(92.0f, 128.0f));
		}
	}

	//
	// Lit smoke
	//

	if (m_eFireStyle == FIRE_2003)
	{
		offset[2] += 32.0f;
	}
	else
	{
		offset[2] += 100.0f;
	}

	m_pSmokeEmitter->SetDirectionalLight(GetAbsOrigin(), Vector(1.0f, 0.5f, 0.2f), 2500);

	CLitSmokeEmitter::LitSmokeParticle* pParticle;
	pParticle = (CLitSmokeEmitter::LitSmokeParticle*)m_pSmokeEmitter->AddParticle(
		sizeof(CLitSmokeEmitter::LitSmokeParticle),
		m_pSmokeEmitter->GetSmokeMaterial(),
		GetAbsOrigin() + offset);

	if (pParticle)
	{
		float lifeTime = SMOKE_LIFETIME * m_flClipPerc;

		pParticle->m_flLifetime = 0;
		pParticle->m_flDieTime = random->RandomFloat(lifeTime * 0.75, lifeTime);

		if (m_eFireStyle == FIRE_2003)
		{

			pParticle->m_vecVelocity = Vector(random->RandomFloat(-16.0f, 16.0f), random->RandomFloat(-16.0f, 16.0f), random->RandomFloat(SMOKE_RISE_RATE - 16.0f, SMOKE_RISE_RATE));

			int	color = random->RandomInt(8, 32);
			pParticle->m_uchColor[0] = color;
			pParticle->m_uchColor[1] = color;
			pParticle->m_uchColor[2] = color;
			pParticle->m_uchColor[3] = random->RandomInt(64, 200);

			pParticle->m_uchStartSize = random->RandomFloat(12.0f, 16.0f);
			pParticle->m_uchEndSize = pParticle->m_uchStartSize * 4.0f;
		}
		else
		{
			pParticle->m_vecVelocity = Vector(random->RandomFloat(-16.0f, 16.0f), random->RandomFloat(-16.0f, 16.0f), random->RandomFloat(2.0f, SMOKE_RISE_RATE));

			int	color = random->RandomInt(8, 64);
			pParticle->m_uchColor[0] = color;
			pParticle->m_uchColor[1] = color;
			pParticle->m_uchColor[2] = color;
			pParticle->m_uchColor[3] = random->RandomInt(128, 256);

			pParticle->m_uchStartSize = random->RandomFloat(32.0f, 48.0f);
			pParticle->m_uchEndSize = pParticle->m_uchStartSize * 3.0f;
		}
	}
}


IMPLEMENT_CLIENTCLASS_DT( C_EntityFlame, DT_EntityFlame, CEntityFlame )
	RecvPropEHandle(RECVINFO(m_hEntAttached)),
	RecvPropInt(RECVINFO(m_eFireType)), //Lychy: client needs to know whether to use normal or plasma particle
	RecvPropBool(RECVINFO(m_bUseHitboxes)),
	RecvPropTime(RECVINFO(m_flLifetime)),
	RecvPropFloat(RECVINFO(m_flSize)),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_EntityFlame::C_EntityFlame( void ) :
m_hEffect( NULL )
{
	m_eFireStyle = (FireStyles_e)manod_fire_new.GetInt();
	m_hOldAttached = NULL;
	m_flSize = 4.0f;
	m_pEmitter = NULL;
	m_flLifetime = 0;
	m_bStartedFading = false;
	m_bCreatedClientside = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_EntityFlame::~C_EntityFlame( void )
{
	StopEffect();
	if (m_bAttachedToHitboxes)
	{
		for (int i = 0; i < NUM_HITBOX_FIRES; i++)
		{
			delete m_pFireSmoke[i];
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EntityFlame::StopEffect( void )
{
	if ( m_hEffect )
	{
		ParticleProp()->StopEmission( m_hEffect, true );
		m_hEffect->SetControlPointEntity( 0, NULL );
		m_hEffect->SetControlPointEntity( 1, NULL );
		m_hEffect = NULL;
	}

	if ( m_hEntAttached )
	{
		m_hEntAttached->RemoveFlag( FL_ONFIRE );
		m_hEntAttached->SetEffectEntity( NULL );
		m_hEntAttached->StopSound( "General.BurningFlesh" );
		m_hEntAttached->StopSound( "General.BurningObject" );
		m_hEntAttached->StopSound("ManodGeneral.BurningPlasmaObject");
		m_hEntAttached->StopSound("ManodGeneral.BurningPlasmaFlesh");
		
		
		m_hEntAttached = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EntityFlame::UpdateOnRemove( void )
{
	if (m_eFireStyle == FIRE_2003)
		CleanUpRagdollOnRemove();
	StopEffect();
	BaseClass::UpdateOnRemove();
}

void C_EntityFlame::CleanUpRagdollOnRemove(void)
{
	if (!m_hEntAttached)
		return;

	m_hEntAttached->RemoveFlag(FL_ONFIRE);
	m_hEntAttached->SetEffectEntity(NULL);
	m_hEntAttached->StopSound("General.BurningFlesh");
	m_hEntAttached->StopSound("General.BurningObject");

	m_hEntAttached = NULL;
}

void C_EntityFlame::CreateEffect( void )
{
	if (m_hEntAttached && m_hEntAttached->IsPlayer())
	{
		return;
	}
	if ( m_hEffect )
	{
		ParticleProp()->StopEmission( m_hEffect, true );
		m_hEffect->SetControlPointEntity( 0, NULL );
		m_hEffect->SetControlPointEntity( 1, NULL );
		m_hEffect = NULL;
	}
	const char* szEffectName = m_eFireType == FIRE_PLASMA ? "burning_character_plasma" : "burning_character";
#ifdef TF_CLIENT_DLL
	m_hEffect = ParticleProp()->Create( "burningplayer_red", PATTACH_ABSORIGIN_FOLLOW );
#else
	m_hEffect = ParticleProp()->Create( szEffectName, PATTACH_ABSORIGIN_FOLLOW );
#endif
	if ( m_hEffect )
	{
		C_BaseEntity *pEntity = m_hEntAttached;
		m_hOldAttached = m_hEntAttached;

		ParticleProp()->AddControlPoint( m_hEffect, 1, pEntity, PATTACH_ABSORIGIN_FOLLOW );
		m_hEffect->SetControlPoint( 0, GetAbsOrigin() );
		m_hEffect->SetControlPoint( 1, GetAbsOrigin() );
		m_hEffect->SetControlPointEntity( 0, pEntity );
		m_hEffect->SetControlPointEntity( 1, pEntity );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EntityFlame::OnDataChanged( DataUpdateType_t updateType )
{
	if (m_eFireStyle != FIRE_ORANGEBOX)
	{
		if (updateType == DATA_UPDATE_CREATED)
		{
			C_BaseEntity* pEnt = m_hEntAttached;
			if (!pEnt)
				return;

			if (pEnt->IsPlayer())
				return;

			if (m_bUseHitboxes && pEnt->GetBaseAnimating() != NULL)
			{
				AttachToHitBoxes();
			}
			else
			{
				m_vecLastPosition = GetRenderOrigin();

				m_ParticleSpawn.Init(60); //Events per second

				m_pEmitter = CEmberEffect::Create("C_EntityFlame::Create");

				Assert(m_pEmitter.IsValid());
				if (m_pEmitter.IsValid())
				{
					for (int i = 1; i < NUM_FLAMELETS + 1; i++)
					{
						m_MaterialHandle[i - 1] = m_pEmitter->GetPMaterial(VarArgs("sprites/flamelet%d", i));
					}

					m_pEmitter->SetSortOrigin(GetAbsOrigin());
				}
			}
		}

	}
	else
	{
		if (updateType == DATA_UPDATE_CREATED)
		{
			CreateEffect();
		}

		// FIXME: This is a bit of a shady path
		if (updateType == DATA_UPDATE_DATATABLE_CHANGED)
		{
			// If our owner changed, then recreate the effect
			if (m_hEntAttached != m_hOldAttached)
			{
				CreateEffect();
			}
		}
}
	BaseClass::OnDataChanged( updateType );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EntityFlame::Simulate( void )
{
	if ( gpGlobals->frametime <= 0.0f )
		return;

	if (m_eFireStyle != FIRE_ORANGEBOX)
	{
#ifdef HL2_EPISODIC 
		// Server side flames need to shrink and die
		if (!m_bStartedFading)
		{
			float flTTL = (m_flLifetime - gpGlobals->curtime);
			if (flTTL < 2.0)
			{
				for (int i = 0; i < NUM_HITBOX_FIRES; i++)
				{
					if (m_pFireSmoke[i])
					{
						m_pFireSmoke[i]->m_flScaleStart = m_pFireSmoke[i]->m_flScaleEnd;
						m_pFireSmoke[i]->m_flScaleEnd = 0.00001;
						m_pFireSmoke[i]->m_flScaleTimeStart = gpGlobals->curtime;
						m_pFireSmoke[i]->m_flScaleTimeEnd = m_flLifetime;
						m_pFireSmoke[i]->m_flScaleRegister = -1;
					}
				}

				m_bStartedFading = true;
			}
		}

		if (IsEffectActive(EF_BRIGHTLIGHT) || IsEffectActive(EF_DIMLIGHT))
		{
			dlight_t* dl = effects->CL_AllocDlight(index);
			dl->origin = GetAbsOrigin();
			dl->origin[2] += 16;
			dl->color.r = 254;
			dl->color.g = 174;
			dl->color.b = 10;
			dl->radius = random->RandomFloat(400, 431);
			dl->die = gpGlobals->curtime + 0.001;

			if (m_pFireSmoke[0])
			{
				if (m_pFireSmoke[0]->m_flScaleRegister == -1)
				{
					// We've started shrinking, but UpdateScale() hasn't been
					// called since then. We want to use the Start scale instead.
					dl->radius *= m_pFireSmoke[0]->m_flScaleStart;
				}
				else
				{
					dl->radius *= m_pFireSmoke[0]->m_flScaleRegister;
				}
			}
		}
#endif // HL2_EPISODIC 

		if (m_bAttachedToHitboxes)
		{
			UpdateHitBoxFlames();
		}
		else if (!!m_pEmitter)
		{
			m_pEmitter->SetSortOrigin(GetAbsOrigin());

			SimpleParticle* pParticle;

			Vector	offset;

			Vector	moveDiff = GetAbsOrigin() - m_vecLastPosition;
			float	moveLength = VectorNormalize(moveDiff);

			int	numPuffs = moveLength / (m_flSize * 0.5f);

			numPuffs = clamp(numPuffs, 1, 8);

			Vector			offsetColor;
			float			step = moveLength / numPuffs;

			//Fill in the gaps
			for (int i = 1; i < numPuffs + 1; i++)
			{
				offset = m_vecLastPosition + (moveDiff * step * i);

				pParticle = (SimpleParticle*)m_pEmitter->AddParticle(sizeof(SimpleParticle), m_MaterialHandle[random->RandomInt(0, NUM_FLAMELETS - 1)], offset);

				if (pParticle)
				{
					pParticle->m_flDieTime = 0.4f;
					pParticle->m_flLifetime = 0.0f;

					pParticle->m_flRoll = random->RandomInt(0, 360);
					pParticle->m_flRollDelta = random->RandomFloat(-2.0f, 2.0f);

					pParticle->m_uchStartSize = random->RandomInt(m_flSize * 0.25f, m_flSize * 0.5f);
					pParticle->m_uchEndSize = pParticle->m_uchStartSize * 2.0f;
					pParticle->m_uchStartAlpha = 255;
					pParticle->m_uchEndAlpha = 0;

					pParticle->m_uchColor[0] = pParticle->m_uchColor[1] = pParticle->m_uchColor[2] = 255;

					Vector dir;

					dir.x = random->RandomFloat(-1.0f, 1.0f);
					dir.y = random->RandomFloat(-1.0f, 1.0f);
					dir.z = random->RandomFloat(0.5f, 1.0f);

					pParticle->m_vecVelocity = dir * random->RandomInt(4, 32);
					pParticle->m_vecVelocity[2] = random->RandomInt(32, 64);
				}
			}
		}

		m_vecLastPosition = GetRenderOrigin();
	}
#ifdef HL2_EPISODIC 

	if ( IsEffectActive(EF_BRIGHTLIGHT) || IsEffectActive(EF_DIMLIGHT) )
	{
		dlight_t *dl = effects->CL_AllocDlight ( index );
		dl->origin = GetAbsOrigin();
 		dl->origin[2] += 16;
		
		if (m_eFireType == FIRE_PLASMA)
		{ 
			dl->color.r = 60;
			dl->color.g = 200;
			dl->color.b = 200;
		} 
		else 
		{
			dl->color.r = 254;
			dl->color.g = 174;
			dl->color.b = 10;
		}
		dl->radius = random->RandomFloat(400,431);
		dl->die = gpGlobals->curtime + 0.001;
	}

#endif // HL2_EPISODIC 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EntityFlame::ClientThink( void )
{
	StopEffect();
	Release();
}


//-----------------------------------------------------------------------------
// Purpose: Attaches fire to the hitboxes of an animating character. The fire
//			is distributed based on hitbox volumes -- it attaches to the larger
//			hitboxes first.
//-----------------------------------------------------------------------------
void C_EntityFlame::AttachToHitBoxes(void)
{
	m_pCachedModel = NULL;

	C_BaseCombatCharacter* pAnimating = (C_BaseCombatCharacter*)m_hEntAttached.Get();
	if (!pAnimating || !pAnimating->GetModel())
	{
		return;
	}

	CStudioHdr* pStudioHdr = pAnimating->GetModelPtr();
	if (!pStudioHdr)
	{
		return;
	}

	mstudiohitboxset_t* set = pStudioHdr->pHitboxSet(pAnimating->m_nHitboxSet);
	if (!set)
	{
		return;
	}

	if (!set->numhitboxes)
	{
		return;
	}

	m_pCachedModel = pAnimating->GetModel();

	CBoneCache* pCache = pAnimating->GetBoneCache(pStudioHdr);
	matrix3x4_t* hitboxbones[MAXSTUDIOBONES];
	pCache->ReadCachedBonePointers(hitboxbones, pStudioHdr->numbones());

	//
	// Sort the hitboxes by volume.
	//
	HitboxVolume_t hitboxvolume[MAXSTUDIOBONES];
	for (int i = 0; i < set->numhitboxes; i++)
	{
		mstudiobbox_t* pBox = set->pHitbox(i);
		hitboxvolume[i].nIndex = i;
		hitboxvolume[i].flVolume = CalcBoxVolume(pBox->bbmin, pBox->bbmax);
	}
	qsort(hitboxvolume, set->numhitboxes, sizeof(hitboxvolume[0]), (int(__cdecl*)(const void*, const void*))SortHitboxVolumes);

	//
	// Attach fire to the hitboxes.
	//
	for (int i = 0; i < NUM_HITBOX_FIRES; i++)
	{
		int hitboxindex;
		//
		// Pick the 5 biggest hitboxes, or random ones if there are less than 5 hitboxes,
		// then pick random ones after that.
		//
		if ((i < 5) && (i < set->numhitboxes))
		{
			hitboxindex = i;
		}
		else
		{
			hitboxindex = random->RandomInt(0, set->numhitboxes - 1);
		}

		mstudiobbox_t* pBox = set->pHitbox(hitboxvolume[hitboxindex].nIndex);

		Assert(hitboxbones[pBox->bone]);

		m_nHitbox[i] = hitboxvolume[hitboxindex].nIndex;
		m_pFireSmoke[i] = new C_FireSmoke;

		//
		// Calculate a position within the hitbox to place the fire.
		//
		m_vecFireOrigin[i] = Vector(random->RandomFloat(pBox->bbmin.x, pBox->bbmax.x), random->RandomFloat(pBox->bbmin.y, pBox->bbmax.y), random->RandomFloat(pBox->bbmin.z, pBox->bbmax.z));
		Vector vecAbsOrigin;
		VectorTransform(m_vecFireOrigin[i], *hitboxbones[pBox->bone], vecAbsOrigin);
		m_pFireSmoke[i]->SetLocalOrigin(vecAbsOrigin);


		//
		// The first fire emits smoke, the rest do not.
		//
		m_pFireSmoke[i]->m_nFlags = bitsFIRESMOKE_ACTIVE;

		if (m_eFireStyle == FIRE_2003)
		{
			m_pFireSmoke[i]->m_nFlags |= bitsFIRESMOKE_GLOW;
			if (m_eFireType == FIRE_NATURAL && i < 2)
			{
				m_pFireSmoke[i]->m_nFlags |= bitsFIRESMOKE_SMOKE;
			}
		}

		
		switch (m_eFireType)
		{
		case FIRE_PLASMA:
			m_pFireSmoke[i]->m_nFlameModelIndex = modelinfo->GetModelIndex("sprites/plasma1.vmt");
			break;
		default:
			m_pFireSmoke[i]->m_nFlameModelIndex = modelinfo->GetModelIndex("sprites/fire1.vmt");
		}
		m_pFireSmoke[i]->m_nFlameFromAboveModelIndex = modelinfo->GetModelIndex("sprites/flamefromabove.vmt");
		m_pFireSmoke[i]->m_flScale = 0;
		m_pFireSmoke[i]->m_flStartScale = 0;
		m_pFireSmoke[i]->m_flScaleTime = 1.5;
		m_pFireSmoke[i]->m_flScaleRegister = 0.1;
		m_pFireSmoke[i]->m_flChildFlameSpread = 20.0;
		m_pFireSmoke[i]->m_flScaleStart = 0;
		m_pFireSmoke[i]->SetOwnerEntity(this);

		// Do a simple That Looks About Right clamp on the volumes
		// so that we don't get flames too large or too tiny.
		float flVolume = hitboxvolume[hitboxindex].flVolume;

		Assert(IsFinite(flVolume));

#define FLAME_HITBOX_MIN_VOLUME 1000.0f
#define FLAME_HITBOX_MAX_VOLUME 4000.0f

		if (flVolume < FLAME_HITBOX_MIN_VOLUME)
		{
			flVolume = FLAME_HITBOX_MIN_VOLUME;
		}
		else if (flVolume > FLAME_HITBOX_MAX_VOLUME)
		{
			flVolume = FLAME_HITBOX_MAX_VOLUME;
		}

		m_pFireSmoke[i]->m_flScaleEnd = 0.00012f * flVolume;
		m_pFireSmoke[i]->m_flScaleTimeStart = Helper_GetTime();
		m_pFireSmoke[i]->m_flScaleTimeEnd = Helper_GetTime() + 2.0;

		m_pFireSmoke[i]->StartClientOnly();
	}

	m_bAttachedToHitboxes = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EntityFlame::UpdateHitBoxFlames(void)
{
	C_BaseCombatCharacter* pAnimating = (C_BaseCombatCharacter*)m_hEntAttached.Get();
	if (!pAnimating)
	{
		return;
	}

	if (pAnimating->GetModel() != m_pCachedModel)
	{
		if (m_pCachedModel != NULL)
		{
			// The model changed, we must reattach the flames.
			DeleteHitBoxFlames();
			AttachToHitBoxes();
		}

		if (m_pCachedModel == NULL)
		{
			// We tried to reattach and failed.
			return;
		}
	}

	studiohdr_t* pStudioHdr = modelinfo->GetStudiomodel(pAnimating->GetModel());
	if (!pStudioHdr)
	{
		return;
	}

	mstudiohitboxset_t* set = pStudioHdr->pHitboxSet(pAnimating->m_nHitboxSet);
	if (!set)
	{
		return;
	}

	if (!set->numhitboxes)
	{
		return;
	}

	pAnimating->SetupBones(NULL, -1, BONE_USED_BY_ANYTHING, gpGlobals->curtime);

	for (int i = 0; i < NUM_HITBOX_FIRES; i++)
	{
		Vector vecAbsOrigin;
		mstudiobbox_t* pBox = set->pHitbox(m_nHitbox[i]);

		VectorTransform(m_vecFireOrigin[i], pAnimating->GetBoneForWrite(pBox->bone), vecAbsOrigin);

		m_pFireSmoke[i]->SetLocalOrigin(vecAbsOrigin);
	}
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EntityFlame::DeleteHitBoxFlames(void)
{
	for (int i = 0; i < NUM_HITBOX_FIRES; i++)
	{
		m_pFireSmoke[i]->RemoveClientOnly();
		delete m_pFireSmoke[i];
	}
}

RenderGroup_t C_EntityFlame::GetRenderGroup()
{
	return RENDER_GROUP_TRANSLUCENT_ENTITY;
}