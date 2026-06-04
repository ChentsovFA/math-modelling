/**
 * @file tests/heat_equation_test.cpp
 * @brief Модульные тесты для решателя теплопроводности.
 */
#include <httplib.h>
// для HTTP-запросов
#include <cmath>
// для математических функций
#include <chrono>
// для паузы между запросами
#include <iostream>
// для std::cerr
#include <random>
// для случайного начального распределения
#include <thread>
// для многопоточности
#include <nlohmann/json.hpp>
// для работы с JSON
#include "heat_equation_solver.hpp"
// И ещё
#include "test_core.hpp"

 /**
  * @brief Проверка граничных условий.
  */
static void TestBoundaryConditions() {
    const size_t M = 10;
    const double h = 3.0 / (3 * M);
    const double tau = h * h / 4.0;
    const double finishTime = tau;

    mm::HeatEquationSolver<double> solver(M, tau, finishTime, finishTime);
    nlohmann::json result;
    bool ok = solver.Solve(&result);
    REQUIRE(ok);

    auto& frames = result["data"];
    REQUIRE(!frames.empty());

    auto& lastFrame = frames[frames.size() - 1];
    auto grid = lastFrame["data"]["grid"];

    size_t N = 3 * M + 1;

    // Функция для безопасного получения значения (проверка на null)
    auto getValue = [&](size_t i, size_t j) -> double {
        if (grid[i][j].is_null()) {
            return 0.0;  // В вырезе температура не определена
        }
        return grid[i][j].get<double>();
        };

    // Нижняя граница (y=0): u = 0
    if (!solver.IsInCutoutPublic(0, 0)) {
        REQUIRE_CLOSE(getValue(0, 0), 0.0, 1e-9);
    }
    if (!solver.IsInCutoutPublic(0, N - 1)) {
        REQUIRE_CLOSE(getValue(0, N - 1), 0.0, 1e-9);
    }
    if (!solver.IsInCutoutPublic(0, N / 2)) {
        REQUIRE_CLOSE(getValue(0, N / 2), 0.0, 1e-9);
    }

    // Левая граница (x=0): u = y
    if (!solver.IsInCutoutPublic(N - 1, 0)) {
        REQUIRE_CLOSE(getValue(N - 1, 0), 3.0, 1e-9);
    }
    if (!solver.IsInCutoutPublic(N / 2, 0)) {
        REQUIRE_CLOSE(getValue(N / 2, 0), 1.5, 1e-9);
    }

    // Правая верхняя часть (x=3, y∈[2,3]): u = 4
    size_t y2 = static_cast<size_t>(2.0 / h + 0.5);
    if (!solver.IsInCutoutPublic(N - 1, N - 1)) {
        REQUIRE_CLOSE(getValue(N - 1, N - 1), 4.0, 1e-9);
    }
    if (!solver.IsInCutoutPublic(y2, N - 1)) {
        REQUIRE_CLOSE(getValue(y2, N - 1), 4.0, 1e-9);
    }

    // Правая нижняя часть (x=3, y∈[0,1]): u = -y
    size_t y1 = static_cast<size_t>(1.0 / h + 0.5);
    if (!solver.IsInCutoutPublic(0, N - 1)) {
        REQUIRE_CLOSE(getValue(0, N - 1), 0.0, 1e-9);
    }
    if (!solver.IsInCutoutPublic(y1, N - 1)) {
        REQUIRE_CLOSE(getValue(y1, N - 1), -1.0, 1e-9);
    }

    // Верхняя часть выреза (y=2, x∈[2,3]): u = 1 + x
    size_t x2 = static_cast<size_t>(2.0 / h + 0.5);
    size_t x3 = static_cast<size_t>(3.0 / h + 0.5);
    if (!solver.IsInCutoutPublic(y2, x2)) {
        REQUIRE_CLOSE(getValue(y2, x2), 3.0, 1e-9);
    }
    if (!solver.IsInCutoutPublic(y2, x3)) {
        REQUIRE_CLOSE(getValue(y2, x3), 4.0, 1e-9);
    }

    // Нижняя часть выреза (y=1, x∈[2,3]): u = 2 - x
    if (!solver.IsInCutoutPublic(y1, x2)) {
        REQUIRE_CLOSE(getValue(y1, x2), 0.0, 1e-9);
    }
    if (!solver.IsInCutoutPublic(y1, x3)) {
        REQUIRE_CLOSE(getValue(y1, x3), -1.0, 1e-9);
    }
}

/**
 * @brief Проверка отсутствия NaN/Inf в решении.
 */
