#pragma once

#include "Macros.hpp"
#include "../config.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <condition_variable>

namespace asp::async {

template <typename Out = void>
class Future {
public:
    using Task = std::function<Out()>;

    Future(Task&& func) : task(std::move(func)) {}
    Future(const Task& func) : task(func) {}

    Future(const Future&) = delete;
    Future& operator=(const Future&) = delete;

    void start() {
        {
            std::unique_lock lock(mtx);

            ASP_ASSERT(!running, "cannot start the same Future twice");
            ASP_ASSERT(!finished, "cannot restart a Future that was already finished");
            ASP_ALWAYS_ASSERT(task, "cannot start an empty Future");

            running = true;
        }

        // this line of code will haunt you in your dreams
        // *new (&resultBuf) Out = std::move(task());

        auto result = task();

        std::unique_lock lock(mtx);
        finished = true;
        running = false;

        *new (&resultBuf) Out = std::move(result);

        cvar.notify_all();
    }

    bool isRunning() const {
        std::unique_lock lock(mtx);
        return running;
    }

    bool hasFinished() const {
        std::unique_lock lock(mtx);
        return finished;
    }

    // Gets the result of the future, if it has finished. If it has not, throws an error.
    Out& getResult() const {
        std::unique_lock lock(mtx);

        ASP_ALWAYS_ASSERT(finished, "cannot get a result of a Future that hasn't yet been finished");

        return _res();
    }

    // Joins the future, blocking until it is complete and then returns a reference to the return value.
    Out& join() const {
        std::unique_lock lock(mtx);

        if (finished) {
            return _res();
        }

        cvar.wait(lock, [this] { return finished; });

        return _res();
    }

private:
    Task task;
    mutable std::mutex mtx;
    mutable std::condition_variable cvar;
    bool running = false, finished = false;
    alignas(Out) std::byte resultBuf[sizeof(Out)];

    Out& _res() const {
        Out* ptr = const_cast<Out*>(std::launder(reinterpret_cast<const Out*>(&resultBuf)));
        return *ptr;
    }
};

template <typename FOut = void>
class FutureHandle {
public:
    FutureHandle(std::shared_ptr<Future<FOut>> fut) : fut(std::move(fut)) {}

    bool isRunning() const {
        return fut->isRunning();
    }

    bool hasFinished() const {
        return fut->hasFinished();
    }

    FOut& getResult() const {
        return fut->getResult();
    }

    FOut& join() const {
        return fut->join();
    }

    FOut& await() const {
        return this->join();
    }

private:
    std::shared_ptr<Future<FOut>> fut;
};

/* Specialization for void futures */

template <>
class Future<void> {
public:
    using Task = std::function<void()>;

    Future(Task&& func) : task(std::move(func)) {}
    Future(const Task& func) : task(func) {}

    Future(const Future&) = delete;
    Future& operator=(const Future&) = delete;

    void start() {
        {
            std::unique_lock lock(mtx);

            ASP_ASSERT(!running, "cannot start the same Future twice");
            ASP_ASSERT(!finished, "cannot restart a Future that was already finished");
            ASP_ALWAYS_ASSERT(task, "cannot start an empty Future");

            running = true;
        }

        task();

        std::unique_lock lock(mtx);
        finished = true;
        running = false;

        cvar.notify_all();
    }

    bool isRunning() const {
        std::unique_lock lock(mtx);
        return running;
    }

    bool hasFinished() const {
        std::unique_lock lock(mtx);
        return finished;
    }

    // Joins the future, blocking until it is complete and then returns a reference to the return value.
    void join() const {
        std::unique_lock lock(mtx);

        if (finished) {
            return;
        }

        cvar.wait(lock, [this] { return finished; });
    }

private:
    Task task;
    mutable std::mutex mtx;
    mutable std::condition_variable cvar;
    bool running = false, finished = false;
};

template <>
class FutureHandle<void> {
public:
    FutureHandle(std::shared_ptr<Future<void>> fut) : fut(std::move(fut)) {}

    bool isRunning() const {
        return fut->isRunning();
    }

    bool hasFinished() const {
        return fut->hasFinished();
    }

    void join() const {
        fut->join();
    }

    void await() const {
        this->join();
    }

private:
    std::shared_ptr<Future<void>> fut;
};

}