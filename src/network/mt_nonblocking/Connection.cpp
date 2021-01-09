#include "Connection.h"

#include <iostream>

namespace Afina {
namespace Network {
namespace MTnonblock {

// See Connection.h
void Connection::Start() {
    std::lock_guard<std::mutex> lock(_mutex);

    _running = true;
    _event.events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
    _logger->debug("Start connection on socket {}", _socket);
}

// See Connection.h
void Connection::OnError() {
    std::lock_guard<std::mutex> lock(_mutex);

    _running = false;
    _logger->error("Error on socket {}", _socket);
}

// See Connection.h
void Connection::OnClose() {
    std::lock_guard<std::mutex> lock(_mutex);

    _running = false;
    _logger->debug("Client on socket {} closed connection", _socket);
    }

// See Connection.h
void Connection::DoRead() {
    std::lock_guard<std::mutex> lock(_mutex);

    _logger->debug("Start reading on socket {}", _socket);
    try {
        int readed_bytes = -1;
        while ((readed_bytes = read(_socket, _buf + _pos, sizeof(_buf) - _pos)) > 0) {
            _logger->debug("Got {} bytes from socket", readed_bytes);
            _pos += readed_bytes;

            while (_pos > 0) {
                _logger->debug("Process {} bytes", _pos);
                if (!_command_to_execute) {
                    std::size_t parsed = 0;
                    if (_parser.Parse(_buf, _pos, parsed)) {
                        _logger->debug("Found new command: {} in {} bytes", _parser.Name(), parsed);
                        _command_to_execute = _parser.Build(_arg_remains);
                        if (_arg_remains > 0) {
                            _arg_remains += 2;
                        }
                    }

                    if (parsed == 0) {
                        break;
                    } else {
                        std::memmove(_buf, _buf + parsed, _pos - parsed);
                        _pos -= parsed;
                    }
                }

                if (_command_to_execute && _arg_remains > 0) {
                    _logger->debug("Fill argument: {} bytes of {}", _pos, _arg_remains);
                    std::size_t to_read = std::min(_arg_remains, std::size_t(_pos));
                    _argument_for_command.append(_buf, to_read);

                    std::memmove(_buf, _buf + to_read, _pos - to_read);
                    _arg_remains -= to_read;
                    _pos -= to_read;
                }

                if (_command_to_execute && _arg_remains == 0) {
                    _logger->debug("Start command execution");

                    std::string result;
                    _command_to_execute->Execute(*_pStorage, _argument_for_command, result);

                    result += "\r\n";

                    bool was_empty = _queue.empty();
                    _queue.push_back(result);

                    if (was_empty) {
                        _event.events |= EPOLLOUT;
                    }

                    _command_to_execute.reset();
                    _argument_for_command.resize(0);
                    _parser.Reset();
                }
            } // end while (_pos)
        } // end while (read)

        if (readed_bytes == 0) {
            _logger->debug("Connection closed");
            _running = false;
        } else {
            throw std::runtime_error(std::string(strerror(errno)));
        }

    } catch (std::runtime_error &ex) {
        if (errno != EAGAIN) {
            _logger->error("Failed to read connection on descriptor {}: {}", _socket, ex.what());
            _running = false;
        }
    }
}

// See Connection.h
void Connection::DoWrite(){
    std::lock_guard<std::mutex> lock(_mutex);
    _logger->debug("Writing on socket {}", _socket);
    iovec write_vector[_max_queue];
    size_t write_ind = 0;
    try{
        auto it = _queue.begin();
        write_vector[write_ind].iov_base = &((*it)[0]) + _offset;
        write_vector[write_ind].iov_len = it->size() - _offset;
        it++;
        write_ind++;

        for(; it!= _queue.end(); it++){
            write_vector[write_ind].iov_base = &((*it)[0]);
            write_vector[write_ind].iov_len = it->size();
            if(++write_ind > _max_queue){ break; }
        }

        int writed = 0;
        
        if((writed = writev(_socket, write_vector, write_ind)) >= 0){
            size_t i = 0;
            while (i < write_ind && writed >= write_vector[i].iov_len){
                _queue.pop_front();
                writed -= write_vector[i].iov_len;
                i++;
            }
            _offset = writed;
        } else {
            throw std::runtime_error("Failed to send response");
        }

        if(_queue.empty()){
            _event.events &= ~EPOLLOUT;
        }

        if(_queue.size() <= _max_queue){
            _event.events |= EPOLLIN;
        }

    } catch (std::runtime_error &ex){
        if(errno != EAGAIN){
            _logger->error("Failed to write connection on descriptor {}: {}", _socket, ex.what());
            _running = false;
        }
    }
}

} // namespace MTnonblock
} // namespace Network
} // namespace Afina
