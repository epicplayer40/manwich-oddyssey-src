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
class C_KleinerCombat : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS( C_KleinerCombat, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();

					C_KleinerCombat();
	virtual			~C_KleinerCombat();

private:
	C_KleinerCombat( const C_KleinerCombat & ); // not defined, not accessible
};

IMPLEMENT_CLIENTCLASS_DT(C_KleinerCombat, DT_NPC_KleinerCombat, CNPC_KleinerCombat)
END_RECV_TABLE()

C_KleinerCombat::C_KleinerCombat()
{
}


C_KleinerCombat::~C_KleinerCombat()
{
}


