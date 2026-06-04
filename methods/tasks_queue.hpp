/**
 * @file tasks_queue.hpp
 * @author Mikhail Lozhnikov
 *
 * Класс очереди задач.
 */

#ifndef METHODS_TASKS_QUEUE_HPP_
#define METHODS_TASKS_QUEUE_HPP_

#include <queue>
#include <utility>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <abstract_solver_wrapper.hpp>

namespace mm {

/**
 * @brief Класс очереди задач.
 */
class TasksQueue {
 private:
  //! Очередь задач.
  std::queue<std::pair<int, AbstractSolverWrapper*>> tasks;
  //! Данные завершенныхзадач.
  std::unordered_map<int, nlohmann::json> finishedTasksData;
  //дентификатор последней задачи.
  int lastTaskId;
  //! Индикатор завершения очереди обработки задач.
  bool finished;
  //! Мьютекс для внутренних структур.
  std::mutex m;
  //! Условная переменная для начала обработки новой задачи.
  std::condition_variable condvar;
  //! Описатель потока - обработчика задач.
  std::thread queueThread;

 public:
  /**
   * @brief Конструктор очереди задач.
   *
   * Запускает поток-обработчик очереди задач.
   */
  TasksQueue();

  /**
   * @brief Деструктор.
   *
   * Останавливает поток-обработчик очереди задач.
   */
  ~TasksQueue();

  /**
   * @brief Добавить решалку в очередь задач.
   * @param task Решалка, добавляемая в очередь.
   * @return Идентификатор добавленной задачи.
   *
   * Функция добавляет задачу в очередь задач и присваивает ей уникальный
   * идентификатор. 
   */
  int AddTask(AbstractSolverWrapper* task);

  /**
   * @brief Проверить, завершена ли задача.
   * @param id Идентификатор задачи.
   * @return Функция возвращает true, если задача завершена, и false в противном
   * случае.
   */
  bool IsTaskFinished(int id);

  /**
   * @brief Получить данные расчетов завершенной задачи.
   * @param id Идентификатор задачи.
   * @return Данный расчетов задачи в формате JSON.
   *
   * Функция возвращает данные расчетов задачи с указанным идентификатором.
   * Если данные задачи не найдены, то функция генерирует исключение типа
   * std::runtime_error. После выполнения функции информация об указанной
   * задаче, а также всё её данные удаляются. Задача также небудет числиться
   * в списке завершенных задач.
   */
  nlohmann::json GetFinishedTaskData(int id);

 private:
  /**
   * @brief Поток, в котором выполняется очередь задач.
   */
  void ThreadFunction();
};

}  // namespace mm

#endif  // METHODS_TASKS_QUEUE_HPP_

