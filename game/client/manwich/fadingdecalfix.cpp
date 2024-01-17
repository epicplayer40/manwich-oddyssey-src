#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif // WIN32

#include "cbase.h"
#include "iefx.h"
#include "materialsystem/imaterialvar.h"
#include "decals.h"



// Most of this is from r_decal.cpp

struct msurface2_t;
typedef msurface2_t* SurfaceHandle_t;

// NOTE: There are used by footprints; maybe we separate into a separate struct?
#define FDECAL_USESAXIS				0x80		// Uses the s axis field to determine orientation
#define FDECAL_DYNAMIC				0x100		// Indicates the decal is dynamic
#define FDECAL_SECONDPASS			0x200		// Decals that have to be drawn after everything else
#define FDECAL_DONTSAVE				0x800		// Decal was loaded from adjacent level, don't save out to save file for this level
#define FDECAL_PLAYERSPRAY			0x1000		// Decal is a player spray
#define FDECAL_DISTANCESCALE		0x2000		// Decal is dynamically scaled based on distance.
#define FDECAL_HASUPDATED			0x4000		// Decal has not been updated this frame yet

//-----------------------------------------------------------------------------
// Handle to decals + shadows on displacements
//-----------------------------------------------------------------------------
typedef unsigned short DispDecalHandle_t;

// JAY: Compress this as much as possible
// decal instance
struct decal_t
{
	decal_t* pnext;				// linked list for each surface
	decal_t* pDestroyList;		//
	SurfaceHandle_t		surfID;		// Surface id for persistence / unlinking
	IMaterial* material;
	float				lightmapOffset;

	// FIXME:
	// make dx and dy in decal space and get rid of position, so that
	// position can be rederived from the decal basis.
	Vector		position;		// location of the decal center in world space.
	Vector		saxis;			// direction of the s axis in world space
	float		dx;				// Offsets into surface texture (in texture coordinates, so we don't need floats)
	float		dy;
	float		scale;			// Pixel scale
	float		flSize;			// size of decal, used for rejecting on dispinfo planes
	float		fadeDuration;				// Negative value means to fade in
	float		fadeStartTime;
	color32		color;
	void* userdata;		// For player decals only, decal index ( first player at slot 1 )
	DispDecalHandle_t	m_DispDecal;	// Handle to displacement decals associated with this
	unsigned short		clippedVertCount;
	unsigned short		cacheHandle;
	unsigned short		m_iDecalPool;		// index into the decal pool.
	short		flags;			// Decal flags  DECAL_*		!!!SAVED AS A BYTE (SEE HOST_CMD.C)
	short		entityIndex;	// Entity this is attached to

	// NOTE: The following variables are dynamic variables.
	// We could put these into a separate array and reference them
	// by index to reduce memory costs of this...

	int			m_iSortTree;			// MaterialSort tree id
	int			m_iSortMaterial;		// MaterialSort id.
};


CUtlVector<decal_t*>*		s_aDecalPool = NULL;

static unsigned int s_DecalFadeVarCache = 0;
static unsigned int s_DecalFadeTimeVarCache = 0;

void DecalShootFixed(int textureIndex, int entity,
	const model_t* model, const Vector& model_origin, const QAngle& model_angles,
	const Vector& position, const Vector* saxis, int flags)
{
	effects->DecalShoot(textureIndex, entity, model, model_origin, model_angles, position, saxis, flags);

	static bool valid = false;
#ifdef WIN32
	if (!valid)
	{
		//Update this offset if engine.dll is ever updated
		//This is where s_aDecalPool is located, would usually be in r_decal.cpp
		DWORD baseAddress = (DWORD)GetModuleHandle("engine.dll");
		s_aDecalPool = reinterpret_cast <CUtlVector<decal_t*>*> (baseAddress + 0x58C24C);
		valid = s_aDecalPool->Size() == 2048;
	}
#endif // WIN32

	if (!valid)
		return;

	const char* pDecalName = decalsystem->GetDecalNameForIndex(textureIndex);
	for (int i = 0; i < s_aDecalPool->Count(); i++)
	{
		decal_t* pDecal = s_aDecalPool->Element(i);
		if (!pDecal)
			continue;
		const char* pMaterialFound = pDecal->material->GetName();

		if (!pMaterialFound)
			continue;

		if (FStrEq(pDecalName, pMaterialFound) && pDecal->position == position)
		{
			IMaterial* pMaterial = pDecal->material;
			IMaterialVar* pFadeVar = pMaterial->FindVarFast("$decalFadeDuration", &s_DecalFadeVarCache);
			if (pFadeVar)
			{
				pDecal->flags |= FDECAL_DYNAMIC;
				pDecal->fadeDuration = pFadeVar->GetFloatValue();
				pFadeVar = pMaterial->FindVarFast("$decalFadeTime", &s_DecalFadeTimeVarCache);
				pDecal->fadeStartTime = pFadeVar ? pFadeVar->GetFloatValue() : 0.0f;
				pDecal->fadeStartTime += gpGlobals->curtime;
			}
		}
	}
}