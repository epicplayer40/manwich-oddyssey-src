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
class C_Animal : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS( C_Animal, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();

					C_Animal();
	virtual			~C_Animal();

private:
	C_Animal( const C_Animal & ); // not defined, not accessible
};

IMPLEMENT_CLIENTCLASS_DT(C_Animal, DT_NPC_Animal, CNPC_Animal)
END_RECV_TABLE()

C_Animal::C_Animal()
{
}


C_Animal::~C_Animal()
{
}


