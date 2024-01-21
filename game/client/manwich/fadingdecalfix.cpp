#include "cbase.h"
#include "iefx.h"
#include "materialsystem/imaterialvar.h"
#include "decals.h"
#include "model_types.h"
#include "engine_hooks.h"

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
struct worldbrushdata_t;

struct mleaf_t;
struct mleafwaterdata_t;
struct mvertex_t;
struct mtexinfo_t;
struct msurface1_t;
struct msurfacelighting_t;
struct msurfacenormal_t;
struct lightzbuffer_t;
struct mprimitive_t;
struct mprimvert_t;
struct mcubemapsample_t;
struct mleafambientindex_t;
struct mleafambientlighting_t;
typedef void* HDISPINFOARRAY;

//-----------------------------------------------------------------------------
// Handle to decals + shadows on displacements
//-----------------------------------------------------------------------------
typedef unsigned short DispDecalHandle_t;

enum
{
	DISP_DECAL_HANDLE_INVALID = (DispDecalHandle_t)~0
};

typedef unsigned short DispShadowHandle_t;

enum
{
	DISP_SHADOW_HANDLE_INVALID = (DispShadowHandle_t)~0
};

struct mnode_t
{
	// common with leaf
	int			contents;		// <0 to differentiate from leafs
	// -1 means check the node for visibility
	// -2 means don't check the node for visibility

	int			visframe;		// node needs to be traversed if current

	mnode_t* parent;
	short		area;			// If all leaves below this node are in the same area, then
	// this is the area index. If not, this is -1.
	short		flags;

	VectorAligned		m_vecCenter;
	VectorAligned		m_vecHalfDiagonal;

	// node specific
	cplane_t* plane;
	mnode_t* children[2];

	unsigned short		firstsurface;
	unsigned short		numsurfaces;
};
struct worldbrushdata_t
{
	int			numsubmodels;

	int			numplanes;
	cplane_t* planes;

	int			numleafs;		// number of visible leafs, not counting 0
	mleaf_t* leafs;

	int			numleafwaterdata;
	mleafwaterdata_t* leafwaterdata;

	int			numvertexes;
	mvertex_t* vertexes;

	int			numoccluders;
	doccluderdata_t* occluders;

	int			numoccluderpolys;
	doccluderpolydata_t* occluderpolys;

	int			numoccludervertindices;
	int* occludervertindices;

	int				numvertnormalindices;	// These index vertnormals.
	unsigned short* vertnormalindices;

	int			numvertnormals;
	Vector* vertnormals;

	int			numnodes;
	mnode_t* nodes;
	unsigned short* m_LeafMinDistToWater;

	int			numtexinfo;
	mtexinfo_t* texinfo;

	int			numtexdata;
	csurface_t* texdata;

	int         numDispInfos;
	HDISPINFOARRAY	hDispInfos;	// Use DispInfo_Index to get IDispInfos..

	/*
	int         numOrigSurfaces;
	msurface_t  *pOrigSurfaces;
	*/

	int			numsurfaces;
	msurface1_t* surfaces1;
	msurface2_t* surfaces2;
	msurfacelighting_t* surfacelighting;
	msurfacenormal_t* surfacenormals;

	bool		unloadedlightmaps;

	int			numvertindices;
	unsigned short* vertindices;

	int nummarksurfaces;
	SurfaceHandle_t* marksurfaces;

	ColorRGBExp32* lightdata;

	int			numworldlights;
	dworldlight_t* worldlights;

	lightzbuffer_t* shadowzbuffers;

	// non-polygon primitives (strips and lists)
	int			numprimitives;
	mprimitive_t* primitives;

	int			numprimverts;
	mprimvert_t* primverts;

	int			numprimindices;
	unsigned short* primindices;

	int				m_nAreas;
	darea_t* m_pAreas;

	int				m_nAreaPortals;
	dareaportal_t* m_pAreaPortals;

	int				m_nClipPortalVerts;
	Vector* m_pClipPortalVerts;

	mcubemapsample_t* m_pCubemapSamples;
	int				   m_nCubemapSamples;

	int				m_nDispInfoReferences;
	unsigned short* m_pDispInfoReferences;

	mleafambientindex_t* m_pLeafAmbient;
	mleafambientlighting_t* m_pAmbientSamples;
#if 0
	int			numportals;
	mportal_t* portals;

	int			numclusters;
	mcluster_t* clusters;

	int			numportalverts;
	unsigned short* portalverts;

