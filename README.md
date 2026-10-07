*This project has been created as part of the 42 curriculum by cguinot, ebansse, rjacquet.*

## Description

ft_irc is an IRC server written in C++98. The goal is to reproduce the core behavior of a real IRC server, handling multiple clients simultaneously through non-blocking I/O and a single `poll()` loop.

The server supports user authentication via password, nickname and username registration, channel management, private messaging, and operator-specific commands. It is compatible with standard IRC clients (tested with HexChat as the reference client).

## Supported Commands

- **PASS / NICK / USER** : Client authentication and registration
- **JOIN / PART** : Join or leave a channel
- **PRIVMSG** : Send messages to channels or users
- **KICK** : Eject a user from a channel (operator only)
- **INVITE** : Invite a user to a channel (operator only)
- **TOPIC** : View or change the channel topic
- **MODE** : Change channel modes (i, t, k, o, l)
- **QUIT** : Disconnect from the server
- **PING / PONG** : Connection keepalive

## Instructions

### Compilation

```
make
```

### Execution

```
./ircserv <port> <password>
```

- `port` : TCP port the server will listen on (1-65535)
- `password` : Connection password required by clients

### Connecting with HexChat

1. Open HexChat, go to **Network List**
2. Add a new network, set the server address to `127.0.0.1/<port>`
3. Set the server password to your chosen password
4. Connect

### Testing with netcat

```
nc -C 127.0.0.1 6667
PASS test
NICK alice
USER alice 0 * :Alice
JOIN #general
PRIVMSG #general :hello everyone
```

## Technical Choices

- **I/O multiplexing** : `poll()` with a single loop handling accept, read, and write
- **Non-blocking sockets** : All file descriptors are set to `O_NONBLOCK` via `fcntl()`
- **Buffer management** : Per-client receive buffer to handle partial data (Ctrl+D test)
- **Memory management** : Dynamic allocation for Client and Channel objects, cleaned up on disconnect and server shutdown
- **Signal handling** : SIGINT and SIGQUIT are caught for clean shutdown
- **Operator system** : First user to join a channel becomes operator; if the operator leaves, the next member is promoted automatically

## Resources

- [RFC 2812 - Internet Relay Chat: Client Protocol](https://datatracker.ietf.org/doc/html/rfc2812)
- [RFC 1459 - Internet Relay Chat Protocol](https://datatracker.ietf.org/doc/html/rfc1459)
- [Modern IRC Client Protocol](https://modern.ircdocs.horse/)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- `man poll`, `man socket`, `man send`, `man recv`, `man fcntl`

### AI Usage

AI (Claude) was used as a learning and productivity tool during this project :
- Debugging help : identifying uninitialized variables, use-after-free risks, and memory leaks in the initial codebase
- Writing repetitive command handlers (PART, KICK, INVITE) that follow the same validation pattern
- Generating the automated test script (`tests.sh`) for regression testing
- Explaining IRC protocol specifics (numeric replies, mode parsing, CAP negotiation)

The core commands (JOIN, PRIVMSG, MODE) were written by hand with guidance, to ensure a solid understanding of the protocol and the codebase.
