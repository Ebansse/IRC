#include "../includes/Client.hpp"
#include "../includes/Channel.hpp"

Client::Client(int fd, const std::string& ip, int port)
{
    if (fd < 0)
        return ;
    _fd = fd;
    _ip = ip;
    _port = port;
    _hostname = ip;
    _passOk = false;
    _nickOk = false;
    _userOk = false;
    _registered = false;
}

Client::~Client() {}

/* ================= COMMANDS ================= */


void Client::appendToSendBuffer(const std::string& data)
{
    _sendBuffer += data;
}

void Client::appendToRecvBuffer(const std::string& data)
{
    _recvBuffer += data;

    if (_recvBuffer.size() > 4096)
        _recvBuffer.clear();
}

bool Client::hasCompleteCommand() const
{
    return _recvBuffer.find("\r\n") != std::string::npos;
}

std::string Client::extractCommand()
{
    size_t pos = _recvBuffer.find("\r\n");

    if (pos == std::string::npos)
        return "";
    
    std::string command = _recvBuffer.substr(0, pos);
    _recvBuffer.erase(0, pos + 2);

    return command;
}

void Client::clearSentData(size_t bytes)
{
    if (bytes >= _sendBuffer.size())
        _sendBuffer.clear();
    else
        _sendBuffer.erase(0, bytes);
}

void Client::clearBuffer()
{
    _recvBuffer.clear();
}

/* ================= CHANNEL ================= */

void Client::addChannel(Channel *channel)
{
    _channels.insert(channel);
}

void Client::removeChannel(Channel *channel)
{
    _channels.erase(channel);
}

bool Client::isInChannel(Channel *channel) const
{
    return (_channels.find(channel) != _channels.end());
}

/* ================= GETTERS ================= */

int Client::getFd() const
{
    return _fd;
}

const std::string& Client::getNickname() const
{
    return _nickname;
}

const std::string& Client::getUsername() const
{
    return _username;
}

const std::string& Client::getRealname() const
{
    return _realname;
}

const std::string& Client::getHostname() const
{
    return _hostname;
}

const std::string Client::getPrefix()
{
    return _nickname + "!" + _username + "@" + _hostname;
}

const std::string& Client::getSendBuffer() const
{
    return _sendBuffer;
}

std::string& Client::getRecvBuffer()
{
    return _recvBuffer;
}

bool Client::isRegistered() const
{
    return _registered;
}

bool Client::isPassOk() const
{
    return _passOk;
}

/* ================= SETTERS ================= */


void Client::setNickname(const std::string& nick)
{
    _nickname = nick;
}

void Client::setUsername(const std::string& user)
{
    _username = user;
}

void Client::setRealname(const std::string& real)
{
    _realname = real;
}

void Client::setPassOk(bool ok)
{
    _passOk = ok;
}

void Client::setNickOk(bool ok)
{
    _nickOk = ok;
}

void Client::setUserOk(bool ok)
{
    _userOk = ok;
}

void Client::setRegistered(bool ok)
{
    _registered = ok;
}

std::string Client::makeReply(const std::string& code, const std::string& target, const std::string& message)
{
    return ":" + _nickname + " " + code + " " + target + " :" + message;
}

void Client::sendMessage(const std::string& message)
{
    if (_fd < 0)
        return;

    std::string msg = message;

    if (msg.find("\r\n") == std::string::npos)
        msg += "\r\n";

    size_t total = 0;

    while (total < msg.size())
    {
        ssize_t sent = send(_fd, msg.c_str() + total, msg.size() - total, 0);
        if (sent <= 0)
            break;
        total += sent;
    }
}