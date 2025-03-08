#ifndef THREADPOOL_INL
#define THREADPOOL_INL

namespace threading {
    
// Template function to enqueue tasks and return their future results
template<class F, class... Args>
std::future<typename std::invoke_result<F, Args...>::type> 
ThreadPool::enqueue(F&& f, Args&&... args) {
    // Define the return type based on the function and its arguments
    using return_type = typename std::invoke_result<F, Args...>::type;
    
    // Create a packaged task that binds the function with its arguments
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    
    // Get future result before moving task to queue
    std::future<return_type> result = task->get_future();
    
    {
        std::unique_lock<std::mutex> lock(queueMutex);  // Lock for thread-safe queue access
        if (stop) {
            throw std::runtime_error("Cannot enqueue task on stopped ThreadPool");
        }
        
        // Add wrapped task to queue
        tasks.emplace([task]() { (*task)(); });
    }
    
    condition.notify_one();  // Wake up one waiting thread
    return result;
}

// Parallel execution of a function over a range of iterators
template<typename Iterator, typename Function>
void ThreadPool::parallelFor(Iterator begin, Iterator end, Function func) {
    std::atomic<size_t> nextIndex(0);  // Thread-safe counter for work distribution
    size_t totalSize = std::distance(begin, end);  // Calculate total work items
    
    size_t numThreads = threads.size();  // Get number of available threads
    std::vector<std::future<void>> futures;  // Store futures for synchronization
    
    // Create tasks for each thread
    for (size_t i = 0; i < numThreads; ++i) {
        futures.push_back(enqueue([&nextIndex, begin, totalSize, &func]() {
            while (true) {
                size_t index = nextIndex.fetch_add(1);  // Atomically get next work item
                if (index >= totalSize) break;  // Exit if no more work
                
                // Calculate iterator position and execute function
                auto it = begin;
                std::advance(it, index);
                func(it, index);
            }
        }));
    }
    
    // Wait for all tasks to complete
    for (auto& future : futures) {
        future.wait();
    }
}

}

#endif // THREADPOOL_INL
