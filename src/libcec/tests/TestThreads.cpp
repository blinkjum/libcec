// Regression for consecutive Sony vendor messages restarting an async worker.
// No libCEC connection, GUI, USB adapter or CEC traffic is required.
#include "platform/threads/threads.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>

class WaitingWorker : public CEC::CThread
{
public:
  ~WaitingWorker() override { StopThread(0); }
  void* Process() override
  {
    while (!IsStopped())
      Sleep(1);
    return nullptr;
  }
};

class QuickWorker : public CEC::CThread
{
public:
  ~QuickWorker() override { StopThread(0); }
  void* Process() override { return nullptr; }
};

static bool Check(bool condition, const char* message)
{
  if (!condition)
    std::fprintf(stderr, "FAIL: %s\n", message);
  return condition;
}

int main()
{
  // The old header deadlocks in the second CreateThread(false) when the
  // first ThreadHandler has not yet acquired m_threadMutex.
  WaitingWorker worker;
  for (unsigned int iteration = 0; iteration < 2000; ++iteration)
  {
    if (!Check(worker.CreateThread(false), "initial async start rejected") ||
        !Check(worker.IsRunning(), "pending async start is not reserved") ||
        !Check(!worker.CreateThread(false), "duplicate async start accepted") ||
        !Check(worker.StopThread(1000), "stop of pending/running worker timed out") ||
        !Check(!worker.IsRunning(), "worker still running after stop"))
      return 1;
  }

  // A naturally completed run remains joinable and must still be restartable.
  QuickWorker quick;
  for (unsigned int iteration = 0; iteration < 1000; ++iteration)
  {
    if (!Check(quick.CreateThread(iteration % 2 == 0), "completed worker cannot restart"))
      return 1;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (quick.IsRunning() && std::chrono::steady_clock::now() < deadline)
      std::this_thread::yield();
    if (!Check(!quick.IsRunning(), "short worker did not finish"))
      return 1;
  }

  // Multiple callers cannot both reserve one run before its handler enters.
  for (unsigned int iteration = 0; iteration < 200; ++iteration)
  {
    std::atomic<bool> go(false);
    std::atomic<unsigned int> accepted(0);
    std::vector<std::thread> starters;
    for (unsigned int index = 0; index < 4; ++index)
      starters.emplace_back([&] {
        while (!go.load())
          std::this_thread::yield();
        if (worker.CreateThread(false))
          ++accepted;
      });
    go.store(true);
    for (auto& starter : starters)
      starter.join();
    if (!Check(accepted.load() == 1, "concurrent starters did not select exactly one run") ||
        !Check(worker.StopThread(1000), "concurrent-started worker could not stop"))
      return 1;
  }

  // Destruction immediately after an async start must wait for cancellation.
  for (unsigned int iteration = 0; iteration < 1000; ++iteration)
  {
    WaitingWorker pending;
    if (!Check(pending.CreateThread(false), "destruction test could not start"))
      return 1;
  }
  std::puts("PASS: 2000 async duplicate/stop cycles, 1000 natural restarts, "
            "200 concurrent starts, 1000 immediate destructions");
  return 0;
}
