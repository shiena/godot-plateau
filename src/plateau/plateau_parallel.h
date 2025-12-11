#pragma once

#include <thread>
#include <vector>
#include <functional>
#include <algorithm>
#include <atomic>

namespace plateau_parallel {

/**
 * Get the number of hardware threads available.
 * Returns at least 1.
 */
inline unsigned int get_num_threads() {
    unsigned int n = std::thread::hardware_concurrency();
    return n > 0 ? n : 4;
}

/**
 * Parallel for loop with automatic range splitting.
 *
 * @param start Start index (inclusive)
 * @param end End index (exclusive)
 * @param func Function to call for each index: void(size_t i)
 * @param min_chunk Minimum elements per thread (default 1000)
 * @param num_threads Number of threads (0 = auto-detect)
 *
 * If the range is smaller than min_chunk, executes serially.
 */
template<typename Func>
void parallel_for(size_t start, size_t end, Func&& func,
                  size_t min_chunk = 1000, unsigned int num_threads = 0) {
    if (end <= start) return;

    size_t range = end - start;

    // Serial execution for small ranges
    if (range < min_chunk) {
        for (size_t i = start; i < end; i++) {
            func(i);
        }
        return;
    }

    if (num_threads == 0) {
        num_threads = get_num_threads();
    }

    // Limit threads based on work available
    size_t max_threads = (range + min_chunk - 1) / min_chunk;
    num_threads = static_cast<unsigned int>(std::min(static_cast<size_t>(num_threads), max_threads));

    if (num_threads <= 1) {
        for (size_t i = start; i < end; i++) {
            func(i);
        }
        return;
    }

    size_t chunk_size = (range + num_threads - 1) / num_threads;
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    for (unsigned int t = 0; t < num_threads; t++) {
        size_t t_start = start + t * chunk_size;
        size_t t_end = std::min(t_start + chunk_size, end);

        if (t_start >= end) break;

        threads.emplace_back([t_start, t_end, &func]() {
            for (size_t i = t_start; i < t_end; i++) {
                func(i);
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }
}

/**
 * Parallel for loop with thread-local accumulators.
 * Useful for reduction operations like summing normals.
 *
 * @param start Start index (inclusive)
 * @param end End index (exclusive)
 * @param init_local Function to create thread-local state: LocalState()
 * @param process Function to process each index: void(size_t i, LocalState& local)
 * @param merge Function to merge thread-local results: void(LocalState& local)
 * @param min_chunk Minimum elements per thread
 * @param num_threads Number of threads (0 = auto-detect)
 */
template<typename LocalState, typename InitFunc, typename ProcessFunc, typename MergeFunc>
void parallel_for_reduce(size_t start, size_t end,
                         InitFunc&& init_local,
                         ProcessFunc&& process,
                         MergeFunc&& merge,
                         size_t min_chunk = 1000,
                         unsigned int num_threads = 0) {
    if (end <= start) return;

    size_t range = end - start;

    // Serial execution for small ranges
    if (range < min_chunk) {
        LocalState local = init_local();
        for (size_t i = start; i < end; i++) {
            process(i, local);
        }
        merge(local);
        return;
    }

    if (num_threads == 0) {
        num_threads = get_num_threads();
    }

    size_t max_threads = (range + min_chunk - 1) / min_chunk;
    num_threads = static_cast<unsigned int>(std::min(static_cast<size_t>(num_threads), max_threads));

    if (num_threads <= 1) {
        LocalState local = init_local();
        for (size_t i = start; i < end; i++) {
            process(i, local);
        }
        merge(local);
        return;
    }

    size_t chunk_size = (range + num_threads - 1) / num_threads;
    std::vector<std::thread> threads;
    std::vector<LocalState> local_states(num_threads);
    threads.reserve(num_threads);

    for (unsigned int t = 0; t < num_threads; t++) {
        size_t t_start = start + t * chunk_size;
        size_t t_end = std::min(t_start + chunk_size, end);

        if (t_start >= end) break;

        local_states[t] = init_local();

        threads.emplace_back([t_start, t_end, &process, &local_states, t]() {
            for (size_t i = t_start; i < t_end; i++) {
                process(i, local_states[t]);
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    // Merge all thread-local results
    for (unsigned int t = 0; t < threads.size(); t++) {
        merge(local_states[t]);
    }
}

/**
 * Parallel transform: apply function to each element of input and store in output.
 * Input and output arrays must be pre-sized.
 *
 * @param size Number of elements
 * @param func Function: void(size_t i) - should read from input[i] and write to output[i]
 * @param min_chunk Minimum elements per thread
 */
template<typename Func>
void parallel_transform(size_t size, Func&& func, size_t min_chunk = 1000) {
    parallel_for(0, size, std::forward<Func>(func), min_chunk);
}

} // namespace plateau_parallel
