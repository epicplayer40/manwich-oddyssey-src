#include "cbase.h"
#include "c_ai_basenpc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



class C_Advisor : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS( C_Advisor, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();

					C_Advisor();
	virtual			~C_Advisor();

private:
	C_Advisor( const C_Advisor & ); // not defined, not accessible
};

IMPLEMENT_CLIENTCLASS_DT(C_Advisor, DT_NPC_Advisor, CNPC_Advisor)
END_RECV_TABLE()

C_Advisor::C_Advisor()
{
}


C_Advisor::~C_Advisor()
{
}

