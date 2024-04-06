#include <asp/async/Runtime.hpp>
#include <asp/sync/Atomic.hpp>
#include <asp/thread/ThreadPool.hpp>

#include <utility>
#include <thread>

namespace asp::async {

static constexpr RuntimeSettings DEFAULT_SETTINGS = {
    .threadCount = 0
};

/* RuntimeImpl declaration */
class RuntimeImpl {
public:
    RuntimeImpl(const RuntimeSettings& settings) : settings(settings) {}

    void launch();
    void runAsync(std::function<void()>&& f);

private:
    friend class Runtime;

    RuntimeSettings settings;
    std::unique_ptr<thread::ThreadPool> tpool;
    std::mutex mtx;
    sync::AtomicBool launched = false;
};

/* RuntimeImpl implementation */

void RuntimeImpl::launch() {
    std::unique_lock lock(mtx);

    ASP_ALWAYS_ASSERT(!launched, "cannot launch the same instance of Runtime twice");

    if (settings.threadCount == 0) {
        settings.threadCount = std::thread::hardware_concurrency();
    }

    ASP_ALWAYS_ASSERT(settings.threadCount != 0, "failed to determine the maximum amount of threads on the target machine");
    ASP_ALWAYS_ASSERT(settings.threadCount <= 1024, "cannot launch a Runtime with over 1024 threads");

    tpool = std::make_unique<thread::ThreadPool>(settings.threadCount);

    launched = true;

    asp::trace("async runtime launched");
}

void RuntimeImpl::runAsync(std::function<void()>&& f) {
    std::unique_lock lock(mtx);

    ASP_ALWAYS_ASSERT(launched, "cannot launch a task on a Runtime that isn't running");

    tpool->pushTask(std::move(f));
}

/* Runtime implementation */

Runtime::Runtime() : impl(std::make_unique<RuntimeImpl>(DEFAULT_SETTINGS)) {}

Runtime& Runtime::get() {
    static Runtime runtime;
    return runtime;
}

void Runtime::configure(const RuntimeSettings& settings) {
    ASP_ALWAYS_ASSERT(!impl->launched, "cannot configure a runtime after it has been launched");
    impl->settings = settings;
}

void Runtime::launch() {
    impl->launch();
}

void Runtime::runAsync(std::function<void()>&& f) {
    impl->runAsync(std::move(f));
}

}