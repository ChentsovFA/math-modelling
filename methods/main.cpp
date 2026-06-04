/**
 * @file methods/main.cpp
 * @author Mikhail Lozhnikov
 *
 * Файл с функцией main() для серверной части программы.
 */

#include <httplib.h>
#include <iostream>
#include <cstdio>
#include <string>
#include <nlohmann/json.hpp>
#include "tasks_queue.hpp"
#include "methods.hpp"

using json = nlohmann::json;

int main(int argc, char* argv[]) {
    // Порт по-умолчанию.
    int port = 8080;

    if (argc >= 2) {
        // Меняем порт по умолчанию, если предоставлен соответствующий
        // аргумент командной строки.
        if (std::sscanf(argv[1], "%d", &port) != 1)
            return -1;
    }

    std::cerr << "Listening on port " << port << "..." << std::endl;

    httplib::Server svr;
    mm::TasksQueue tasksQueue;

    // Обработчик для GET запроса по адресу /stop.
    svr.Get("/stop", [&](const httplib::Request&, httplib::Response& res) {
        svr.stop();
        res.set_content("Stopped", "text/plain");
        });

    svr.Post("/CheckTaskStatus", [&](const httplib::Request& req,
        httplib::Response& res) {
            nlohmann::json input;
            try {
                input = nlohmann::json::parse(req.body);
            }
            catch (...) {
                input = nlohmann::json::object();
            }

            nlohmann::json output;
            int taskId = input.value("id", 0);
            output["id"] = taskId;

            if (tasksQueue.IsTaskFinished(taskId)) {
                output["status"] = "finished";
            } else {
                output["status"] = "unknown";
            }

            res.set_content(output.dump(), "application/json");
        });

    svr.Post("/DownloadTaskData", [&](const httplib::Request& req,
        httplib::Response& res) {
            nlohmann::json input = nlohmann::json::parse(req.body);
            nlohmann::json output;
            int taskId = input["id"];

            if (tasksQueue.IsTaskFinished(taskId)) {
                output = tasksQueue.GetFinishedTaskData(taskId);
            } else {
                output["status"] = "unknown";
            }

            output["id"] = taskId;
            res.set_content(output.dump(), "application/json");
        });

    /* Сюда нужно вставить обработчик post запроса для алгоритма. */
    svr.Post("/HeatEquation", [&](const httplib::Request& req,
        httplib::Response& res) {
            nlohmann::json input = nlohmann::json::parse(req.body);
            nlohmann::json output;
            int taskId = mm::HeatEquationMethod(input, &output, tasksQueue);
            output["id"] = taskId;
            res.set_content(output.dump(), "application/json");
        });
    /* Конец вставки. */

    // Эта функция запускает сервер на указанном порту.
    svr.listen("0.0.0.0", port);

    return 0;
}
