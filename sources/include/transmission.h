#ifndef TRANSMISSION_H

#ifndef INTERFACE
#define INTERFACE "eth0"
#endif

enum MessageType
{
    MessageType_None,
    MessageType_HelloReq,
    MessageType_HelloRep,
    MessageType_Payload,
    MessageType_Command,
    
    // Debug message types
    MessageType_DebugCameraPose,
    MessageType_DebugRays,
    MessageType_DebugFrame,
    MessageType_DebugIntersections
};

enum CommandType
{
    CommandType_None,
    CommandType_GrabFrame,
    CommandType_EstimatePose,
    CommandType_SendGrayscale,
    CommandType_SendBinarized,
    CommandType_SaveRaysToFile,
    CommandType_SaveFramesToFile,
    CommandType_MatchModel,
    CommandType_BinarizationThreshold,
    CommandType_NoFrames,
    CommandType_StopSystem,
    CommandType_StartDebugging,
    CommandType_StopDebugging
};

struct MessageHeader
{
    MessageType type;
    u8 spotterID;
    u32 payloadSize; //Message size in byte
    // TODO(jan): maybe add a spotterMessageHeader and a mimicMessageHeader
    // or something, to prevent transmission of useless data
};

struct HelloPayload
{
    char ip[16];
    i32 frameWidth;
    i32 frameHeight;
};

struct DebugFrameInfo
{
    i32 width;
    i32 height;
    i32 bytesPerPixel;
};

struct Message
{
    MessageHeader header;
    void* data;
};

enum TransmissionStatus
{
    TransmissionStatus_None,
    TransmissionStatus_Ok
};

enum SendBufferType
{
    SendBufferType_None,
    SendBufferType_Grayscale,
    SendBufferType_Binarized
};

struct TransmissionState
{
    TransmissionStatus status;
    u8 spotterID;
    
    // 0MQ Shizzle
    void* context;
    void* socket;   //ZMQ Socket
    
    // TODO(jan): put somewhere smarter
    SendBufferType sendBufferType;
};

inline static std::string getLocalIP()
{   
    std::string result;
    int fd;
    struct ifreq ifr;
    
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    
    /* I want to get an IPv4 IP address */
    ifr.ifr_addr.sa_family = AF_INET;
    
    //NOTE(dave): String may vary based on OS running (eth0 on raspberry)
    strncpy(ifr.ifr_name, INTERFACE, IFNAMSIZ-1);
    
    ioctl(fd, SIOCGIFADDR, &ifr);
    
    close(fd);
    
    result = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
    return result;
}

inline static void initTransmissionState(TransmissionState* state)
{
    state->status = TransmissionStatus_Ok;
    state->context = zmq_ctx_new();
}

// NOTE(jan): keepOnlyLastMessage: this option deletes all messages queued up for 
// sending except for the last one
inline static void openPushTransmissionChannel(TransmissionState* state, 
                                               std::string serverIp,
                                               std::string portNumber,
                                               bool32 keepOnlyLastMessage = 1)
{
    state->socket = zmq_socket(state->context, ZMQ_PUSH);
    /*zmq_setsockopt(state->socket, 
                   ZMQ_CONFLATE, 
                   &keepOnlyLastMessage,
                   sizeof(keepOnlyLastMessage));*/
    std::string serverAddress = "tcp://" + serverIp + ":" + portNumber;
    zmq_connect(state->socket, serverAddress.c_str());
}

inline static void openPullTransmissionChannel(TransmissionState* state,
                                               std::string portNumber)
{
    state->socket = zmq_socket(state->context, ZMQ_PULL);
    std::string address = "tcp://*:" + portNumber;
    i32 keepOnlyLastMessage = 1;
    /*zmq_setsockopt(state->socket, 
                   ZMQ_CONFLATE, 
                   &keepOnlyLastMessage,
                   sizeof(keepOnlyLastMessage));*/
    
    zmq_bind(state->socket, address.c_str());
}

