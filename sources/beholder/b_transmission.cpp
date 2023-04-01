#include "../include/transmission.h"
#include "b_transmission.h"

static u8 getKnownClientID(ClientList* clientList, std::string newClient) 
{
    u8 result = 0;
    i32 length = arrayLength(clientList->clients);
    
    for(u8 i = 0; i < length; i++) 
    {
        if(clientList->clients[i].ip == newClient) 
        {
            result = clientList->clients[i].id;
        }
    }
    
    return result;
}

static void receiveHandshake(MemoryArena* arena,
                             TransmissionState* state,
                             std::string portNumber, ClientList* clientList, u8 estimatedClientCout)
{
    printf("Entering handshake routine \n");
    openReplyTransmissionChannel(state, portNumber);
    
    //NOTE(dave): Wait until response with ID is returned by the server
    while (clientList->clientCount < estimatedClientCout) {
        Message msg = receiveMessageBlocking(arena, state);
        
        if (msg.header.type == MessageType_HelloReq)
        {
            if (msg.data) 
            {
                u8 clientID = getKnownClientID(clientList, std::string((char*)msg.data));
                if(clientID != 0) 
                {
                    sendMessage(arena,
                                state, 
                                MessageType_HelloRep,
                                &clientID, 
                                sizeof(clientID),
                                0);  
                    
                    printf("Know Spotter %i reregistered\n", clientID);               
                }
                else 
                {
                    Client* client = &clientList->clients[clientList->clientCount];
                    client->id = clientList->clientCount + 1;
                    client->ip = std::string((char*)msg.data);
                    clientList->clientCount++;
                    
                    sendMessage(arena,
                                state, 
                                MessageType_HelloRep,
                                &clientList->clientCount, 
                                sizeof(clientList->clientCount),
                                0);
                    
                    printf("New Spotter %i registered with ip %s \n", client->id, client->ip.c_str());
                }
            }
        }
    }
    
    closeTransmissionChannel(state);
    printf("Exiting handshake routine \n");
}