	int			numclusterportals;
	unsigned short* clusterportals;
#endif
};

// This structure contains the information used to create new decals
struct decalinfo_t
{
	Vector		m_Position;			// world coordinates of the decal center
	Vector		m_SAxis;			// the s axis for the decal in world coordinates
	model_t* m_pModel;			// the model the decal is going to be applied in
	worldbrushdata_t* m_pBrush;		// The shared brush data for this model
	IMaterial* m_pMaterial;		// The decal material
	float		m_Size;				// Size of the decal (in world coords)
	int			m_Flags;
	int			m_Entity;			// Entity the decal is applied to.
	float		m_scale;
	float		m_flFadeDuration;
	float		m_flFadeStartTime;
	int			m_decalWidth;
	int			m_decalHeight;
	color32		m_Color;
	Vector		m_Basis[3];
	void* m_pUserData;
	const Vector* m_pNormal;
	CUtlVector<SurfaceHandle_t>	m_aApplySurfs;
};

// only models with type "mod_brush" have this data
struct brushdata_t
{
	worldbrushdata_t* pShared;
	int			firstmodelsurface, nummodelsurfaces;

	unsigned short	renderHandle;
	unsigned short	firstnode;
};

// only models with type "mod_sprite" have this data
struct spritedata_t
{
	int				numframes;
	int				width;
	int				height;
	CEngineSprite* sprite;
};

struct model_t
{
	FileNameHandle_t	fnHandle;
	CUtlString			strName;

	int					nLoadFlags;		// mark loaded/not loaded
	int					nServerCount;	// marked at load
	IMaterial** ppMaterials;	// null-terminated runtime material cache; ((intptr_t*)(ppMaterials))[-1] == nMaterials

	modtype_t			type;
	int					flags;			// MODELFLAG_???

	// volume occupied by the model graphics	
	Vector				mins, maxs;
	float				radius;

	union
	{
		brushdata_t		brush;
		MDLHandle_t		studio;
		spritedata_t	sprite;
	};
};

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


struct DecalEntry
{
#ifdef _DEBUG
	char* m_pDebugName;	// only used in debug builds
#endif
	IMaterial* material;
	int			index;
};

//-----------------------------------------------------------------------------
// Types associated with normal decals
//-----------------------------------------------------------------------------

typedef unsigned short DispDecalFragmentHandle_t;

enum
{
	DISP_DECAL_FRAGMENT_HANDLE_INVALID = (DispDecalFragmentHandle_t)~0
};


class CDispDecalBase
{
public:
	CDispDecalBase(int flags) : m_Flags(flags) {}

	enum
	{
		NODE_BITFIELD_COMPUTED = 0x1,
		DECAL_SHADOW = 0x2,
		NO_INTERSECTION = 0x4,
		FRAGMENTS_COMPUTED = 0x8,	// *non-shadow* fragments
	};

	// Precalculated flags on the nodes telling which nodes this decal can intersect.
	// See CPowerInfo::m_NodeIndexIncrements for a description of how the node tree is
	// walked using this bit vector.
	CBitVec<85>	m_NodeIntersect;	// The number of nodes on a 17x17 is 85.
	// Note: this must be larger if MAX_MAP_DISP_POWER gets larger.

// Number of triangles + verts that got generated for this decal.
	unsigned char	m_Flags;
	unsigned short	m_nVerts;
	unsigned short	m_nTris;
};

class CDispDecal : public CDispDecalBase
{
public:
	CDispDecal() : CDispDecalBase(0) {}

	decal_t* m_pDecal;
	float		m_DecalWorldScale[2];
	Vector		m_TextureSpaceBasis[3];
	float		m_flSize;
	DispDecalFragmentHandle_t	m_FirstFragment;
};

///////////////////////////////
// We gotta find the pointers!!
///////////////////////////////

static IMaterial* (*Draw_DecalMaterial)(int);
static void (*DispInfo_ClearAllTags)(HDISPINFOARRAY);
static void (*R_DecalNode)(mnode_t*, decalinfo_t*);


static decal_t** s_pDecalDestroyList;
static CUtlLinkedList< CDispDecal, DispShadowHandle_t, true >*			s_DispDecals;


///////////////////////////////
///////////////////////////////
///////////////////////////////

// Forward Declarations

