#ifndef INCLUDE_HEAT_EQUATION_SOLVER_HPP_
#define INCLUDE_HEAT_EQUATION_SOLVER_HPP_

/**
 * @file include/heat_equation_solver.hpp
 * @author Chentsovfedor
 * @brief Решение уравнения теплопроводности на квадрате с вырезом.
 *
 * Область: квадрат [0,3]×[0,3] с вырезанным квадратом [2,3]×[1,2].
 * Используется явная разностная схема с параллелизацией через std::thread.
 *
 * Граничные условия:
 * - Нижняя граница (y=0): u = 0
 * - Левая граница (x=0): u = y
 * - Верхняя граница (y=3): ∂u/∂y = 0 (Нейман)
 * - Правая верхняя часть (x=3, y∈[2,3]): u = 4
 * - Верхняя часть выреза (y=2, x∈[2,3]): u = 1 + x
 * - Левая часть выреза (x=2, y∈[1,2]): ∂u/∂x = 0 (Нейман)
 * - Нижняя часть выреза (y=1, x∈[2,3]): u = 2 - x
 * - Правая нижняя часть (x=3, y∈[0,1]): u = -y
 */

#include <vector>
#include <thread>
#include <functional>
#include <cmath>
#include <nlohmann/json.hpp>
#include <abstract_solver.hpp>

namespace mm {

    /**
     * @brief Решатель уравнения теплопроводности на квадрате с вырезом.
     *
     * @tparam T Тип данных (float/double)
     */
    template<typename T>
    class HeatEquationSolver : public AbstractSolver<T> {
    public:
        using InitialFunc = std::function<T(size_t, size_t)>;

        /**
         * @brief Конструктор.
         * @param M Число точек на единицу длины (всего точек = 3*M+1)
         * @param tau Шаг по времени
         * @param finishTime Конечное время
         * @param exportPeriod Период экспорта
         * @param init Начальное условие
         * @param numThreads Количество потоков
         */
        HeatEquationSolver(size_t M, T tau, T finishTime, T exportPeriod,
            InitialFunc init = nullptr, size_t numThreads = 4)
            : AbstractSolver<T>(tau, finishTime, exportPeriod),
            M_(M),
            numThreads_(numThreads),
            h_(static_cast<T>(3.0) / static_cast<T>(3 * M)),
            u_((3 * M + 1)* (3 * M + 1)),
            u_next_((3 * M + 1)* (3 * M + 1)),
            initial_(init) {
            InitializeArrays();
        }

        /**
         * @brief Проверка, находится ли точка в вырезе (публичный метод для тестов).
         * @param i Индекс по y
         * @param j Индекс по x
         * @return true если точка в вырезе [2,3]×[1,2], false иначе
         */
        bool IsInCutoutPublic(size_t i, size_t j) const {
            return IsInCutout(i, j);
        }

        bool MakeStep() override;
        void ExportData(nlohmann::json* output) override;

    private:
        size_t M_;                    // Число разбиений на единицу длины
        size_t numThreads_;           // Количество потоков
        T h_;                         // Шаг сетки
        std::vector<T> u_;            // Текущий слой
        std::vector<T> u_next_;       // Следующий слой
        InitialFunc initial_;         // Начальное условие

        // Размеры сетки
        size_t Nx() const { return 3 * M_ + 1; }
        size_t Ny() const { return 3 * M_ + 1; }

        // Индексация: i - по y, j - по x
        size_t Index(size_t i, size_t j) const { return i * Nx() + j; }

        // Координаты узлов сетки
        T X(size_t j) const { return static_cast<T>(j) * h_; }
        T Y(size_t i) const { return static_cast<T>(i) * h_; }

        /**
         * @brief Проверка, находится ли точка в вырезе.
         *
         * Вырез: [2,3]×[1,2]
         *
         * @param i Индекс по оси Y (строка сетки)
         * @param j Индекс по оси X (столбец сетки)
         * @return true - точка в вырезе, false - точка не в вырезе
         */
        bool IsInCutout(size_t i, size_t j) const {
            T x = X(j);
            T y = Y(i);
            return (x >= 2.0 - 1e-12 && x <= 3.0 + 1e-12 &&
                y >= 1.0 - 1e-12 && y <= 2.0 + 1e-12);
        }

