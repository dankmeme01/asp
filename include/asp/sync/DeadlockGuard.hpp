#pragma once
#include "../config.hpp"
#include <thread>

namespace asp::sync {

class DeadlockGuard {
public:
    void lockAttempt();
    void lockSuccess();
    void unlock();

private:
    std::thread::id current;
    std::mutex mtx;
};

}