#ifndef __SERVER_SESSION_H__
#define __SERVER_SESSION_H__

#include "common.h"
#include "network_common.h"
#include "tcp_connection.h"

// ServerConnection 用于隐藏底层Connection的部分接口
// 提供基本方法给上层对连接进行操作

class NetworkMessage;

class ServerConnection
{
public:
    ServerConnection(TcpConnection* connection);
    virtual ~ServerConnection();

public:
    void sendMessage(uint16 opcode, NetworkMessage& message);
    void close(const ShutdownReason& reason = SR_SERVICE_CLOSE_INITIATIVE);
    uint32 sequence() const;

private:
    TcpConnection* _connection;
};

#endif