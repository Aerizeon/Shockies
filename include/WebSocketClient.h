#ifndef _WebSocketClient_h
#define _WebSocketClient_h
#include <AsyncWebSocket.h>
#include <list>
using std::list;
using std::unique_ptr;

enum WebsocketState 
{
    None,
    Init,
    Upgrade,
    Ready,
    Disconnecting,
    Disconnected
};

class WebSocketClient
{
public:
    WebSocketClient(std::string host, unsigned short port, std::string path);

private:
    AsyncClient _client;
    AwsFrameInfo _frameInfo;
    std::string _path;
    std::string _host;
    WebsocketState _currentState = WebsocketState::None;
    AwsFrameInfo _currentFrame;
    //list<unique_ptr<AsyncWebSocketControl>> _controlQueue;
    list<unique_ptr<AsyncWebSocketMessage>> _messageQueue;
    void OnConnect();
    void OnDisconnect();
    void OnAck(size_t len, uint32_t time);
    void OnData(void *buf, size_t len);
    void OnPoll();
    void ProcessQueues();
    size_t SendFrameWindow();
    void QueueMessage(unique_ptr<AsyncWebSocketMessage> dataMessage);
    void QueueControl(unique_ptr<AsyncWebSocketControl> controlMessage);
    void SendText(const char *message, size_t len);
    void SendText(const char *message);
};

#endif