/**
 * @file include/abstract_solver.hpp
 * @author Mikhail Lozhnikov
 *
 * Абстрактный класс решалки.
 */

#ifndef INCLUDE_ABSTRACT_SOLVER_HPP_
#define INCLUDE_ABSTRACT_SOLVER_HPP_

#include <nlohmann/json.hpp>

namespace mm {

/**
 * @brief Абстрактный класс решалки.
 *
 * @tparam T Тип данных, используемый для арифметики.
 */
template<typename T>
class AbstractSolver {
 protected:
  //! Текущее время.
  T t;
  //! Шаг по времени.
  T tau;
  //! Конечное время.
  T finishTime;
  //! Временной интервал сохранения данных.
  T exportPeriod;

 public:
  //! Тип данных, используемый для арифметики.
  using ValueType = T;

  /**
   * @brief Конструктор калсса.
   * @param tau Величина шага по времени.
   * @param finishTime Конечное время.
   * @param exportPeriod Временной интервал для сохранения данных.
   */
  AbstractSolver(T tau, T finishTime, T exportPeriod) :
    t(T(0)),
    tau(tau),
    finishTime(finishTime),
    exportPeriod(exportPeriod)
  { }

  /**
   * @brief Виртуальный деструктор.
   */
  virtual ~AbstractSolver() { }

  /**
   * @brief Получить текущее время.
   * @return Текущее время.
   */
  T CurrentTime() const { return t; }

  /**
   * @brief Получить величину шага по времени.
   * @return Шаг по времени.
   */
  T Tau() const { return tau; }

  /**
   * @brief Получить конечное время.
   * @return Конечное время.
   */
  T FinishTime() const { return finishTime; }

  /**
   * @brief Получить временной интервал, после которого нужно делать
   *  сохранение данных.
   * @return Временной интервал сохранения данных.
   */
  T ExportPeriod() const { return exportPeriod; }

  /**
   * @brief Выполнить один шаг по времени.
   *
   * @return Функция возвращает true в случае успешного выполнения шага и
   *  false в противном случае.
   *
   */
  virtual bool MakeStep() = 0;

  /**
   * @brief Функция решает задачу и сохраняет решение в выходной JSON.
   *
   * @param output Выходной JSON.
   * @return Функция возвращает true в случае успешного нахождения решения и
   * false в противном случае.
   */
  bool Solve(nlohmann::json* output);

  /**
   * @brief Получить данные на текущем шаге по времени.
   *
   * @param output Выходной JSON.
   */
  virtual void ExportData(nlohmann::json* output) = 0;
};

}  // namespace mm

#include <abstract_solver_impl.hpp>

#endif  // INCLUDE_ABSTRACT_SOLVER_HPP_


