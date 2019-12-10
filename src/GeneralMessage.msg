message GeneralMessage
{
    int type;
    int source;
    int destination;
    int port;

    int fRuleSource;
    int fRuleDestination;
    int fRulePort;
    int fRuleNextDestination;

    int nextHop[];
    int gateInfo[];
}
