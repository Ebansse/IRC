#ifndef SERVER_HPP
#define SERVER_HPP

#include <unistd.h>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <poll.h>
#include <netinet/in.h>
#include <stdexcept>
#include <time.h>
#include <ctime>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <cctype>
#include <cstdlib>
#include <csignal>
#include <sstream>

class Client;
class Channel;

extern volatile sig_atomic_t g_signal;

class Server
{
    private:
        int         _port;
        std::string _password;
        int         _fdserver;
        std::string _name;

        std::vector<pollfd> _pollfd;
        std::map <int, Client*> _clients;
        std::map <std::string, Channel*> _channels;


        void initSocket();
        void acceptClient();
        void handleClient(int fd);
        void disconnectClient(int fd);
        void processCommand(int fd, std::string command);
        void handlePass(int fd, std::vector<std::string> params);
        void handleNick(int fd, std::vector<std::string> params);
        void handleUser(int fd, std::vector<std::string> params);
        void handleJoin(int fd, std::vector<std::string> params);
        void handlePart(int fd, std::vector<std::string> params);
        void handlePrivmsg(int fd, std::vector<std::string> params);
        void handleKick(int fd, std::vector<std::string> params);
        void handleInvite(int fd, std::vector<std::string> params);
        void handleTopic(int fd, std::vector<std::string> params);
        void handleMode(int fd, std::vector<std::string> params);
        void sendReply(int fd, std::string code, std::string message);
        Client* getClientByNick(const std::string& nick);
    public:
        Server(int _port, std::string _password);
        ~Server();

        void run();
        void stop();

        // Getters
        int getPort() const;
        std::string getPassword() const;
        int getFdServer() const;
        std::string getName() const;
        std::vector<pollfd>& getPollfd();
        std::map<int, Client*>& getClients();
        std::map<std::string, Channel*>& getChannels();
};
#endif