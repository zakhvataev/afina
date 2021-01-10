#ifndef AFINA_NETWORK_MT_NONBLOCKING_CONNECTION_H
#define AFINA_NETWORK_MT_NONBLOCKING_CONNECTION_H

#include <cstring>
#include <vector>
#include <string>

#include <sys/socket.h>
#include <unistd.h>

#include <spdlog/logger.h>
#include <afina/execute/Command.h>
#include "protocol/Parser.h"
#include <sys/uio.h>
#include <deque>
#include <sys/epoll.h>
#include <iostream>
#include <atomic>

namespace Afina {
namespace Network {
namespace MTnonblock {

class Connection {
public:
    Connection(int s, std::shared_ptr<spdlog::logger> &pl, std::shared_ptr<Afina::Storage> &ps) :
    _socket(s),
    _logger(pl),
    _pStorage(ps),
    _arg_remains(0),
    _offset(0),
    _pos(0),
    _max_queue(32)
    {
        std::memset(&_event, 0, sizeof(struct epoll_event));
        _event.data.ptr = this;
    }

    inline bool isAlive(){
        return _running.load(std::memory_order_acquire);
    }

    void Start();

protected:
    void OnError();
    void OnClose();
    void DoRead();
    void DoWrite();

private:
    friend class Worker;
    friend class ServerImpl;

    int _socket;
    struct epoll_event _event;
    
    std::shared_ptr<spdlog::logger> _logger;
    std::shared_ptr<Afina::Storage> _pStorage;
    std::unique_ptr<Execute::Command> _command_to_execute;
    Protocol::Parser _parser;

    char _buf [2048];

    std::size_t _arg_remains;
    std::size_t _pos;
    std::size_t _offset;
    std::size_t _max_queue;

    std::string _argument_for_command;
    
    std::deque <std::string> _queue;

    std::atomic<bool> _running;
    std::atomic<bool> _eof;

    std::mutex _mutex;
};

} // namespace MTnonblock
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_MT_NONBLOCKING_CONNECTION_H
