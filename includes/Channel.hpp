#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <time.h>
#include <ctime>

class Client;

class Channel
{
    private:
        std::string _name;
        std::string _topic;
        std::string _topicSetter;
        time_t _topicTime;
        std::string _key;
        size_t _userLimit;

        std::map<int, Client*> _members;
        std::set<int> _operators;
        std::set<int> _invited;

        bool _inviteOnly;
        bool _topicRestricted;
        bool _hasKey;
        bool _hasLimit;
        bool _moderated;

    public:
        Channel(const std::string& name);
        ~Channel();

        const std::string& getName() const;
        const std::string& getTopic() const;
        const std::string& getTopicSetter() const;
        const time_t& getTopicTime() const;
        const std::string& getKey() const;
        const size_t& getUserLimit() const;
        size_t getMemberCount() const;
        std::string getModeString() const;

        void setTopic(const std::string& topic, const std::string& setter);
        void setKey(const std::string& key);
        void setUserLimit(size_t limit);
        void setInviteOnly(bool value);
        void setTopicRestricted(bool value);

        void addMember(Client* client);
        void removeMember(Client* client);
        bool isMember(Client* client) const;
        bool isFull() const;
        std::vector<Client*> getMembers() const;
        std::string getMemberList() const;

        void addOperator(int fd);
        void removeOperator(int fd);
        bool isOperator(int fd) const;

        void addInvite(int fd);
        void removeInvite(int fd);
        bool isInvited(int fd) const;

        bool canJoin(Client* client, const std::string& key) const;
        bool canSpeak(Client* client) const;
        bool canChangeTopic(Client* client) const;
        
        void broadcast(const std::string& message, Client* exclude);
};

#endif