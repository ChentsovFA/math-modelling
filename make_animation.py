#!/usr/bin/env python3

"""
Интерактивный генератор анимации для уравнения теплопроводности.
Отправляет задачу на сервер, ожидает завершения, скачивает результат
и строит анимацию.
"""

import http.client
import json
import time
import subprocess
import sys
import os

def ask(prompt, default):
    """Задать вопрос с значением по умолчанию."""
    user = input(f"{prompt} (по умолчанию {default}): ").strip()
    return user if user != "" else default

def check_server(host, port):
    """Проверить, отвечает ли сервер."""
    conn = None
    try:
        conn = http.client.HTTPConnection(host, port, timeout=2)
        conn.request("POST", "/CheckTaskStatus", "{}",
                     {"Content-Type": "application/json"})
        resp = conn.getresponse()
        return resp.status == 200
    except Exception:
        return False
    finally:
        if conn:
            conn.close()

def send_request(host, port, path, body):
    """Отправить POST-запрос и вернуть JSON-ответ."""
    conn = http.client.HTTPConnection(host, port, timeout=30)
    headers = {"Content-Type": "application/json"}
    conn.request("POST", path, json.dumps(body), headers)
    resp = conn.getresponse()
    data = resp.read().decode()
    conn.close()
    
    # Если ответ пустой, вернуть пустой словарь
    if not data or data.strip() == "":
        print(f"Предупреждение: пустой ответ от сервера на {path}")
        return {}
    
    # Попробовать распарсить JSON
    try:
        return json.loads(data)
    except json.JSONDecodeError as e:
        print(f"Ошибка парсинга JSON от {path}: {e}")
        print(f"Полученные данные: {data[:200]}")  # Показать первые 200 символов
        return {}

def main():
    print("=== Интерактивный генератор анимации теплопроводности ===")
    print("Область: квадрат [0,3]×[0,3] с вырезом [2,3]×[1,2]\n")

    # Параметры с значениями по умолчанию
    M = int(ask("Число разбиений M (рекомендуется 20-40)", "30"))
    tau = float(ask("Шаг по времени tau (должен быть <= h²/4)", "0.00025"))
    finish_time = float(ask("Конечное время finishTime", "0.1"))
    export_period = float(ask("Период сохранения exportPeriod", "0.01"))
    num_threads = int(ask("Количество потоков", "4"))
    initial = ask("Начальное условие (zero/random/sin)", "zero")
    output_file = ask("Имя выходного видеофайла", "heat_equation_animation.mp4")

    host = "localhost"
    port = 8080

    # Запуск сервера, если он ещё не работает
    server_process = None
    if not check_server(host, port):
        print("Запускаем сервер...")
        server_exec = os.path.join("build", "math_modelling_server")
        if sys.platform == "win32":
            server_exec += ".exe"
        
        if not os.path.exists(server_exec):
            print(f"Ошибка: сервер не найден по пути {server_exec}")
            print("Сначала соберите проект: cd build && make")
            return 1
        
        server_process = subprocess.Popen([server_exec],
                                          stdout=subprocess.DEVNULL,
                                          stderr=subprocess.DEVNULL)
        time.sleep(2)
    else:
        print("Сервер уже запущен.")

    # Отправка задачи
    print("\nОтправляем задачу на сервер...")
    req_body = {
        "M": M,
        "tau": tau,
        "finishTime": finish_time,
        "exportPeriod": export_period,
        "num_threads": num_threads,
        "initial": initial
    }
    
    try:
        resp = send_request(host, port, "/HeatEquation", req_body)
    except Exception as e:
        print(f"Ошибка при отправке запроса: {e}")
        if server_process:
            server_process.terminate()
        return 1
    
    if "id" not in resp:
        print(f"Ошибка: сервер вернул некорректный ответ: {resp}")
        if server_process:
            server_process.terminate()
        return 1
    
    task_id = resp["id"]
    print(f"Задача зарегистрирована, ID = {task_id}")

    # Ожидание завершения
    print("Ожидаем завершения расчёта...")
    max_attempts = 300
    for attempt in range(max_attempts):
        try:
            status_resp = send_request(host, port, "/CheckTaskStatus",
                                       {"id": task_id})
            if status_resp.get("status") == "finished":
                print("Расчёт завершён.")
                break
        except Exception as e:
            print(f"Ошибка при проверке статуса: {e}")
        
        time.sleep(1)
    else:
        print("Превышено время ожидания завершения задачи.")
        if server_process:
            server_process.terminate()
        return 1

    # Скачивание данных
    print("Скачиваем результаты...")
    try:
        result = send_request(host, port, "/DownloadTaskData",
                              {"id": task_id})
    except Exception as e:
        print(f"Ошибка при скачивании данных: {e}")
        if server_process:
            server_process.terminate()
        return 1
    
    # Сохранение во временный файл
    tmp_json = "temp_heat_result.json"
    with open(tmp_json, "w") as f:
        json.dump(result, f)
    print(f"Данные сохранены в {tmp_json}")

    # Построение анимации
    print("Строим анимацию...")
    plot_script = os.path.join("python", "plot.py")
    
    if not os.path.exists(plot_script):
        print(f"Ошибка: скрипт визуализации не найден по пути {plot_script}")
        os.remove(tmp_json)
        if server_process:
            server_process.terminate()
        return 1
    
    plotter_cmd = [
        sys.executable,
        plot_script,
        "heat_equation",
        tmp_json,
        output_file
    ]
    
    try:
        subprocess.run(plotter_cmd, check=True)
        print(f"Анимация успешно сохранена в {output_file}")
    except subprocess.CalledProcessError as e:
        print(f"Ошибка при создании анимации: {e}")
    except FileNotFoundError:
        print("Ошибка: не найден интерпретатор Python или ffmpeg")
        print("Убедитесь, что установлены: python3, matplotlib, numpy, ffmpeg")

    # Очистка временного файла
    os.remove(tmp_json)

    # Остановка сервера, если запускали сами
    if server_process is not None:
        print("Останавливаем сервер...")
        try:
            send_request(host, port, "/stop", {})
        except Exception:
            pass
        server_process.terminate()
        server_process.wait()
        print("Сервер остановлен.")

    print(f"\nГотово! Видео сохранено как {output_file}")
    return 0

if __name__ == "__main__":
    sys.exit(main())