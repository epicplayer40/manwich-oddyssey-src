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
class C_Vance : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS( C_Vance, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();

					C_Vance();
	virtual			~C_Vance();

private:
	C_Vance( const C_Vance & ); // not defined, not accessible
};

IMPLEMENT_CLIENTCLASS_DT(C_Vance, DT_NPC_Vance, CNPC_Vance)
END_RECV_TABLE()

C_Vance::C_Vance()
{
}


C_Vance::~C_Vance()
{
}


