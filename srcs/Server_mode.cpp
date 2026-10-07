#include "../includes/Server.hpp"
#include "../includes/replies.hpp"
#include "../includes/utils.hpp"
#include "../includes/Client.hpp"
#include "../includes/Channel.hpp"

void Server::handleMode(int fd, std::vector<std::string> params)
{
    if(params.size() < 2)
    {
        sendReply(fd, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters");
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

    if(params.size() == 2)
    {
        std::string modes = channel->getModeString();
        std::string reply = ":" + _name + " " + RPL_CHANNELMODEIS + " " + nick + " " + chanName + " " + modes;
        _clients[fd]->sendMessage(reply);
        return;
    }

    if(!channel->isOperator(fd))
    {
        std::string reply = ":" + _name + " " + ERR_CHANOPRIVSNEEDED + " " + nick + " " + chanName + " :You're not channel operator";
        _clients[fd]->sendMessage(reply);
        return;
    }

    std::string mode = params[2];
    bool adding = true;
    size_t argIndex = 3;
    std::string appliedModes;
    std::string appliedArgs;

    for(size_t i = 0; i < mode.size(); i++)
    {
        char c = mode[i];
        if(c == '+')
        {
            adding = true;
            appliedModes += "+";
        }
        else if(c == '-')
        {
            adding = false;
            appliedModes += "-";
        }
        else if(c == 'i')
        {
            channel->setInviteOnly(adding);
            appliedModes += "i";
        }
        else if(c == 't')
        {
            channel->setTopicRestricted(adding);
            appliedModes += "t";
        }
        else if(c == 'k')
        {
            if(adding)
            {
                if(argIndex >= params.size())
                    continue;
                channel->setKey(params[argIndex]);
                appliedArgs += " " + params[argIndex];
                argIndex++;
            }
            else
                channel->setKey("");
            appliedModes += "k";
        }
        else if(c == 'o')
        {
            if(argIndex >= params.size())
                continue;
            Client* target = getClientByNick(params[argIndex]);
            if(!target || !channel->isMember(target))
            {
                std::string reply = ":" + _name + " " + ERR_NOSUCHNICK + " " + nick + " " + params[argIndex] + " :No such nick/channel";
                _clients[fd]->sendMessage(reply);
                argIndex++;
                continue;
            }
            if(adding)
                channel->addOperator(target->getFd());
            else
                channel->removeOperator(target->getFd());
            appliedModes += "o";
            appliedArgs += " " + params[argIndex];
            argIndex++;
        }
        else if(c == 'l')
        {
            if(adding)
            {
                if(argIndex >= params.size())
                    continue;
                int limit = std::atoi(params[argIndex].c_str());
                if(limit <= 0)
                {
                    argIndex++;
                    continue;
                }
                channel->setUserLimit(limit);
                appliedArgs += " " + params[argIndex];
                argIndex++;
            }
            else
                channel->setUserLimit(0);
            appliedModes += "l";
        }
        else
        {
            std::string reply = ":" + _name + " " + ERR_UNKNOWNMODE + " " + nick + " " + std::string(1, c) + " :is unknown mode char";
            _clients[fd]->sendMessage(reply);
        }
    }

    if(!appliedModes.empty())
    {
        std::string msg = ":" + _clients[fd]->getPrefix() + " MODE " + chanName + " " + appliedModes + appliedArgs;
        channel->broadcast(msg, NULL);
    }
}
