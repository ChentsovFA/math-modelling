/**
 * @file methods/heat_equation_method.cpp
 * @brief Серверный метод для уравнения теплопроводности.
 */

#include <memory>
#include <random>
#include <string>
#include <cmath>
#include <nlohmann/json.hpp>

#include "abstract_solver_wrapper.hpp"
#include "heat_equation_solver.hpp"
#include "tasks_queue.hpp"
#include "methods.hpp"

namespace mm {

    //вспомогательная функция для начальных условий
    static typename HeatEquationSolver<double>::InitialFunc
        MakeInitial(const std::string& type, size_t M = 0) {
        if (type == "zero" || type.empty()) {
            return nullptr;
        }
        else if (type == "random") {
            auto rng = std::make_shared<std::mt19937>(std::random_device{}());
            auto dist = std::make_shared<std::uniform_real_distribution<double>>(-5.0, 5.0);
            return [rng, dist](size_t, size_t) { return (*dist)(*rng); };
        }
        else if (type == "sin") {
            if (M == 0) return nullptr;
            return [M](size_t i, size_t j) {
                double x = static_cast<double>(j) / static_cast<double>(3 * M);
                double y = static_cast<double>(i) / static_cast<double>(3 * M);
                const double pi = std::acos(-1.0);
                return std::sin(pi * x) * std::sin(pi * y);
                };
        }
        return nullptr;
    }

    int HeatEquationMethod(const nlohmann::json& input,
        nlohmann::json* output,
        mm::TasksQueue& tasksQueue) {
        size_t M = input.at("M").get<size_t>();
        double tau = input.at("tau").get<double>();
        double finishTime = input.at("finishTime").get<double>();
        double exportPeriod = input.at("exportPeriod").get<double>();

        // Количество потоков (по умолчанию 4)
        size_t numThreads = input.value("num_threads", 4);

        // Вот и начальное условие
        std::string initType = input.value("initial", "zero");
        auto initFunc = MakeInitial(initType, M);

        //создание решателя и обёртки
        auto* solver = new mm::HeatEquationSolver<double>(
            M, tau, finishTime, exportPeriod, initFunc, numThreads
        );
        auto* wrapper = new mm::DoubleAbstractSolverWrapper(solver);

        //добавление в нашу очередь
        int taskId = tasksQueue.AddTask(wrapper);

        // Сохранение ID в выходной JSON
        (*output)["id"] = taskId;

        return taskId;
    }

}  // namespace mm