static void TestStability() {
    const size_t M = 8;
    const double h = 3.0 / (3 * M);
    const double tau = h * h / 4.0;
    const double finishTime = 0.05;

    mm::HeatEquationSolver<double> solver(M, tau, finishTime, finishTime / 2);
    nlohmann::json result;
    bool ok = solver.Solve(&result);
    REQUIRE(ok);

    for (auto& frame : result["data"]) {
        auto grid = frame["data"]["grid"];
        for (auto& row : grid) {
            for (auto& val : row) {
                if (!val.is_null()) {
                    double v = val.get<double>();
                    REQUIRE(!std::isnan(v));
                    REQUIRE(!std::isinf(v));
                }
            }
        }
    }
}

/**
 * @brief Проверка принципа максимума.
 */
static void TestMaximumPrinciple() {
    const size_t M = 10;
    const double h = 3.0 / (3 * M);
    const double tau = h * h / 4.0;
    const double finishTime = 0.1;

    mm::HeatEquationSolver<double> solver(M, tau, finishTime, finishTime / 2);
    nlohmann::json result;
    bool ok = solver.Solve(&result);
    REQUIRE(ok);

    double max_val = -1e9;
    double min_val = 1e9;

    for (auto& frame : result["data"]) {
        auto grid = frame["data"]["grid"];
        for (auto& row : grid) {
            for (auto& val : row) {
                if (!val.is_null()) {
                    double v = val.get<double>();
                    if (v > max_val) max_val = v;
                    if (v < min_val) min_val = v;
                }
            }
        }
    }

    // Максимум не должен превышать максимальное граничное значение (4)
    REQUIRE(max_val <= 4.0 + 1e-9);
    // Минимум не должен быть меньше минимального граничного значения (-1)
    REQUIRE(min_val >= -1.0 - 1e-9);
}

/**
 * @brief Тест со случайным начальным распределением.
 */
static void TestRandomInitial() {
    const size_t M = 8;
    const double h = 3.0 / (3 * M);
    const double tau = h * h / 4.0;
    const double finishTime = 0.05;

    std::mt19937 rng(19042005);
    std::uniform_real_distribution<double> dist(-5.0, 5.0);

    auto init_func = [&](size_t, size_t) { return dist(rng); };

    mm::HeatEquationSolver<double> solver(
    M, tau, finishTime, finishTime, init_func);
    nlohmann::json result;
    bool ok = solver.Solve(&result);
    REQUIRE(ok);

    for (auto& frame : result["data"]) {
        auto grid = frame["data"]["grid"];
        for (auto& row : grid) {
            for (auto& val : row) {
                if (!val.is_null()) {
                    double v = val.get<double>();
                    REQUIRE(!std::isnan(v));
                    REQUIRE(!std::isinf(v));
                    REQUIRE(fabs(v) < 1e6);
                }
            }
        }
    }
}

/**
 * @brief HTTP-тест взаимодействия с сервером.
 */
static void TestHttpWorkflow() {
    // Используем httplib без using namespace
    httplib::Client cli("localhost", 8080);

    // Проверяем, запущен ли сервер
    auto chk = cli.Post("/CheckTaskStatus", "{}", "application/json");
    if (!chk) {
        std::cerr << "Skipping HTTP test (server not reachable)\n";
        return;
    }

    nlohmann::json req;
    req["M"] = 10;
    req["tau"] = 0.0002;
    req["finishTime"] = 0.02;
    req["exportPeriod"] = 0.01;
    req["num_threads"] = 2;
    req["initial"] = "zero";

    auto post = cli.Post("/HeatEquation", req.dump(), "application/json");
    REQUIRE(post != nullptr);
    REQUIRE(post->status == 200);

    auto json = nlohmann::json::parse(post->body);
    REQUIRE(json.contains("id"));
    int task_id = json["id"];

    // Ожидаем завершения
    for (int i = 0; i < 100; ++i) {
        auto st = cli.Post("/CheckTaskStatus",
            nlohmann::json{ {"id", task_id} }.dump(),
            "application/json");
        REQUIRE(st != nullptr);
        auto st_json = nlohmann::json::parse(st->body);
        if (st_json.value("status", "") == "finished") break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Скачиваем данные
    auto down = cli.Post("/DownloadTaskData",
        nlohmann::json{ {"id", task_id} }.dump(),
        "application/json");
    REQUIRE(down != nullptr);
    auto result = nlohmann::json::parse(down->body);

    // Проверяем структуру
    REQUIRE(result.contains("data"));
    REQUIRE(!result["data"].empty());
    REQUIRE(result["data"][0]["data"].contains("grid"));
}

/**
 * @brief Главная тестовая функция.
 */
void TestHeatEquation() {
    TestSuite suite("Heat Equation Solver");

    RUN_TEST(suite, TestBoundaryConditions);
    RUN_TEST(suite, TestStability);
    RUN_TEST(suite, TestMaximumPrinciple);
    RUN_TEST(suite, TestRandomInitial);
    RUN_TEST(suite, TestHttpWorkflow);
}