void DecalShoot(int textureIndex, int entity, const model_t* model, const Vector& model_origin, const QAngle& model_angles, const Vector& position, const Vector* saxis, int flags);
void DecalColorShoot(int textureIndex, int entity, const model_t* model, const Vector& model_origin, const QAngle& model_angles,	const Vector& position, const Vector* saxis, int flags, const color32& rgbaColor);
void R_DecalShoot(int textureIndex, int entity, const model_t* model, const Vector& position, const Vector* saxis, int flags, const color32& rgbaColor, const Vector* pNormal);
static void R_DecalShoot_(IMaterial* pMaterial, int entity, const model_t* model, const Vector& position, const Vector* saxis, int flags, const color32& rgbaColor, const Vector* pNormal, void* userdata = 0);
//

static unsigned int s_DecalFadeVarCache = 0;
static unsigned int s_DecalFadeTimeVarCache = 0;
static unsigned int s_DecalSecondPassVarCache = 0;

static void R_DecalAddToDestroyList(decal_t* pDecal)
{
	s_pDecalDestroyList = (decal_t**)GetAbsoluteAddress(0x5AC780);
	if (!pDecal->pDestroyList)
	{
		pDecal->pDestroyList = *s_pDecalDestroyList;
		*s_pDecalDestroyList = pDecal;
	}
}


