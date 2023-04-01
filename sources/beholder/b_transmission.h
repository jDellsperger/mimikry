#ifndef B_TRANSMISSION_H

#define MAX_CLIENT_COUNT 6

struct Client
{
    u8 id;
    std::string ip;
};

struct ClientList
{
    Client clients[MAX_CLIENT_COUNT];
    u8 clientCount;
};

#define B_TRANSMISSION_H
#endif