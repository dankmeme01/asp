#include <asp/thread/Thread.hpp>

namespace asp::thread {

template <>
void Thread<>::start() {
    _storage->_stopped.clear();
    _handle = std::thread([_storage = _storage]() {
        if (_storage->onStart) {
            _storage->onStart();
        }

        try {
            while (!_storage->_stopped) {
                _storage->loopFunc();
            }
        } catch (const std::exception& e) {
            if (_storage->onException) {
                _storage->onException(e);
            } else {
                asp::log(LogLevel::Error, "unhandled exception from a Thread: {}", e.what());
                throw;
            }
        }

        if (_storage->onTermination) {
            _storage->onTermination();
        }
    });
}

}