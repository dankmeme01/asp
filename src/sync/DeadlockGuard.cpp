#include <asp/sync/DeadlockGuard.hpp>

#include <stdexcept>
#include <thread>

namespace asp::sync {

void DeadlockGuard::lockAttempt() {
    std::lock_guard lock(mtx);

    auto myId = std::this_thread::get_id();

    if (current == myId) {
        throw std::runtime_error("failed to lock mutex: already locked by this thread.");
    }

    current = myId;
}

void DeadlockGuard::lockSuccess() {
    std::lock_guard lock(mtx);
    current = {};
}

void DeadlockGuard::unlock() {
    std::lock_guard lock(mtx);
    current = {};
}

}