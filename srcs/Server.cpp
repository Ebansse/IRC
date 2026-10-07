#include "../includes/Server.hpp"
#include "../includes/replies.hpp"
#include "../includes/utils.hpp"
#include "../includes/Client.hpp"
#include "../includes/Channel.hpp"

volatile sig_atomic_t g_signal = 0;

static void signalHandler(int sig)
{
    g_signal = sig;
}

Server::Server(int _port, std::string _password)
{
    this->_port = _port;
    this->_password = _password;
    this->_name = "ircserv";
    initSocket();
    run();
}

Server::~Server()
{
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        close(it->first);
        delete it->second;
    }
    for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
        delete it->second;
    close(_fdserver);
}

void Server::sendReply(int fd, std::string code, std::string message)
{
    std::string nick = _clients[fd]->getNickname();
    if(nick.empty())
        nick = "*";
    std::string _reply = ":" + this->_name + " " + code + " " + nick + " :" + message + "\r\n";
    send(fd, _reply.c_str(), _reply.size(), 0);
}

void Server::acceptClient()
{
    sockaddr_in addr;
    socklen_t len = sizeof(addr);

    int _fdclient = accept(_fdserver, (sockaddr*)&addr, &len);
    if(_fdclient  == -1)
        return;
    if(fcntl(_fdclient, F_SETFL, O_NONBLOCK) == -1)
        throw std::runtime_error("Failed to set non-blocking mode on client fd");

    std::string ip = inet_ntoa(addr.sin_addr);
    int port = ntohs(addr.sin_port);

    Client* client = new Client(_fdclient, ip, port);
    this->_clients.insert(std::make_pair(_fdclient, client));
    
    pollfd clientPollfd;

        clientPollfd.fd = _fdclient;
        clientPollfd.events = POLLIN;
        clientPollfd.revents = 0;
    _pollfd.push_back(clientPollfd);
}

void Server::disconnectClient(int fd)
{
    std::vector<std::string> emptyChannels;
    for (std::map<std::string, Channel *>::iterator it = _channels.begin(); it != _channels.end(); ++it)
    {
        if (it->second->isMember(_clients[fd]))
        {
            it->second->removeMember(_clients[fd]);
            if (it->second->getMemberCount() == 0)
                emptyChannels.push_back(it->first);
        }
    }
    for (size_t i = 0; i < emptyChannels.size(); i++)
    {
        delete _channels[emptyChannels[i]];
        _channels.erase(emptyChannels[i]);
    }

    for(size_t i = 0; i < _pollfd.size(); i++)
    {
        if(_pollfd[i].fd == fd)
        {
            _pollfd.erase(_pollfd.begin() + i);
            break;
        }
    }
    delete _clients[fd];
    _clients.erase(fd);
    close(fd);
}

void Server::handleClient(int fd)
{
    char buffer[512];
    int bytesRead = recv(fd, buffer, 511, 0);

    if(bytesRead == 0)
    {
        disconnectClient(fd);
        return;
    }
    else if(bytesRead < 0)
        return;

    buffer[bytesRead] = '\0';
    _clients[fd]->appendToRecvBuffer(buffer);

    if(_clients[fd]->getRecvBuffer().size() > 510
        && _clients[fd]->getRecvBuffer().find("\n") == std::string::npos)
    {
        _clients[fd]->clearBuffer();
        return;
    }

    size_t pos;
    while((pos = _clients[fd]->getRecvBuffer().find("\n")) != std::string::npos)
    {
        if(pos > 510)
        {
            _clients[fd]->getRecvBuffer().erase(0, pos + 1);
            continue;
        }
        std::string command = _clients[fd]->getRecvBuffer().substr(0, pos);
        _clients[fd]->getRecvBuffer().erase(0, pos + 1);
        if(!command.empty() && command[command.size() - 1] == '\r')
            command.erase(command.size() - 1);
        if(!command.empty())
            processCommand(fd, command);
        if(_clients.find(fd) == _clients.end())
            return;
    }
}

void Server::run()
{
    signal(SIGINT, signalHandler);
    signal(SIGQUIT, signalHandler);
    while(!g_signal)
    {
        int ready = poll(_pollfd.data(), _pollfd.size(), -1);
        if(ready == -1)
        {
            if(g_signal)
                break;
            throw std::runtime_error("Poll failed");
        }
        for(size_t i = 0; i < _pollfd.size(); i++)
        {
            if(_pollfd[i].revents == 0)
                continue;
            if(_pollfd[i].fd == _fdserver)
            {
                acceptClient();
                continue;
            }
            int clientFd = _pollfd[i].fd;
            if(_pollfd[i].revents & (POLLHUP | POLLERR))
            {
                disconnectClient(clientFd);
                i--;
                continue;
            }
            if(_pollfd[i].revents & POLLIN)
            {
                handleClient(clientFd);
                if(_clients.find(clientFd) == _clients.end())
                {
                    i--;
                    continue;
                }
            }
        }
    }
}

void Server::stop()
{
    
}

void Server::initSocket()
{
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSocket == -1)
        throw std::runtime_error("Failed to create socket");
    this->_fdserver = serverSocket;
    int opt = 1;
    if(setsockopt(_fdserver, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        throw std::runtime_error("Failed to set socker options");
    if(fcntl(_fdserver, F_SETFL, O_NONBLOCK) == -1)
        throw std::runtime_error("Failed to set non-blocking mode");

    sockaddr_in serverAddr;
      serverAddr.sin_family = AF_INET;
      serverAddr.sin_port = htons(_port);
      serverAddr.sin_addr.s_addr = INADDR_ANY;

    if(bind(_fdserver, (sockaddr*)&serverAddr, sizeof(serverAddr)) == -1)
        throw std::runtime_error("Failed to bind socket ");
    if(listen(_fdserver, 10) == -1 )
        throw std::runtime_error("Failed to listen on socket ");

    pollfd serverPollfd;

      serverPollfd.fd = _fdserver;
      serverPollfd.events = POLLIN;
      serverPollfd.revents = 0;
    _pollfd.push_back(serverPollfd);

}

// Getters
int Server::getPort() const
{
    return this->_port;
}

std::string Server::getPassword() const
{
    return this->_password;
}

int Server::getFdServer() const
{
    return this->_fdserver;
}

std::string Server::getName() const
{
    return this->_name;
}

std::vector<pollfd>& Server::getPollfd()
{
    return this->_pollfd;
}

std::map<int, Client*>& Server::getClients()
{
    return this->_clients;
}

std::map<std::string, Channel*>& Server::getChannels()
{
    return this->_channels;
}
