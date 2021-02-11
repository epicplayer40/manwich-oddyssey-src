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
class C_Conscript_new : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS(C_Conscript_new, C_AI_BaseNPC);
	DECLARE_CLIENTCLASS();

	C_Conscript_new();
	virtual			~C_Conscript_new();

private:
	C_Conscript_new(const C_Conscript_new &); // not defined, not accessible
};

IMPLEMENT_CLIENTCLASS_DT(C_Conscript_new, DT_NPC_Conscript_new, CNPC_Conscript_new)
END_RECV_TABLE()

C_Conscript_new::C_Conscript_new()
{
}


C_Conscript_new::~C_Conscript_new()
{
}