        /**
         * @brief Проверка, является ли точка внутренней (не граница и не вырез)
         *
         * @param i Индекс по оси Y (строка сетки)
         * @param j Индекс по оси X (столбец сетки)
         * @return true - точка внутри области и не на границе,
         *         false - точка на границе или в вырезе
         */
        bool IsInterior(size_t i, size_t j) const {
            if (IsInCutout(i, j)) return false;
            if (i == 0 || i == Ny() - 1 || j == 0 || j == Nx() - 1) return false;
            return true;
        }

        /**
         * @brief Инициализация сетки начальными значениями
         *
         * Устанавливает начальные значения температуры во всех узлах сетки:
         * - В вырезанной области: 0
         * - На границах Дирихле: согласно граничным условиям
         * - Внутри области: из функции начального условия (initial_)
         *
         * @note Граничные условия Неймана не применяются на этом этапе,
         *       они устанавливаются после каждого шага по времени
         */
        void InitializeArrays() {
            size_t N = Nx();
            for (size_t i = 0; i < N; ++i) {
                for (size_t j = 0; j < N; ++j) {
                    if (IsInCutout(i, j)) {
                        u_[Index(i, j)] = 0;
                        continue;
                    }

                    T x = X(j), y = Y(i);

                    // Нижняя граница: u = 0
                    if (std::abs(y) < 1e-12 && x >= 0 && x <= 3.0) {
                        u_[Index(i, j)] = 0;
                    }
                    else if (std::abs(x) < 1e-12 && y >= 0 && y <= 3.0) {
                        // Левая граница: u = y
                        u_[Index(i, j)] = y;
                    }
                    else if (std::abs(x - 3.0) < 1e-12 && y >= 2.0 && y <= 3.0) {
                        // Правая верхняя часть: u = 4
                        u_[Index(i, j)] = 4;
                    }
                    else if (std::abs(y - 2.0) < 1e-12 && x >= 2.0 && x <= 3.0 &&
                        !IsInCutout(i, j)) {
                        // Верхняя часть выреза: u = 1 + x
                        u_[Index(i, j)] = 1 + x;
                    }
                    else if (std::abs(y - 1.0) < 1e-12 && x >= 2.0 && x <= 3.0 &&
                        !IsInCutout(i, j)) {
                        // Нижняя часть выреза: u = 2 - x
                        u_[Index(i, j)] = 2 - x;
                    }
                    else if (std::abs(x - 3.0) < 1e-12 && y >= 0 && y <= 1.0) {
                        // Правая нижняя часть: u = -y
                        u_[Index(i, j)] = -y;
                    }
                    else if (initial_) {
                        u_[Index(i, j)] = initial_(i, j);
                    }
                    else {
                        u_[Index(i, j)] = 0;
                    }
                }
            }
        }

        /**
         * @brief Вычисление одного шага по времени для диапазона строк
         *        (для параллельных потоков)
         *
         * Для каждой внутренней точки в указанном диапазоне строк вычисляет
         * новое значение температуры по явной разностной схеме:
         * u_{i,j}^{n+1} = u_{i,j}^n + τ * Δu_{i,j}^n
         *
         * @param startRow Начальная строка (включительно)
         * @param endRow Конечная строка (исключительно)
         *
         * @note Потоки не конфликтуют, так как каждый обрабатывает свой
         *       непересекающийся диапазон строк
         */
        void MakeStepRange(size_t startRow, size_t endRow) {
            T tau = this->tau;
            T h2 = h_ * h_;
            T coeff = tau / h2;
            size_t N = Nx();

            for (size_t i = startRow; i < endRow; ++i) {
                for (size_t j = 1; j < N - 1; ++j) {
                    if (!IsInterior(i, j)) continue;

                    T laplacian = (u_[Index(i - 1, j)] - 2 * u_[Index(i, j)] +
                        u_[Index(i + 1, j)]) / h2 +
                        (u_[Index(i, j - 1)] - 2 * u_[Index(i, j)] +
                            u_[Index(i, j + 1)]) / h2;

                    u_next_[Index(i, j)] = u_[Index(i, j)] + tau * laplacian;
                }
            }
        }
    };