void DecalShootFixed(int textureIndex, int entity,
	const model_t* model, const Vector& model_origin, const QAngle& model_angles,
	const Vector& position, const Vector* saxis, int flags)
{
#ifdef WIN32
	if (IsEngineValidChecksum())
	{
		Draw_DecalMaterial = (IMaterial * (__cdecl*)(int))GetAbsoluteAddress(0xE2AE0);
		DispInfo_ClearAllTags = (void(__cdecl*)(HDISPINFOARRAY))GetAbsoluteAddress(0xE9340);
		R_DecalNode = (void(__cdecl*)(mnode_t*, decalinfo_t*))GetAbsoluteAddress(0x12C830);
		DecalShoot(textureIndex, entity, model, model_origin, model_angles, position, saxis, flags);
	}
	else
#endif // WIN32
	{
		effects->DecalShoot(textureIndex, entity, model, model_origin, model_angles, position, saxis, flags);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : textureIndex - 
//			entity - 
//			modelIndex - 
//			position - 
//			flags - 
//-----------------------------------------------------------------------------
void DecalShoot(int textureIndex, int entity, const model_t* model, const Vector& model_origin, const QAngle& model_angles, const Vector& position, const Vector* saxis, int flags)
{
	color32 white = { 255,255,255,255 };
	DecalColorShoot(textureIndex, entity, model, model_origin, model_angles, position, saxis, flags, white);
}

void DecalColorShoot(int textureIndex, int entity, const model_t* model, const Vector& model_origin, const QAngle& model_angles,
	const Vector& position, const Vector* saxis, int flags, const color32& rgbaColor)
{
	Vector localPosition = position;
	if (entity) 	// Not world?
	{
		matrix3x4_t matrix;
		AngleMatrix(model_angles, model_origin, matrix);
		VectorITransform(position, matrix, localPosition);
	}

	R_DecalShoot(textureIndex, entity, model, localPosition, saxis, flags, rgbaColor, NULL);
}

// Shoots a decal onto the surface of the BSP.  position is the center of the decal in world coords
// This is called from cl_parse.cpp, cl_tent.cpp
void R_DecalShoot(int textureIndex, int entity, const model_t* model, const Vector& position, const Vector* saxis, int flags, const color32& rgbaColor, const Vector* pNormal)
{
	IMaterial* pMaterial = Draw_DecalMaterial(textureIndex);
	R_DecalShoot_(pMaterial, entity, model, position, saxis, flags, rgbaColor, pNormal);
}

// Shoots a decal onto the surface of the BSP.  position is the center of the decal in world coords
static void R_DecalShoot_(IMaterial* pMaterial, int entity, const model_t* model,
	const Vector& position, const Vector* saxis, int flags, const color32& rgbaColor, const Vector* pNormal, void* userdata)
{
	decalinfo_t decalInfo;
	decalInfo.m_flFadeDuration = 0;
	decalInfo.m_flFadeStartTime = 0;

	VectorCopy(position, decalInfo.m_Position);	// Pass position in global

	if (!model || model->type != mod_brush || !pMaterial)
		return;

	decalInfo.m_pModel = (model_t*)model;
	decalInfo.m_pBrush = model->brush.pShared;

	// Deal with the s axis if one was passed in
	if (saxis)
	{
		flags |= FDECAL_USESAXIS;
		VectorCopy(*saxis, decalInfo.m_SAxis);
	}

	// More state used by R_DecalNode()
	decalInfo.m_pMaterial = pMaterial;
	decalInfo.m_pUserData = userdata;

	decalInfo.m_Flags = flags;
	decalInfo.m_Entity = entity;
	decalInfo.m_Size = pMaterial->GetMappingWidth() >> 1;
	if ((int)(pMaterial->GetMappingHeight() >> 1) > decalInfo.m_Size)
		decalInfo.m_Size = pMaterial->GetMappingHeight() >> 1;

	// Compute scale of surface
	// FIXME: cache this?
	IMaterialVar* decalScaleVar;
	bool found;
	decalScaleVar = decalInfo.m_pMaterial->FindVar("$decalScale", &found, false);
	if (found)
	{
		decalInfo.m_scale = 1.0f / decalScaleVar->GetFloatValue();
		decalInfo.m_Size *= decalScaleVar->GetFloatValue();
	}
	else
	{
		decalInfo.m_scale = 1.0f;
	}
	IMaterialVar* pFadeVar = pMaterial->FindVarFast("$decalFadeDuration", &s_DecalFadeVarCache);
	if (pFadeVar)
	{
		decalInfo.m_flFadeDuration = pFadeVar->GetFloatValue();
		pFadeVar = pMaterial->FindVarFast("$decalFadeTime", &s_DecalFadeTimeVarCache);
		decalInfo.m_flFadeStartTime = pFadeVar ? pFadeVar->GetFloatValue() : 0.0f;
	}

	IMaterialVar* pSecondPassVar = pMaterial->FindVarFast("$decalSecondPass", &s_DecalSecondPassVarCache);
	if (pSecondPassVar)
	{
		decalInfo.m_Flags |= FDECAL_SECONDPASS;
	}

	// compute the decal dimensions in world space
	decalInfo.m_decalWidth = pMaterial->GetMappingWidth() / decalInfo.m_scale;
	decalInfo.m_decalHeight = pMaterial->GetMappingHeight() / decalInfo.m_scale;
	decalInfo.m_Color = rgbaColor;
	decalInfo.m_pNormal = pNormal;
	decalInfo.m_aApplySurfs.Purge();

	// Clear the displacement tags because we use them in R_DecalNode.
	DispInfo_ClearAllTags(decalInfo.m_pBrush->hDispInfos);

	mnode_t* pnodes = decalInfo.m_pBrush->nodes + decalInfo.m_pModel->brush.firstnode;
	R_DecalNode(pnodes, &decalInfo);
}
//-----------------------------------------------------------------------------
// Updates all decals, returns true if the decal should be retired
//-----------------------------------------------------------------------------

bool DecalUpdate(decal_t* pDecal)
{
	// retire the decal if it's time has come
	if (pDecal->fadeDuration > 0)
	{
		return (gpGlobals->curtime >= pDecal->fadeStartTime + pDecal->fadeDuration);
	}
	return false;
}


void UpdateDispDecals()
{
#ifdef WIN32
	if (!IsEngineValidChecksum())
	{
		return;
	}
	s_DispDecals = (CUtlLinkedList< CDispDecal, DispShadowHandle_t, true >*)GetAbsoluteAddress(0x3B60F4);
	int iElement = s_DispDecals->Head();
	while (iElement != s_DispDecals->InvalidIndex())
	{
		CDispDecal* pDispDecal = &s_DispDecals->Element(iElement);
		decal_t* pDecal = pDispDecal->m_pDecal;
		iElement = s_DispDecals->Next(iElement);

		// Dynamic decals - fading.
		if (pDecal->flags & FDECAL_DYNAMIC)
		{
			float localClientTime = gpGlobals->curtime;
			float flFadeValue;

			// Negative fadeDuration value means to fade in
			if (pDecal->fadeDuration < 0)
			{
				flFadeValue = -(localClientTime - pDecal->fadeStartTime) / pDecal->fadeDuration;
			}
			else
			{
				flFadeValue = 1.0 - (localClientTime - pDecal->fadeStartTime) / pDecal->fadeDuration;
			}

			flFadeValue = clamp(flFadeValue, 0.0f, 1.0f);

			// Set base color.
			color32 color = { pDecal->color.r, pDecal->color.g, pDecal->color.b, pDecal->color.a };
			color.a = (byte)(color.a * flFadeValue);
			pDecal->color = color;
		}

		if (DecalUpdate(pDecal))
		{
			R_DecalAddToDestroyList(pDecal);
			continue;
		}
	}
#endif // WIN32
}