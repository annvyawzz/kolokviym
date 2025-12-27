@echo off
chcp 65001 > nul
echo ========================================
echo     Тестирование To-Do List API
echo          (для Windows)
echo ========================================
echo.

echo [1] Проверка сервера:
curl -X GET http://127.0.0.1:8080/
timeout /t 2 > nul
echo.

echo [2] Получить все задачи:
curl -X GET http://127.0.0.1:8080/tasks
timeout /t 2 > nul
echo.

echo [3] Создать новую задачу:
curl -X POST http://127.0.0.1:8080/tasks ^
  -H "Content-Type: application/json" ^
  -d "{\"title\":\"Купить молоко\",\"description\":\"3.2%% жирности\",\"status\":\"todo\"}"
timeout /t 2 > nul
echo.

echo [4] Создать еще одну задачу:
curl -X POST http://127.0.0.1:8080/tasks ^
  -H "Content-Type: application/json" ^
  -d "{\"title\":\"Сделать API\",\"description\":\"Для коллоквиума\",\"status\":\"in_progress\"}"
timeout /t 2 > nul
echo.

echo [5] Получить все задачи (после создания):
curl -X GET http://127.0.0.1:8080/tasks
timeout /t 2 > nul
echo.

echo [6] Получить задачу по ID (ID=1):
curl -X GET http://127.0.0.1:8080/tasks/1
timeout /t 2 > nul
echo.

echo [7] Обновить статус задачи (PATCH):
curl -X PATCH http://127.0.0.1:8080/tasks/1 ^
  -H "Content-Type: application/json" ^
  -d "{\"status\":\"done\"}"
timeout /t 2 > nul
echo.

echo [8] Получить обновленную задачу:
curl -X GET http://127.0.0.1:8080/tasks/1
timeout /t 2 > nul
echo.

echo [9] Попробовать получить несуществующую задачу (404):
curl -X GET http://127.0.0.1:8080/tasks/999
timeout /t 2 > nul
echo.

echo [10] Получить статистику:
curl -X GET http://127.0.0.1:8080/stats
timeout /t 2 > nul
echo.

echo [11] Удалить задачу:
curl -X DELETE http://127.0.0.1:8080/tasks/1
timeout /t 2 > nul
echo.

echo [12] Проверить что задача удалена:
curl -X GET http://127.0.0.1:8080/tasks
timeout /t 2 > nul
echo.

echo ========================================
echo Тестирование завершено!
echo Открой браузер и перейди по адресу:
echo http://127.0.0.1:8080
echo для просмотра документации API
echo ========================================
pause