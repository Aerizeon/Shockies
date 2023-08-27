#ifndef _ShockiesRemote_h
#define _ShockiesRemote_h
#include <functional>

class AsyncSSLClient;

typedef std::function<void(char*, size_t)> CommandHandler;
#define UUID_STR_LEN 37

class ShockiesRemote {
public:
 ShockiesRemote(char* uuid);
 void connect(const char* addr, unsigned int port);
 void onCommand(CommandHandler handler);

 private:
 char*  deviceUuid = nullptr;
 CommandHandler _commandHandler = nullptr;
 void onConnect(AsyncSSLClient *client);
 void onData(AsyncSSLClient *client, void *data, int len);
};

#endif