// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>

#include <atomic>
#include <iostream>
#include <algorithm>
#include <mutex>
#include <unordered_set>
#include <thread>

#include "include/libplatform/libplatform.h"
#include "include/v8.h"

// Geez I haven't been writing any serious C++ code for ages...

class CounterJob: public v8::JobTask {
public:
  static std::atomic<int> global_count;
  static std::unordered_set<unsigned> task_ids;
  static std::unordered_set<std::thread::id> thread_ids;
  static std::mutex task_id_mutex;

  CounterJob(size_t times_to_run): cnt(times_to_run) {}

  void Run(v8::JobDelegate* delegate) override {
    auto guard = std::lock_guard<std::mutex>(print_mutex);
    // task ID does not seem to have any strong association with actual thread it is working on.
    // task IDs could be reused across different tasks.
    std::cout << "From delegate task ID " << unsigned(delegate->GetTaskId()) << std::endl << std::flush;
    cnt--;
    CounterJob::global_count++;
    {
      auto task_ids_guard = std::lock_guard<std::mutex>(task_id_mutex);
      task_ids.insert((unsigned) delegate->GetTaskId());
      thread_ids.insert(std::this_thread::get_id());
    }
  }

  size_t GetMaxConcurrency(size_t worker_count) const override {
    // Supposedly locked by platform itself
    return cnt.load();
  }
private:
  std::atomic<size_t> cnt;
  std::mutex print_mutex;
};

std::atomic<int> CounterJob::global_count(0);
std::unordered_set<unsigned> CounterJob::task_ids{};
std::unordered_set<std::thread::id> CounterJob::thread_ids{};
std::mutex CounterJob::task_id_mutex{};

// A v8::platform::Platform provides services for v8 to use depending on platform.
// For example, it provides task running policies that allows v8 to run tasks (e.g. GC) through it, using PostJob() API.
// It also deals with problems such as page allocation, which allows user to inject custom solutions.

int main(int argc, char* argv[]) {
  auto platform = v8::platform::NewDefaultPlatform();
  size_t times_to_run = argc < 2 ? 500 : std::atoi(argv[1]);
  auto counter_job = std::make_unique<CounterJob>(times_to_run);
  // If kBestEffort, num_worker_threads capped to 2
  // Since task ID is stored in uint32_t using a bit vector, max ID should be 31. (which should normally never be reached, due to kMaxThreadPoolSize = 16)
  // Investigate DefaultJobState::NotifyConcurrencyIncrease().
  // Tasks are appended to a queue where currently running threads will take convenience of popping from them.
  platform->PostJob(v8::TaskPriority::kBestEffort, std::move(counter_job))->Join();
  std::cout << "# expected: " << times_to_run << ", # actual exec: " << CounterJob::global_count.load() << std::endl;
  unsigned max_task_id = 0;
  std::for_each(CounterJob::task_ids.cbegin(), CounterJob::task_ids.cend(), [&max_task_id](const unsigned task_id) {
    max_task_id = std::max(task_id, max_task_id);
  });
  std::cout << "# task IDs: " << CounterJob::task_ids.size() << ", max task ID: " << max_task_id << std::endl;
  // This number should always be no smaller (and often quite larger) than task IDs. Task IDs only ensure that concurrent task runs of the same task should have different IDs.
  std::cout << "# threads: " << CounterJob::thread_ids.size() << std::endl;
}