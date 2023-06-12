#include <WebSocketClient.h>
#include <stdio.h>
#include <AsyncWebSocket.h>

WebSocketClient::WebSocketClient(const std::string host, unsigned short port, std::string path)
    : _client(),
      _currentFrame(),
      //_controlQueue(),
      _messageQueue(),
      _frameInfo()
{
    _host = host;
    _path = path;
    _client.onConnect([](void *r, AsyncClient *c)
                      { ((WebSocketClient *)r)->OnConnect(); },
                      this);
    _client.onDisconnect([](void *r, AsyncClient *c)
                         { ((WebSocketClient *)r)->OnDisconnect(); },
                         this);
    _client.onAck([](void *r, AsyncClient *c, size_t len, uint32_t time)
                  { ((WebSocketClient *)r)->OnAck(len, time); },
                  this);
    _client.onData([](void *r, AsyncClient *c, void *buf, size_t len)
                   { ((WebSocketClient *)r)->OnData(buf, len); },
                   this);
    _client.onPoll([](void *r, AsyncClient *c)
                   { ((WebSocketClient *)r)->OnPoll(); },
                   this);
    Serial.println("Attempting connect");
    _client.connect(host.c_str(), port);
}

void WebSocketClient::OnConnect()
{
    Serial.println("Connected, sending data");
    std::string upgrade =
        "GET " + _path + " HTTP/1.1\r\n" +
        "Host: " + _host + "\r\n" +
        "User-Agent: Shockies-device\r\n" +
        "Accept: */*\r\n" +
        "Pragma: no-cache\r\n" +
        "Connection: keep-alive, Upgrade\r\n" +
        "Upgrade: WebSocket\r\n" +
        "Origin: shockies.local\r\n" +
        "Sec-WebSocket-Version: 13\r\n" +
        "Sec-WebSocket-Key: POfbDfkEeGsOxs/iWs8YtA==\r\n\r\n";
    _client.write(upgrade.c_str(), upgrade.length());
    _currentState = WebsocketState::Upgrade;
}
void WebSocketClient::OnDisconnect()
{
    Serial.println("Disconnected");
}
void WebSocketClient::OnAck(size_t len, uint32_t time)
{
    if (len && !_messageQueue.empty())
    {
        _messageQueue.front()->ack(len, time);
    }
    ProcessQueues();
    Serial.println("Ack");
}