    template<typename T>
    bool HeatEquationSolver<T>::MakeStep() {
        size_t N = Nx();

        // Проверка условия устойчивости: τ ≤ h²/4
        T maxTau = h_ * h_ / 4.0;
        if (this->tau > maxTau + 1e-12) {
            return false;
        }

        // Параллельное вычисление с использованием std::thread
        std::vector<std::thread> threads;
        size_t rowsPerThread = (N - 2) / numThreads_;
        if (rowsPerThread < 1) rowsPerThread = 1;

        for (size_t t = 0; t < numThreads_; ++t) {
            size_t startRow = 1 + t * rowsPerThread;
            size_t endRow = (t == numThreads_ - 1) ? N - 1 : startRow + rowsPerThread;

            threads.emplace_back([this, startRow, endRow]() {
                MakeStepRange(startRow, endRow);
                });
        }

        // Ожидание завершения всех потоков
        for (auto& th : threads) {
            th.join();
        }

        // Применение граничных условий Дирихле
        for (size_t i = 0; i < N; ++i) {
            for (size_t j = 0; j < N; ++j) {
                if (IsInCutout(i, j)) continue;

                T x = X(j), y = Y(i);

                // Нижняя граница: u = 0
                if (std::abs(y) < 1e-12 && x >= 0 && x <= 3.0) {
                    u_next_[Index(i, j)] = 0;
                }
                else if (std::abs(x) < 1e-12 && y >= 0 && y <= 3.0) {
                    // Левая граница: u = y
                    u_next_[Index(i, j)] = y;
                }
                else if (std::abs(x - 3.0) < 1e-12 && y >= 2.0 && y <= 3.0) {
                    // Правая верхняя часть: u = 4
                    u_next_[Index(i, j)] = 4;
                }
                else if (std::abs(y - 2.0) < 1e-12 && x >= 2.0 && x <= 3.0 &&
                    !IsInCutout(i, j)) {
                    // Верхняя часть выреза: u = 1 + x
                    u_next_[Index(i, j)] = 1 + x;
                }
                else if (std::abs(y - 1.0) < 1e-12 && x >= 2.0 && x <= 3.0 &&
                    !IsInCutout(i, j)) {
                    // Нижняя часть выреза: u = 2 - x
                    u_next_[Index(i, j)] = 2 - x;
                }
                else if (std::abs(x - 3.0) < 1e-12 && y >= 0 && y <= 1.0) {
                    // Правая нижняя часть: u = -y
                    u_next_[Index(i, j)] = -y;
                }
            }
        }

        // Применение условий Неймана

        // Верхняя граница: ∂u/∂y = 0 → u(i,j) = u(i-1,j)
        for (size_t j = 1; j < N - 1; ++j) {
            size_t i = N - 1;
            if (!IsInCutout(i, j)) {
                u_next_[Index(i, j)] = u_next_[Index(i - 1, j)];
            }
        }

        // Левая часть выреза: ∂u/∂x = 0 → u(i,j) = u(i,j+1)
        size_t cutoutLeftJ = static_cast<size_t>(2.0 / h_ + 0.5);
        for (size_t i = 1; i < N - 1; ++i) {
            if (!IsInCutout(i, cutoutLeftJ)) {
                u_next_[Index(i, cutoutLeftJ)] = u_next_[Index(i, cutoutLeftJ + 1)];
            }
        }

        // Копирование следующего слоя в текущий
        u_.swap(u_next_);

        return true;
    }

    template<typename T>
    void HeatEquationSolver<T>::ExportData(nlohmann::json* output) {
        size_t N = Nx();
        nlohmann::json grid = nlohmann::json::array();

        for (size_t i = 0; i < N; ++i) {
            nlohmann::json row = nlohmann::json::array();
            for (size_t j = 0; j < N; ++j) {
                if (IsInCutout(i, j)) {
                    row.push_back(nullptr);
                }
                else {
                    row.push_back(u_[Index(i, j)]);
                }
            }
            grid.push_back(row);
        }

        (*output)["grid"] = grid;
        (*output)["M"] = M_;
        (*output)["h"] = h_;
        (*output)["x_size"] = N;
        (*output)["y_size"] = N;
    }

}  // namespace mm

#endif  // INCLUDE_HEAT_EQUATION_SOLVER_HPP_