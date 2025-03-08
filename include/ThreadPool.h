#ifndef THREADPOOL_H
#define THREADPOOL_H

// Standard library includes for thread pool implementation
#include <vector>
#include <thread>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>

namespace threading {

// Thread pool class that manages a collection of worker threads
class ThreadPool {
private:
    // Custom deleter for the singleton instance
    struct Deleter {
        // Custom deleter for the singleton instance
        void operator()(ThreadPool* p) {
            delete p;
        }
    };

public:
    // Prevent copying and moving of the thread pool instance
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    // Get or create the singleton instance with specified thread count
    static ThreadPool& getInstance(size_t numThreads = std::thread::hardware_concurrency() - 1);

    // Submit a task to the thread pool and receive a future for the result
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type>;

    // Execute a function on each element in a range in parallel
    template<typename Iterator, typename Function>
    void parallelFor(Iterator begin, Iterator end, Function func);

    // Synchronization point to wait for all submitted tasks to complete
    void waitForAll();

    // Returns the number of threads in the pool
    size_t getThreadCount() const { return threads.size(); }

protected:
    // Protected destructor to prevent direct deletion
    ~ThreadPool();

private:
    // Private constructor for singleton pattern
    explicit ThreadPool(size_t numThreads);

    // Singleton instance and synchronization
    static std::unique_ptr<ThreadPool, Deleter> instance;
    static std::mutex instanceMutex;

    std::vector<std::thread> threads;            // Collection of worker threads
    std::queue<std::function<void()>> tasks;     // Queue of pending tasks
    
    std::mutex queueMutex;                       // Protects access to the task queue
    std::condition_variable condition;           // For thread synchronization
    std::atomic<bool> stop{false};               // Signals threads to stop
    std::atomic<size_t> activeThreads{0};        // Number of currently executing tasks
};

}

// Include template implementations
#include "ThreadPool.inl"

#endif // THREADPOOL_H