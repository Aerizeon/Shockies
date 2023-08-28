#ifndef _ShockiesRemote_h
#define _ShockiesRemote_h
#include <functional>

class AsyncSSLClient;

typedef std::function<void(char *, size_t)> CommandHandler;
typedef std::function<void(void)> ConnectionHandler;

#define UUID_STR_LEN 37

class ShockiesRemote
{
public:
    ShockiesRemote(const char *uuid);
    void connect(const char *addr, unsigned int port);
    void sendCommand(const char *command);
    void onCommand(CommandHandler handler);
    void onConnected(ConnectionHandler handler);
    void onDisconnected(ConnectionHandler handler);

private:
    char dataBuf[256];
    const char* deviceUuid = nullptr;
    const char* addr = nullptr;
    unsigned int port = 0;


    CommandHandler _commandHandler = nullptr;
    ConnectionHandler _connectedHandler = nullptr;
    ConnectionHandler _disconnectedHandler = nullptr;

    void connected(AsyncSSLClient *client);
    void disconnected(AsyncSSLClient *client);
    void data(AsyncSSLClient *client, void *data, int len);
};

#endif