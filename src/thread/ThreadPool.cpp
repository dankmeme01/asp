#include <asp/thread/ThreadPool.hpp>
#include <asp/Log.hpp>

namespace asp::thread {

ThreadPool::ThreadPool(size_t tc) {
#ifdef ASP_ENABLE_FORMAT
    asp::trace("Creating ThreadPool with size {}", tc);
#else
    asp::trace("Creating ThreadPool with size " + std::to_string(tc));
#endif

    for (size_t i = 0; i < tc; i++) {
        Thread<> thread;
        thread.setLoopFunction([this, i = i] {
            auto& worker = this->workers.at(i);

            auto task = this->taskQueue.popTimeout(std::chrono::milliseconds(10));

            if (!task) return;

            worker.doingWork = true;

            task.value()();

            worker.doingWork = false;
        });

        Worker worker = {
            .thread = std::move(thread),
            .doingWork = false
        };

        workers.emplace_back(std::move(worker));
    }

    for (auto& worker : workers) {
        worker.thread.start();
    }
}

ThreadPool::~ThreadPool() {
    asp::trace("Destroying ThreadPool");
    try {
        this->join();

        // stop all threads and wait for them to terminate
        for (auto& worker : workers) {
            worker.thread.stop();
        }

        for (auto& worker : workers) {
            worker.thread.join();
        }

        workers.clear();
    } catch (const std::exception& e) {
        asp::log(LogLevel::Error, std::string("failed to cleanup thread pool: ") + e.what());
    }
}

void ThreadPool::pushTask(const Task& task) {
    taskQueue.push(task);
}

void ThreadPool::pushTask(Task&& task) {
    taskQueue.push(std::move(task));
}

void ThreadPool::join() {
    while (!taskQueue.empty()) {
        std::this_thread::sleep_for(std::chrono::microseconds(200));
    }

    // wait for working threads to finish
    bool stillWorking;

    do {
        std::this_thread::sleep_for(std::chrono::microseconds(200));
        stillWorking = false;
        for (const auto& worker : workers) {
            if (worker.doingWork) {
                stillWorking = true;
                break;
            }
        }
    } while (stillWorking);
}

bool ThreadPool::isDoingWork() {
    if (!taskQueue.empty()) return true;

    for (const auto& worker : workers) {
        if (worker.doingWork) {
            return true;
        }
    }

    return false;
}

void ThreadPool::setExceptionFunction(const std::function<void(const std::exception&)>& f) {
    onException = f;

    for (auto& worker : workers) {
        worker.thread.setExceptionFunction(f);
    }
}

}