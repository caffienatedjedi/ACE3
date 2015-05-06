#pragma once

#include <thread>
#include <mutex>
#include <queue>

#include "shared.hpp"
#include "arguments.hpp"
#include "singleton.hpp"

namespace ace {
    class dispatcher {
    public:
        virtual bool call(const std::string & name_, const arguments & args_, std::string & result_) {
            if (_methods.find(name_) == _methods.end()) {
                // @TODO: Exceptions
                return false;
            }
            return _methods[name_](args_, result_);
        }

        bool add(const std::string & name_, std::function<bool(const arguments &, std::string &)> func_) {
            if (_methods.find(name_) != _methods.end()) {
                // @TODO: Exceptions
                return false;
            }
            _methods[name_] = func_;

            return true;
        }
    protected:
        std::map < std::string, std::function<bool(const arguments &, std::string &)> > _methods;
    };
    class dispatch : public dispatcher, public singleton<dispatch> { };

    struct dispatch_message {
        dispatch_message(const std::string & command_, const arguments & args_, const uint64_t id_) : command(command_), args(args_), id(id_) {}
        std::string command;
        arguments args;
        uint64_t    id;
    };
    struct dispatch_result {
        dispatch_result() {}
        dispatch_result(const std::string &res, const uint64_t id_) : message(res), id(id_) {}
        std::string message;
        uint64_t    id;
    };

    class threaded_dispatcher : public dispatcher {
    public:
        threaded_dispatcher() : _worker(&ace::threaded_dispatcher::monitor, this) {
 
        }
        ~threaded_dispatcher() {}
        
        bool call(const std::string & name_, const arguments & args_, std::string & result_, bool threaded) {
            if (_methods.find(name_) == _methods.end()) {
                // @TODO: Exceptions
                return false;
            }
            if (threaded) {
                std::lock_guard<std::mutex> lock(_messages_lock);
                _messages.push(dispatch_message(name_, args_, _message_id));
                
                // @TODO: We should provide an interface for this serialization.
                std::stringstream ss;
                ss << "[\"result_id\", \"" << _message_id << "\" ]";
                result_ = ss.str();

                _message_id = _message_id + 1;
            } else {
                LOG(TRACE) << "dispatch[immediate]:\t[" << name_ << "] { " << args_.get() << " }";
                return dispatcher::call(name_, args_, result_);
            }

            return true;
        }
        bool call(const std::string & name_, const arguments & args_, std::string & result_) override {
            return call(name_, args_, result_, false);
        }

        void push_result(const dispatch_result & result) {
            {
                std::lock_guard<std::mutex> lock(_results_lock);
                _results.push(result);
            }
        }
        void push_result(const std::string & result) {
            push_result(dispatch_result(result, -1));
        }

    protected:
        void monitor() {
            while (true) {
                {
                    std::lock_guard<std::mutex> lock(_messages_lock);
                    while (!_messages.empty()) {
                        dispatch_result result;
                        result.id = _messages.front().id;
                        result.message.resize(4096);
                        
                        LOG(TRACE) << "dispatch[threaded]:\t[" << _messages.front().command << "] { " << _messages.front().args.get() << " }";
                        dispatcher::call(_messages.front().command, _messages.front().args, result.message);
                        {
                            std::lock_guard<std::mutex> lock(_results_lock);
                            _results.push(result);
                        }

                        _messages.pop();
                    }
                }
            }
        }
        std::queue<dispatch_result> _results;
        std::mutex              _results_lock;

        std::queue<dispatch_message> _messages;
        std::mutex              _messages_lock;

        std::thread             _worker;

        uint64_t                _message_id;
    };
    class threaded_dispatch : public threaded_dispatcher, public singleton<dispatch> { };
};