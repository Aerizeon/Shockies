#include <ShockiesRemote.h>
#include <AsyncTCP_SSL.h>

AsyncSSLClient *client = nullptr;

ShockiesRemote::ShockiesRemote(char* uuid)
{
    deviceUuid = uuid;
    client = new AsyncSSLClient();
}

void ShockiesRemote::connect(const char *addr, unsigned int port)
{
    client->connect(addr, port, true);
    client->onConnect([](void *sr, AsyncSSLClient *c)
                      { ShockiesRemote *remote = (ShockiesRemote*)sr; remote->onConnect(c); },
                      this);
    client->onData([](void *sr, AsyncSSLClient *c, void *d, size_t l)
                   { ShockiesRemote *remote = (ShockiesRemote*)sr; remote->onData(c, d, l); },
                   this);
}

 void ShockiesRemote::onCommand(CommandHandler handler)
 {
    _commandHandler = handler;
 }

void ShockiesRemote::onConnect(AsyncSSLClient *c)
{
    char dataBuf[UUID_STR_LEN + 1];
    strncpy(dataBuf + 1, deviceUuid, UUID_STR_LEN);
    dataBuf[0] = UUID_STR_LEN;

    c->setNoDelay(true);
    c->write(dataBuf, sizeof(dataBuf));
    Serial.write("Connected!");
}

void ShockiesRemote::onData(AsyncSSLClient *client, void *data, int len)
{
    char *str = (char *)data;

    if (str[0] == len - 1 && len > 0)
    {
        if(_commandHandler != nullptr)
        {
            _commandHandler(str + 1, len - 1);
        }
    }
    else
    {
        Serial.println(len);
    }
}