#pragma once

#include "../config.hpp"
#include "Future.hpp"
#include <memory>

namespace asp::async {

class RuntimeImpl;

struct RuntimeSettings {
    // 0 - auto, 1 - single threaded
    size_t threadCount;
};

class Runtime {
public:
    static Runtime& get();

    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;
    Runtime(Runtime&&) = delete;
    Runtime& operator=(Runtime&&) = delete;

    // Set the settings of the async runtime. Throws an error if the runtime has already been launched.
    void configure(const RuntimeSettings& settings);

    // Asynchronously launches the runtime, without blocking the calling thread.
    void launch();

    // Spawns a future in a different thread, returns a handle that allows you to see the progress of the execution.
    template <typename FOut>
    FutureHandle<FOut> spawn(std::function<FOut()>&& func) {
        auto fut = std::make_shared<Future<FOut>>(std::move(func));
        this->runAsync([fut] {
            fut->start();
        });
        return FutureHandle<FOut>(std::move(fut));
    }

    // Spawns a future in a different thread, returns a handle that allows you to see the progress of the execution.
    template <typename FOut>
    FutureHandle<FOut> spawn(const std::function<FOut()>& func) {
        auto fut = std::make_shared<Future<FOut>>(func);
        this->runAsync([fut] {
            fut->start();
        });
        return FutureHandle<FOut>(std::move(fut));
    }

private:
    std::unique_ptr<RuntimeImpl> impl;

    Runtime();

    void runAsync(std::function<void()>&& f);
};


// Equivalent to `Runtime::get().spawn(func)`
template <typename FOut>
FutureHandle<FOut> spawn(std::function<FOut()>&& func) {
    return Runtime::get().spawn(std::move(func));
}

// Equivalent to `Runtime::get().spawn(func)`
template <typename FOut>
FutureHandle<FOut> spawn(const std::function<FOut()>& func) {
    return Runtime::get().spawn(func);
}

template<typename... Futures>
std::tuple<> _join_helper(const Futures&...) {
    return std::make_tuple();
}

template<typename Future, typename... Futures>
auto _join_helper(const Future& future, const Futures&... futures) {
    // join the current future and recursively join the rest
    auto current_result = std::make_tuple(future.join());
    auto remaining_results = _join_helper(futures...);

    return std::tuple_cat(current_result, remaining_results);
}

// Joins all the given futures and returns their return values.
template<typename... Futures>
auto join(const Futures&... futures) {
    return _join_helper(futures...);
}

// Like `join`, but for void futures.
template <typename... Futures>
void join_void(const Futures&... futures) {
    (futures.join(), ...);
}

}