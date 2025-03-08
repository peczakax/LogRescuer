#include "ThreadPool.h"

// Singleton instance and mutex for thread-safe initialization
std::unique_ptr<threading::ThreadPool, threading::ThreadPool::Deleter> threading::ThreadPool::instance;
std::mutex threading::ThreadPool::instanceMutex;

namespace threading {

// Thread-safe singleton accessor that ensures only one ThreadPool instance exists
ThreadPool& ThreadPool::getInstance(size_t numThreads) {
    std::lock_guard<std::mutex> lock(instanceMutex);  // Lock to prevent concurrent initialization
    if (!instance) {
        instance.reset(new ThreadPool(numThreads));  // Create new instance if none exists
    }
    return *instance;
}

// Initialize thread pool with specified number of worker threads
ThreadPool::ThreadPool(size_t numThreads) {    
    for (size_t i = 0; i < numThreads; ++i) {
        // Create worker threads that continuously process tasks from the queue
        threads.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                
                {
                    std::unique_lock<std::mutex> lock(queueMutex);  // Lock for task queue access
                    condition.wait(lock, [this] { return stop || !tasks.empty(); }); // Wait until there's work to do or pool is stopping
                    
                    // Exit if pool is stopping and no tasks remain
                    if (stop && tasks.empty()) {
                        return;
                    }
                    
                    // Get next task from queue
                    task = std::move(tasks.front());
                    tasks.pop();
                }
                
                activeThreads++;  // Track number of busy threads
                task();          // Execute the task
                activeThreads--; // Decrease active thread count
            }
        });
    }
}

// Clean shutdown of thread pool
ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        stop = true;  // Signal all threads to exit
    }
    
    condition.notify_all();  // Wake up all waiting threads
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

}