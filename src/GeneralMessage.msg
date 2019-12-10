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

    int triggerMessageId[];
    double triggerMessageTime[];

    int nextHop[];
    int gateInfo[];
}
