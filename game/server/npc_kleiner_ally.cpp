class CNPC_KleinerCombat : public CNPC_Citizen
{
public:
    DECLARE_CLASS( CNPC_KleinerCombat, CNPC_Citizen );
    DECLARE_SERVERCLASS();

    virtual void Precache();
    void    Spawn( void );
    Class_T Classify( void );
};


LINK_ENTITY_TO_CLASS( npc_kleiner_ally, CNPC_KleinerCombat );

IMPLEMENT_SERVERCLASS_ST(CNPC_KleinerCombat, DT_NPC_KleinerCombat)
END_SEND_TABLE()


void CNPC_KleinerCombat::Precache(void)
{
    m_Type = CT_UNIQUE;

    CNPC_Citizen::Precache();

    SetModelName(AllocPooledString("models/kleiner.mdl"));
}

void CNPC_KleinerCombat::Spawn( void )
{
    Precache();

    CNPC_Citizen::Spawn();

    m_iHealth = sk_kleiner_a_health.GetInt();
}

Class_T    CNPC_KleinerCombat::Classify( void )
{
    return    CLASS_PLAYER_ALLY;
}