/**
 * @file methods/methods.hpp
 * @author chentsovfedor
 *
 * Объявления функций для серверной части алгоритмов.
 */

#ifndef METHODS_METHODS_HPP_
#define METHODS_METHODS_HPP_

#include <nlohmann/json.hpp>

namespace mm {

    // Forward declaration
    class TasksQueue;

    /* Сюда нужно вставить объявление серверной части алгоритма. */
    int HeatEquationMethod(const nlohmann::json& input,
        nlohmann::json* output,
        mm::TasksQueue& tasksQueue);

    /* Конец вставки. */

}  // namespace mm

#endif  // METHODS_METHODS_HPP_