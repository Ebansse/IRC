#include "../includes/Channel.hpp"
#include "../includes/Client.hpp"

Channel::Channel(const std::string& name) : _name(name), _topicTime(0),
    _userLimit(0), _inviteOnly(false), _topicRestricted(false),
    _hasKey(false), _hasLimit(false), _moderated(false) {}

Channel::~Channel() {}

/* ================= GETTERS ================= */


const std::string& Channel::getName() const
{
    return _name;
}

const std::string& Channel::getTopic() const
{
    return _topic;
}

const std::string& Channel::getTopicSetter() const
{
    return _topicSetter;
}

const time_t& Channel::getTopicTime() const
{
    return _topicTime;
}

const std::string& Channel::getKey() const
{
    return _key;
}

const size_t& Channel::getUserLimit() const
{
    return _userLimit;
}

size_t Channel::getMemberCount() const
{
    return _members.size();
}

std::string Channel::getModeString() const
{
    std::string modes = "+";

    if (_inviteOnly)
        modes += "i";
    if (_topicRestricted)
        modes += "t";
    if (_hasKey)
        modes += "k";
    if (_hasLimit)
        modes += "l";

    if (modes.length() == 1) // aucun mode actif
        return "";

    return modes;
}

/* ==================== */
/*        SETTERS       */
/* ==================== */

void Channel::setTopic(const std::string& topic, const std::string& setter)
{
    _topic = topic;
    _topicSetter = setter;
    _topicTime = std::time(NULL);
}

void Channel::setKey(const std::string& key)
{
    if (key.empty())
    {
        _key = "";
        _hasKey = false;
    }
    else
    {
        _key = key;
        _hasKey = true;
    }
}

void Channel::setUserLimit(size_t limit)
{
    if (limit == 0)
    {
        _userLimit = 0;
        _hasLimit = false;
    }
    else
    {
        _userLimit = limit;
        _hasLimit = true;
    }
}

void Channel::setInviteOnly(bool value)
{
    _inviteOnly = value;
}

void Channel::setTopicRestricted(bool value)
{
    _topicRestricted = value;
}

/* ==================== */
/*          ADD         */
/* ==================== */

void Channel::addMember(Client* client)
{
    if (!client)
        return;

    int fd = client->getFd();

    if (_members.find(fd) != _members.end())
        return;

    if (_operators.empty())
        _operators.insert(fd);

    _members[fd] = client;
    client->addChannel(this);
}

void Channel::removeMember(Client* client)
{
    if (!client)
        return;

    int fd = client->getFd();
    
    _members.erase(fd);
    _operators.erase(fd);
    client->removeChannel(this);

    if (_operators.empty() && !_members.empty())
    {
        int newOp = _members.begin()->first;
        _operators.insert(newOp);
    }
}

bool Channel::isMember(Client* client) const
{
    if (_members.find(client->getFd()) != _members.end())
        return true;
    return false;
}

std::vector<Client*> Channel::getMembers() const
{
    std::vector<Client*> vecMembers;

    for (std::map<int, Client*>::const_iterator it = _members.begin(); it != _members.end(); ++it)
    {
        vecMembers.push_back(it->second);
    }

    return vecMembers;
}

void Channel::addOperator(int fd)
{
    _operators.insert(fd);
}

void Channel::removeOperator(int fd)
{
    _operators.erase(fd);
}

bool Channel::isOperator(int fd) const
{
    if (_operators.find(fd) != _operators.end())
        return true;
    return false;
}

void Channel::addInvite(int fd)
{
    _invited.insert(fd);
}

void Channel::removeInvite(int fd)
{
    _invited.erase(fd);
}

bool Channel::isInvited(int fd) const
{
    if (_invited.find(fd) != _invited.end())
        return true;
    return false;
}

bool Channel::canJoin(Client* client, const std::string& key) const
{
    if (_inviteOnly && _invited.find(client->getFd()) == _invited.end())
        return false;
    
    if (_hasKey && key != _key)
        return false;

    if (_hasLimit && _members.size() >= _userLimit)
        return false;

    return true;
}

bool Channel::canSpeak(Client* client) const
{
    int fd = client->getFd();

    if (_members.find(fd) == _members.end())
        return false;

    if (!_moderated)
        return true;

    if (_operators.find(fd) != _operators.end())
        return true;
    
    return false;
}

bool Channel::isFull() const
{
    if (_hasLimit)
    {
        if (_members.size() >= _userLimit)
        return true;
    }
    return false;
}

bool Channel::canChangeTopic(Client* client) const
{
    if (_topicRestricted)
    {
        if (isOperator(client->getFd()))
            return true;
        return false;
    }
    return true;
}

std::string Channel::getMemberList() const
{
    std::string list;
    for (std::map<int, Client*>::const_iterator it = _members.begin(); it != _members.end(); ++it)
    {
        if (!list.empty())
            list += " ";
        if (_operators.find(it->first) != _operators.end())
            list += "@";
        list += it->second->getNickname();
    }
    return list;
}

void Channel::broadcast(const std::string& message, Client* sender)
{
    for (std::map<int, Client*>::iterator it = _members.begin();
         it != _members.end();
         ++it)
    {
        if (it->second != sender)
            it->second->sendMessage(message);
    }
}
