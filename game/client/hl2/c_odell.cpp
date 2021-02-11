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
class C_Odell : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS( C_Odell, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();

					C_Odell();
	virtual			~C_Odell();

private:
	C_Odell( const C_Odell & ); // not defined, not accessible
};

IMPLEMENT_CLIENTCLASS_DT(C_Odell, DT_NPC_Odell, CNPC_Odell)
END_RECV_TABLE()

C_Odell::C_Odell()
{
}


C_Odell::~C_Odell()
{
}