inline static void openSubscriberTransmissionChannel(TransmissionState* state, 
                                                     std::string serverIp,
                                                     std::string portNumber)
{
    state->socket = zmq_socket(state->context, ZMQ_SUB);
    std::string serverAddress = "tcp://" + serverIp + ":" + portNumber;
    zmq_connect(state->socket, serverAddress.c_str());
    zmq_setsockopt(state->socket, 
                   ZMQ_SUBSCRIBE, 
                   0,
                   0);
}

inline static void openPublisherTransmissionChannel(TransmissionState* state,
                                                    std::string portNumber)
{
    state->socket = zmq_socket(state->context, ZMQ_PUB);
    std::string address = "tcp://*:" + portNumber;
    
    zmq_bind(state->socket, address.c_str());
}

inline static void openRequestTransmissionChannel(TransmissionState* state, 
                                                  std::string serverIp,
                                                  std::string portNumber)
{
    state->socket = zmq_socket(state->context, ZMQ_REQ);
    
    std::string serverAddress = "tcp://" + serverIp + ":" + portNumber;
    zmq_connect(state->socket, serverAddress.c_str());
}

inline static void openReplyTransmissionChannel(TransmissionState* state, 
                                                std::string portNumber)
{
    state->socket = zmq_socket(state->context, ZMQ_REP);
    
    std::string serverAddress = "tcp://*:" + portNumber;
    zmq_bind(state->socket, serverAddress.c_str());
}

inline static void closeTransmissionChannel(TransmissionState* state)
{
    if (state->status == TransmissionStatus_Ok)
    {
        zmq_close(state->socket); 
    }
}

inline static void destroyTransmissionState(TransmissionState* state)
{
    zmq_ctx_destroy(state->context);
}

inline static void sendMessage(MemoryArena* arena,
                               TransmissionState* transmissionState,
                               MessageType type,
                               void* payload,
                               u32 payloadSize,
                               u8 spotterId)
{
    MessageHeader* header = pushStruct(arena, MessageHeader);
    header->type = type;
    header->spotterID = spotterId;
    header->payloadSize = payloadSize;
    
    memcpy((u8*)header + sizeof(MessageHeader), payload, payloadSize);
    
    i32 bytesSent = zmq_send(transmissionState->socket, 
                             header, 
                             payloadSize + sizeof(MessageHeader),
                             0);
    
    if (bytesSent == -1)
    {
        printf("Error when sending message\n");
    }
    else if (bytesSent != (i32)(sizeof(MessageHeader) + payloadSize))
    {
        printf("Not all of message sent\n");
    }
}

static Message receiveMessageBlocking(MemoryArena* arena,
                                      TransmissionState* state) 
{
    zmq_msg_t msg;
    zmq_msg_init(&msg);
    Message result = {};
    i32 msgSize = zmq_recvmsg(state->socket, &msg, ZMQ_DONTWAIT);
    
    if (msgSize == -1)
    {
        //printf("Error while receiving message!");
    }
    else
    {
        u8* data = (u8*)zmq_msg_data(&msg);
        
        MessageHeader* header = (MessageHeader*)data;
        data += sizeof(MessageHeader);
        
        result.header = *header;
        result.data = pushSize(arena, header->payloadSize);
        
        memcpy(result.data, data, header->payloadSize);
    }
    
    return result;
}

static bool32 receiveMessageNonBlocking(MemoryArena* arena,
                                        TransmissionState* state,
                                        Message* message) 
{
    zmq_msg_t msg;
    zmq_msg_init(&msg);
    bool32 result = 0;
    i32 msgSize = zmq_recvmsg(state->socket, &msg, ZMQ_DONTWAIT);
    
    if (msgSize == -1)
    {
        result = 0;
    }
    else
    {
        u8* data = (u8*)zmq_msg_data(&msg);
        
        MessageHeader* header = (MessageHeader*)data;
        data += sizeof(MessageHeader);
        
        message->header = *header;
        message->data = pushSize(arena, header->payloadSize);
        
        memcpy(message->data, data, header->payloadSize);
        result = 1;
    }
    
    zmq_msg_close(&msg);
    
    return result;
}

#define TRANSMISSION_H
#endif
