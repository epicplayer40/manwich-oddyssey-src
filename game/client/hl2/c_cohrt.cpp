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
class C_Cohrt : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS( C_Cohrt, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();

					C_Cohrt();
	virtual			~C_Cohrt();

private:
	C_Cohrt( const C_Cohrt & ); // not defined, not accessible
};

IMPLEMENT_CLIENTCLASS_DT(C_Cohrt, DT_NPC_Cohrt, CNPC_Cohrt)
END_RECV_TABLE()

C_Cohrt::C_Cohrt()
{
}


C_Cohrt::~C_Cohrt()
{
}


