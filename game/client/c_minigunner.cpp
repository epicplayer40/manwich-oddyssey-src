//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_ai_basenpc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_Minigunner : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS( C_Minigunner, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();

					C_Minigunner();
	virtual			~C_Minigunner();

private:
	C_Minigunner( const C_Minigunner & ); // not defined, not accessible
};

IMPLEMENT_CLIENTCLASS_DT(C_Minigunner, DT_MONSTER_Minigunner, CNPC_Minigunner)
END_RECV_TABLE()

C_Minigunner::C_Minigunner()
{
}


C_Minigunner::~C_Minigunner()
{
}


