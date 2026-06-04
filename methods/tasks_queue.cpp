/**
 * @file tasks_queue.cpp
 * @author Mikhail Lozhnikov
 */

#include <chrono>
#include <utility>
#include "tasks_queue.hpp"

namespace mm {

using std::chrono::steady_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

TasksQueue::TasksQueue() :
  lastTaskId(0),
  finished(false),
  queueThread([&]() { return this->ThreadFunction(); } ) {
}


TasksQueue::~TasksQueue() {
  {
    std::unique_lock locker(m);
    finished = true;
    condvar.notify_one();
  }
  queueThread.join();
}

int TasksQueue::AddTask(AbstractSolverWrapper* task) {
  std::unique_lock locker(m);

  tasks.emplace(++lastTaskId, task);

  condvar.notify_one();


  return lastTaskId;
}

bool TasksQueue::IsTaskFinished(int id) {
  std::unique_lock locker(m);

  return finishedTasksData.find(id) != finishedTasksData.end();
}

nlohmann::json TasksQueue::GetFinishedTaskData(int id) {
  std::unique_lock locker(m);

  nlohmann::json data = finishedTasksData.at(id);

  finishedTasksData.erase(id);

  return data;
}

void TasksQueue::ThreadFunction() {
  while (true) {
    std::pair<int, AbstractSolverWrapper*> ticket;

    {
      std::unique_lock locker(m);

      if (finished)
        return;

      if (tasks.empty()) {
        condvar.wait(locker);
        continue;
      }

      ticket = tasks.back();

      tasks.pop();
    }

    auto [currentTaskId, solverWrapper] = ticket;

    nlohmann::json data;

    auto start = steady_clock::now();

    bool status = solverWrapper->Solve(&data);

    auto finish = steady_clock::now();

    delete solverWrapper;

    double elapsed = duration_cast<milliseconds>(finish - start).count() /
        1000.0;

    if (status) {
      data["status"] = "ok";
      data["time"] = elapsed;
    } else {
      data["status"] = "failed";
    }

    std::unique_lock locker(m);

    finishedTasksData[currentTaskId] = data;
  }
}


}  // namespace mm



