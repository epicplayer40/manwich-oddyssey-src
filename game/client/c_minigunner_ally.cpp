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
class C_Militiagunner : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS( C_Militiagunner, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();

					C_Militiagunner();
	virtual			~C_Militiagunner();

private:
	C_Militiagunner( const C_Militiagunner & ); // not defined, not accessible
};

IMPLEMENT_CLIENTCLASS_DT(C_Militiagunner, DT_MONSTER_Militiagunner, CNPC_Militiagunner)
END_RECV_TABLE()

C_Militiagunner::C_Militiagunner()
{
}


C_Militiagunner::~C_Militiagunner()
{
}


