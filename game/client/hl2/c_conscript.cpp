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
class C_Conscript : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS( C_Conscript, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();

					C_Conscript();
	virtual			~C_Conscript();

private:
	C_Conscript( const C_Conscript & ); // not defined, not accessible
};

IMPLEMENT_CLIENTCLASS_DT(C_Conscript, DT_NPC_Conscript, CNPC_Conscript)
END_RECV_TABLE()

C_Conscript::C_Conscript()
{
}


C_Conscript::~C_Conscript()
{
}