void WebSocketClient::OnData(void *buf, size_t len)
{
    if (_currentState == WebsocketState::Upgrade)
    {
        char *str = (char *)buf;
        Serial.println("UPG");
        int responseCode = 0;
        if (sscanf(str, "HTTP/%*s %i %*s\r\n", &responseCode) != 1 || responseCode != 101)
        {
            Serial.println("Failed to upgrade to websocket");
            _client.close(true);
            return;
        }
        Serial.println("RCV ResponseCode");
        char *body = strstr(str, "\r\n\r\n");
        if (!body)
        {
            Serial.println("Failed to find end of header");
            return;
        }
        body += 4;
        _currentState = WebsocketState::Ready;
        if (body - str == len)
        {
            Serial.println("No data after header");
        }
        SendText("Hi");
    }
    else if (_currentState == WebsocketState::Ready)
    {
        uint8_t *data = (uint8_t *)buf;
        uint8_t state = 0;
        while (len > 0)
        {
            if (!state)
            {
                const uint8_t *fdata = (uint8_t *)data;
                _frameInfo.index = 0;
                _frameInfo.final = (fdata[0] & 0x80) != 0;
                _frameInfo.opcode = fdata[0] & 0x0F;
                _frameInfo.masked = (fdata[1] & 0x80) != 0;
                _frameInfo.len = fdata[1] & 0x7F;
                data += 2;
                len -= 2;
                if (_frameInfo.len == 126)
                {
                    _frameInfo.len = fdata[3] | (uint16_t)(fdata[2]) << 8;
                    data += 2;
                    len -= 2;
                }
                else if (_frameInfo.len == 127)
                {
                    _frameInfo.len = fdata[9] | (uint16_t)(fdata[8]) << 8 | (uint32_t)(fdata[7]) << 16 | (uint32_t)(fdata[6]) << 24 | (uint64_t)(fdata[5]) << 32 | (uint64_t)(fdata[4]) << 40 | (uint64_t)(fdata[3]) << 48 | (uint64_t)(fdata[2]) << 56;
                    data += 8;
                    len -= 8;
                }

                if (_frameInfo.masked)
                {
                    memcpy(_frameInfo.mask, data, 4);
                    data += 4;
                    len -= 4;
                }
            }
            const size_t datalen = std::min((size_t)(_frameInfo.len - _frameInfo.index), len);
            if (_frameInfo.masked)
            {
                for (size_t i = 0; i < datalen; i++)
                    data[i] ^= _frameInfo.mask[(_frameInfo.index + i) % 4];
            }

            if ((datalen + _frameInfo.index) < _frameInfo.len)
            {
                state = 1;
                if (_frameInfo.index == 0)
                {
                    if (_frameInfo.opcode)
                    {
                        _frameInfo.message_opcode = _frameInfo.opcode;
                        _frameInfo.num = 0;
                    }
                    else
                        _frameInfo.num += 1;
                }
                //_server->_handleEvent(this, WS_EVT_DATA, (void *)&_frameInfo, (uint8_t *)data, datalen);
                Serial.printf("RCV DATA: %s\r\n", data);
                _frameInfo.index += datalen;
            }
            else if ((datalen + _frameInfo.index) == _frameInfo.len)
            {
                state = 0;
                if (_frameInfo.opcode == WS_DISCONNECT)
                {
                    if (datalen)
                    {
                        uint16_t reasonCode = (uint16_t)(data[0] << 8) + data[1];
                        char *reasonString = (char *)(data + 2);
                        if (reasonCode > 1001)
                        {
                            //_server->_handleEvent(this, WS_EVT_ERROR, (void *)&reasonCode, (uint8_t *)reasonString, strlen(reasonString));
                        }
                    }
                    if (_currentState == WebsocketState::Disconnecting)
                    {
                        _currentState = WebsocketState::Disconnected;
                        _client.close(true);
                    }
                    else
                    {
                        _currentState = WebsocketState::Disconnecting;
                        _client.ackLater();
                        //_queueControl(new AsyncWebSocketControl(WS_DISCONNECT, data, datalen));
                    }
                }
                else if (_frameInfo.opcode == WS_PING)
                {
                    Serial.println("PING");
                }
                else if (_frameInfo.opcode == WS_PONG)
                {
                    Serial.println("PONG");
                }
                else if (_frameInfo.opcode < 8)
                { // continuation or text/binary frame

                    if (_frameInfo.opcode == WS_TEXT)
                    {
                        Serial.printf("Text: \"%s\"\r\n", data);
                    }
                    else if (_frameInfo.opcode == WS_BINARY)
                    {
                        Serial.printf("Binary Len:%d", datalen);
                    }
                    else if (_frameInfo.opcode == WS_CONTINUATION)
                    {
                        Serial.println("Continuation");
                    }
                    else
                    {
                        Serial.printf("Unknown opcode: %d\r\n", _frameInfo.opcode);
                    }
                }
            }
            data += datalen;
            len -= datalen;
        }
    }
    else
    {
    }
}
void WebSocketClient::OnPoll()
{
    Serial.println("Poll");
    if (_client.canSend() && (/*!_controlQueue.empty() ||*/ !_messageQueue.empty()))
    {
        ProcessQueues();
    }
    /*else if (_keepAlivePeriod > 0 && _controlQueue.isEmpty() && _messageQueue.isEmpty() && (millis() - _lastMessageTime) >= _keepAlivePeriod)
    {
        ping((uint8_t *)AWSC_PING_PAYLOAD, AWSC_PING_PAYLOAD_LEN);
    }*/
    SendText("Hi");
}

void WebSocketClient::ProcessQueues()
{
    Serial.println("ProcessQueue");
    while (!_messageQueue.empty() && _messageQueue.front()->finished())
    {
        Serial.println("Remove Complete");
        _messageQueue.remove(_messageQueue.front());
    }

    /*if (!_controlQueue.empty() && (_messageQueue.empty() || _messageQueue.front()->betweenFrames()) && SendFrameWindow() > (size_t)(_controlQueue.front()->len() - 1))
    {
        _controlQueue.front()->send(&_client);
    }*/
    if (!_messageQueue.empty() && _messageQueue.front()->betweenFrames() && SendFrameWindow())
    {
        Serial.println("Send Message");
        _messageQueue.front()->send(&_client);
    }
}

size_t WebSocketClient::SendFrameWindow()
{
    if (!_client.canSend())
        return 0;
    size_t space = _client.space();
    if (space < 9)
        return 0;
    return space - 8;
}

void WebSocketClient::QueueMessage(unique_ptr<AsyncWebSocketMessage> dataMessage)
{
    Serial.println("StartQueueMessage");
    if (dataMessage == NULL)
    {
        Serial.println("Datamessage unset");
        return;
    }
    if (_currentState != WebsocketState::Ready)
    {
        Serial.println("Not ready");
        return;
    }
    if (_messageQueue.size() >= WS_MAX_QUEUED_MESSAGES)
    {
        Serial.println("Too many messages");
        ets_printf("ERROR: Too many messages queued\n");
    }
    else
    {
        Serial.println("Adding to queue");
        _messageQueue.push_back(std::move(dataMessage));
    }
    if (_client.canSend())
    {
        Serial.println("ProcessQueue");
        ProcessQueues();
    }
    else
    {
        Serial.println(_client.stateToString());
    }
}
void WebSocketClient::QueueControl(unique_ptr<AsyncWebSocketControl> controlMessage)
{
}
void WebSocketClient::SendText(const char *message, size_t len)
{
    QueueMessage(std::unique_ptr<AsyncWebSocketMessage>(new AsyncWebSocketBasicMessage(message, len)));
}

void WebSocketClient::SendText(const char *message)
{
    SendText(message, strlen(message));
}