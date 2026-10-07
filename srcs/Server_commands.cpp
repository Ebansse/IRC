#include "../includes/Server.hpp"
#include "../includes/replies.hpp"
#include "../includes/utils.hpp"
#include "../includes/Client.hpp"
#include "../includes/Channel.hpp"

void Server::processCommand(int fd, std::string command)
{
    std::vector<std::string> params = parseCommand(command);
    if(params.size() == 0)
        return;

    std::string cmd = params[0];
    for(size_t i = 0; i < cmd.size(); i++)
        cmd[i] = std::toupper(cmd[i]);

    if(cmd == "CAP")
        return;
    if(cmd == "PING")
    {
        std::string token = params.size() > 1 ? params[1] : "";
        _clients[fd]->sendMessage(":" + _name + " PONG " + _name + " :" + token);
        return;
    }
    if(cmd == "QUIT")
    {
        std::string quitMsg = params.size() > 1 ? params[1] : "Quit";
        if(_clients[fd]->isRegistered())
        {
            std::string prefix = ":" + _clients[fd]->getPrefix() + " QUIT :" + quitMsg;
            for(std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
            {
                if(it->second->isMember(_clients[fd]))
                    it->second->broadcast(prefix, _clients[fd]);
            }
        }
        disconnectClient(fd);
        return;
    }

    if(_clients[fd]->isRegistered() == false)
    {
        if(cmd == "PASS")
            handlePass(fd, params);
        else if(cmd == "NICK")
            handleNick(fd, params);
        else if(cmd == "USER")
            handleUser(fd, params);
        else
            sendReply(fd, ERR_NOTREGISTERED, "You have not registered");
        return;
    }

    if(cmd == "PASS")
        handlePass(fd, params);
    else if(cmd == "NICK")
        handleNick(fd, params);
    else if(cmd == "USER")
        handleUser(fd, params);
    else if(cmd == "JOIN")
        handleJoin(fd, params);
    else if(cmd == "PART")
        handlePart(fd, params);
    else if(cmd == "PRIVMSG")
        handlePrivmsg(fd, params);
    else if(cmd == "KICK")
        handleKick(fd, params);
    else if(cmd == "INVITE")
        handleInvite(fd, params);
    else if(cmd == "TOPIC")
        handleTopic(fd, params);
    else if(cmd == "MODE")
        handleMode(fd, params);
    else
        sendReply(fd, ERR_UNKNOWNCOMMAND, cmd + " :Unknown command");
}

void Server::handlePass(int fd, std::vector<std::string> params)    
{
    if(params.size() < 2)
    {
        sendReply(fd, ERR_NEEDMOREPARAMS, "PASS :Not enough parameters");
        return;
    }
    if(_clients[fd]->isRegistered() == true)
    {
        sendReply(fd, ERR_ALREADYREGISTRED , "You may not reregister");
        return;
    }
    if(this->_password == params[1])
        _clients[fd]->setPassOk(true);
    else
        sendReply(fd, ERR_PASSWDMISMATCH, "Password incorrect");
}

void Server::handleNick(int fd, std::vector<std::string> params)
{
    if(params.size() < 2)
    {
        sendReply(fd, ERR_NONICKNAMEGIVEN, "No nickname given");
        return;
    }
    //check validité du Nickname.
    if(params[1].size() > 9 || isdigit(params[1][0]) )
    {
        sendReply(fd, ERR_ERRONEUSNICKNAME, "Erroneous nickname");
        return;
    }
    for(size_t i = 0 ; i < params[1].size(); i++)
    {
        if(params[1][i] == ' ')
        {
            sendReply(fd, ERR_ERRONEUSNICKNAME, "Erroneous nickname");
            return;
        }
    }
    std::map<int, Client*>::iterator it;
    for(it = _clients.begin(); it != _clients.end(); it++)
    {
        if(it->first != fd && it->second->getNickname() == params[1])
        {
            sendReply(fd, ERR_NICKNAMEINUSE, "Nickname is already in use");
            return;
        }
    }
    std::string oldNick = _clients[fd]->getNickname();
    _clients[fd]->setNickname(params[1]);
    if(_clients[fd]->isRegistered())
    {
        if(oldNick != params[1])
        {
            std::string msg = ":" + oldNick + "!" + _clients[fd]->getUsername()
                + "@" + _clients[fd]->getHostname() + " NICK :" + params[1];
            _clients[fd]->sendMessage(msg);
            for(std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
            {
                if(it->second->isMember(_clients[fd]))
                    it->second->broadcast(msg, _clients[fd]);
            }
        }
    }
    else if(_clients[fd]->isPassOk() && _clients[fd]->getNickname().size() != 0 && _clients[fd]->getUsername().size() != 0)
    {
        _clients[fd]->setRegistered(true);
        std::string nick = _clients[fd]->getNickname();
        std::string user = _clients[fd]->getUsername();
        sendReply(fd, RPL_WELCOME, "Welcome to the IRC Network " + nick + "!" + user + "@localhost");
        sendReply(fd, RPL_YOURHOST, "Your host is " + this->_name + ", running version 1.0");
        sendReply(fd, RPL_CREATED, "This server was created today");
        sendReply(fd, RPL_MYINFO, this->_name + " 1.0 o itkol");
    }
}

void Server::handleUser(int fd, std::vector<std::string> params)
{
    if(params.size() < 5)
    {
        sendReply(fd, ERR_NEEDMOREPARAMS, "USER :Not enough parameters");
        return;
    }
    if(_clients[fd]->isRegistered() == true)
    {
        sendReply(fd, ERR_ALREADYREGISTRED, "You may not reregister");
        return;
    }
    if(_clients[fd]->isPassOk() == false)
    {
        sendReply(fd, ERR_PASSWDMISMATCH, "Password incorrect");
        return;
    }
    _clients[fd]->setUsername(params[1]);
    _clients[fd]->setRealname(params[4]);
    if(_clients[fd]->isPassOk() && _clients[fd]->getNickname().size() != 0 && _clients[fd]->getUsername().size() != 0)
    {
        _clients[fd]->setRegistered(true);
        std::string nick = _clients[fd]->getNickname();
        std::string user = _clients[fd]->getUsername();
        sendReply(fd, RPL_WELCOME, "Welcome to the IRC Network " + nick + "!" + user + "@localhost");
        sendReply(fd, RPL_YOURHOST, "Your host is " + this->_name + ", running version 1.0");
        sendReply(fd, RPL_CREATED, "This server was created today");
        sendReply(fd, RPL_MYINFO, this->_name + " 1.0 o itkol");
    }
}

Client* Server::getClientByNick(const std::string& nick)
{
    for(std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        if(it->second->getNickname() == nick)
            return it->second;
    }
    return NULL;
}

void Server::handleJoin(int fd, std::vector<std::string> params)
{
    if(params.size() < 2)
    {
        sendReply(fd, ERR_NEEDMOREPARAMS, "need more params");
        return;
    }

    std::string chanName = params[1];
    std::string key = params.size() > 2 ? params[2] : "";
    if(chanName[0] != '#')
    {
      sendReply(fd, ERR_NOSUCHCHANNEL, chanName + " :No such channel");
      return;
    }

    Channel* channel;
    if(_channels.find(chanName) == _channels.end())
    {
        channel = new Channel(chanName);
        _channels[chanName] = channel;
    }
    else
    {
        channel = _channels[chanName];
        if(channel->isMember(_clients[fd]))
            return;
        if(channel->isFull())
        {
            std::string reply = ":" + _name + " " + ERR_CHANNELISFULL + " " + _clients[fd]->getNickname() + " " + chanName + " :Cannot join channel (+l)";
            _clients[fd]->sendMessage(reply);
            return;
        }
        if(channel->isInvited(fd) == false && channel->getModeString().find('i') != std::string::npos)
        {
            std::string reply = ":" + _name + " " + ERR_INVITEONLYCHAN + " " + _clients[fd]->getNickname() + " " + chanName + " :Cannot join channel (+i)";
            _clients[fd]->sendMessage(reply);
            return;
        }
        if(channel->getKey().empty() == false && channel->getKey() != key)
        {
            std::string reply = ":" + _name + " " + ERR_BADCHANNELKEY + " " + _clients[fd]->getNickname() + " " + chanName + " :Cannot join channel (+k)";
            _clients[fd]->sendMessage(reply);
            return;
        }
    }

    channel->addMember(_clients[fd]);
    channel->removeInvite(fd);
    std::string msg = ":" + _clients[fd]->getPrefix() + " JOIN " + chanName;
    channel->broadcast(msg, NULL);

    if(!channel->getTopic().empty())
    {
        std::string reply = ":" + _name + " 332 " + _clients[fd]->getNickname() + " " + chanName + " :" + channel->getTopic();
        _clients[fd]->sendMessage(reply);
    }                                                                                                                                                                                                                                                                                                                        

    std::string reply = ":" + _name + " 353 " + _clients[fd]->getNickname() + " = " + chanName + " :" + channel->getMemberList();
    _clients[fd]->sendMessage(reply);

    std::string endreply = ":" + _name + " 366 " + _clients[fd]->getNickname() + " " + chanName + " :End of /NAMES list";
    _clients[fd]->sendMessage(endreply);
}

void Server::handlePrivmsg(int fd, std::vector<std::string> params)
{
    if(params.size() < 2)
    {
        sendReply(fd, ERR_NORECIPIENT, "No recipient given");
        return;
    }
    else if(params.size() < 3)
    {
        sendReply(fd, ERR_NOTEXTTOSEND, "No text to send");
        return;
    }
    std::string target = params[1];
    std::string text = params[2];

    std::string msg = ":" + _clients[fd]->getPrefix() + " PRIVMSG " + target + " :" + text ;
    
    if(target[0] == '#')
    {
        if(_channels.find(target) == _channels.end())
        {
            sendReply(fd, ERR_NOSUCHCHANNEL, target + " :No such channel");
            return;
        }
        Channel* channel = _channels[target];
        if(!channel->isMember(_clients[fd]))
        {
            sendReply(fd, ERR_CANNOTSENDTOCHAN, target + " :Cannot send to channel");
            return;
        }
        channel->broadcast(msg, _clients[fd]);
    }
    else
    {
        if(getClientByNick(target) == NULL)
        {
            sendReply(fd, ERR_NOSUCHNICK, target + " :Client doesn't exist");
            return;
        }
        getClientByNick(target)->sendMessage(msg);
    }
}

void Server::handlePart(int fd, std::vector<std::string> params)
{
    if(params.size() < 2)
    {
        sendReply(fd, ERR_NEEDMOREPARAMS, "PART :Not enough parameters");
        return;
    }
    std::string chanName = params[1];
    if(_channels.find(chanName) == _channels.end())
    {
        std::string reply = ":" + _name + " " + ERR_NOSUCHCHANNEL + " " + _clients[fd]->getNickname() + " " + chanName + " :No such channel";
        _clients[fd]->sendMessage(reply);
        return;
    }
    Channel* channel = _channels[chanName];
    if(!channel->isMember(_clients[fd]))
    {
        std::string reply = ":" + _name + " " + ERR_NOTONCHANNEL + " " + _clients[fd]->getNickname() + " " + chanName + " :You're not on that channel";
        _clients[fd]->sendMessage(reply);
        return;
    }
    std::string reason = params.size() > 2 ? params[2] : "";
    std::string msg = ":" + _clients[fd]->getPrefix() + " PART " + chanName;
    if(!reason.empty())
        msg += " :" + reason;
    channel->broadcast(msg, NULL);
    channel->removeMember(_clients[fd]);
    if(channel->getMemberCount() == 0)
    {
        delete channel;
        _channels.erase(chanName);
    }
}


void Server::handleKick(int fd, std::vector<std::string> params)
{
    if(params.size() < 3)
    {
        sendReply(fd, ERR_NEEDMOREPARAMS, "KICK :Not enough parameters");
        return;
    }
    std::string chanName = params[1];
    std::string targetNick = params[2];
    std::string reason = params.size() > 3 ? params[3] : targetNick;

    if(_channels.find(chanName) == _channels.end())
    {
        std::string reply = ":" + _name + " " + ERR_NOSUCHCHANNEL + " " + _clients[fd]->getNickname() + " " + chanName + " :No such channel";
        _clients[fd]->sendMessage(reply);
        return;
    }
    Channel* channel = _channels[chanName];
    if(!channel->isMember(_clients[fd]))
    {
        std::string reply = ":" + _name + " " + ERR_NOTONCHANNEL + " " + _clients[fd]->getNickname() + " " + chanName + " :You're not on that channel";
        _clients[fd]->sendMessage(reply);
        return;
    }
    if(!channel->isOperator(fd))
    {
        std::string reply = ":" + _name + " " + ERR_CHANOPRIVSNEEDED + " " + _clients[fd]->getNickname() + " " + chanName + " :You're not channel operator";
        _clients[fd]->sendMessage(reply);
        return;
    }
    Client* target = getClientByNick(targetNick);
    if(!target || !channel->isMember(target))
    {
        std::string reply = ":" + _name + " " + ERR_USERNOTINCHANNEL + " " + _clients[fd]->getNickname() + " " + targetNick + " " + chanName + " :They aren't on that channel";
        _clients[fd]->sendMessage(reply);
        return;
    }
    std::string msg = ":" + _clients[fd]->getPrefix() + " KICK " + chanName + " " + targetNick + " :" + reason;
    channel->broadcast(msg, NULL);
    channel->removeMember(target);
    if(channel->getMemberCount() == 0)
    {
        delete channel;
        _channels.erase(chanName);
    }
}

void Server::handleInvite(int fd, std::vector<std::string> params)
{
    if(params.size() < 3)
    {
        sendReply(fd, ERR_NEEDMOREPARAMS, "INVITE :Not enough parameters");
        return;
    }
    std::string targetNick = params[1];
    std::string chanName = params[2];

    Client* target = getClientByNick(targetNick);
    if(!target)
    {
        std::string reply = ":" + _name + " " + ERR_NOSUCHNICK + " " + _clients[fd]->getNickname() + " " + targetNick + " :No such nick/channel";
        _clients[fd]->sendMessage(reply);
        return;
    }
    if(_channels.find(chanName) == _channels.end())
    {
        std::string reply = ":" + _name + " " + ERR_NOSUCHCHANNEL + " " + _clients[fd]->getNickname() + " " + chanName + " :No such channel";
        _clients[fd]->sendMessage(reply);
        return;
    }
    Channel* channel = _channels[chanName];
    if(!channel->isMember(_clients[fd]))
    {
        std::string reply = ":" + _name + " " + ERR_NOTONCHANNEL + " " + _clients[fd]->getNickname() + " " + chanName + " :You're not on that channel";
        _clients[fd]->sendMessage(reply);
        return;
    }
    if(!channel->isOperator(fd))
    {
        std::string reply = ":" + _name + " " + ERR_CHANOPRIVSNEEDED + " " + _clients[fd]->getNickname() + " " + chanName + " :You're not channel operator";
        _clients[fd]->sendMessage(reply);
        return;
    }
    if(channel->isMember(target))
    {
        std::string reply = ":" + _name + " " + ERR_USERONCHANNEL + " " + _clients[fd]->getNickname() + " " + targetNick + " " + chanName + " :is already on channel";
        _clients[fd]->sendMessage(reply);
        return;
    }
    channel->addInvite(target->getFd());
    std::string reply = ":" + _name + " " + RPL_INVITING + " " + _clients[fd]->getNickname() + " " + targetNick + " " + chanName;
    _clients[fd]->sendMessage(reply);
    std::string msg = ":" + _clients[fd]->getPrefix() + " INVITE " + targetNick + " :" + chanName;
    target->sendMessage(msg);
}


void Server::handleTopic(int fd, std::vector<std::string> params)
{
    if(params.size() < 2)
    {
        sendReply(fd, ERR_NEEDMOREPARAMS, "TOPIC :Not enough parameters");
        return;
    }
    std::string chanName = params[1];
    std::string nick = _clients[fd]->getNickname();

    if(_channels.find(chanName) == _channels.end())
    {
        std::string reply = ":" + _name + " " + ERR_NOSUCHCHANNEL + " " + nick + " " + chanName + " :No such channel";
        _clients[fd]->sendMessage(reply);
        return;
    }
    Channel* channel = _channels[chanName];
    if(!channel->isMember(_clients[fd]))
    {
        std::string reply = ":" + _name + " " + ERR_NOTONCHANNEL + " " + nick + " " + chanName + " :You're not on that channel";
        _clients[fd]->sendMessage(reply);
        return;
    }
    if(params.size() == 2)
    {
        if(channel->getTopic().empty())
        {
            std::string reply = ":" + _name + " " + RPL_NOTOPIC + " " + nick + " " + chanName + " :No topic is set";
            _clients[fd]->sendMessage(reply);
        }
        else
        {
            std::string reply = ":" + _name + " " + RPL_TOPIC + " " + nick + " " + chanName + " :" + channel->getTopic();
            _clients[fd]->sendMessage(reply);
            std::ostringstream oss;
            oss << channel->getTopicTime();
            std::string reply2 = ":" + _name + " " + RPL_TOPICWHOTIME + " " + nick + " " + chanName + " " + channel->getTopicSetter() + " " + oss.str();
            _clients[fd]->sendMessage(reply2);
        }
        return;
    }
    if(!channel->canChangeTopic(_clients[fd]))
    {
        std::string reply = ":" + _name + " " + ERR_CHANOPRIVSNEEDED + " " + nick + " " + chanName + " :You're not channel operator";
        _clients[fd]->sendMessage(reply);
        return;
    }
    channel->setTopic(params[2], nick);
    std::string msg = ":" + _clients[fd]->getPrefix() + " TOPIC " + chanName + " :" + params[2];
    channel->broadcast(msg, NULL);
}