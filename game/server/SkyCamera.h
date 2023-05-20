//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Resource collection entity
//
// $NoKeywords: $
//=============================================================================//

#ifndef SKYCAMERA_H
#define SKYCAMERA_H

#ifdef _WIN32
#pragma once
#endif

class CSkyCamera;

//=============================================================================
//
// Sky Camera Class
//
class CSkyCamera : public CBaseEntity
{
	DECLARE_CLASS( CSkyCamera, CBaseEntity);

public:

	DECLARE_DATADESC();

	CSkyCamera();
	~CSkyCamera();
	virtual void Spawn( void );
	virtual void Activate();
	int UpdateTransmitState();	// always send to all clients

public:
	sky3dparams_t	m_skyboxData;
	bool			m_bUseAngles;
	CSkyCamera		*m_pNext;
};


//-----------------------------------------------------------------------------
// Retrives the current skycamera
//-----------------------------------------------------------------------------
CSkyCamera*		GetCurrentSkyCamera();
CSkyCamera*		GetSkyCameraList();


#endif // SKYCAMERA_H
