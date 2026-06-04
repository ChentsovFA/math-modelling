/**
 * @file abstract_solver_impl.hpp
 * @author Mikhail Lozhnikov
 */

#ifndef INCLUDE_ABSTRACT_SOLVER_IMPL_HPP_
#define INCLUDE_ABSTRACT_SOLVER_IMPL_HPP_

#include <abstract_solver.hpp>

namespace mm {

template<typename T>
bool AbstractSolver<T>::Solve(nlohmann::json* output) {
  nlohmann::json& data = *output;
  int numExports = 0;

  for (t = 0; t < finishTime; t += tau) {
    if (!MakeStep()) {
      *output = data;
      return false;
    }

    if (numExports * exportPeriod <= t) {
      data["data"][numExports]["time"] = t;
      ExportData(&data["data"][numExports]["data"]);
      numExports++;
    }
  }

  data["data"][numExports]["time"] = t;
  ExportData(&data["data"][numExports]["data"]);

  return true;
}

}  // namespace mm

#endif  // INCLUDE_ABSTRACT_SOLVER_IMPL_HPP_



