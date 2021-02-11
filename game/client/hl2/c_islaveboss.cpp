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
class C_ISlaveBoss : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS( C_ISlaveBoss, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();

					C_ISlaveBoss();
	virtual			~C_ISlaveBoss();

private:
	C_ISlaveBoss( const C_ISlaveBoss & ); // not defined, not accessible
};



IMPLEMENT_CLIENTCLASS_DT(C_ISlaveBoss, DT_NPC_ISlaveBoss, CNPC_ISlaveBoss)
END_RECV_TABLE()

C_ISlaveBoss::C_ISlaveBoss()
{
}


C_ISlaveBoss::~C_ISlaveBoss()
{
}


