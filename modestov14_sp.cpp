// modestov14_sp.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <windows.h>
#include <iostream>
#include <cstdio>
#include <cstdlib>

const int MAX_LOGS = 1000;
const int MAX_TEXT = 128;

struct LogRecord {
    DWORD threadId;
    int priority;
    DWORD tick;
    char message[MAX_TEXT];
};

struct LogBuffer {
    LogRecord records[MAX_LOGS];
    LONG index;
};

LogBuffer buf = {};
CRITICAL_SECTION cs;
bool CS;

const int priorities[4] = {
    THREAD_PRIORITY_HIGHEST,
    THREAD_PRIORITY_NORMAL,
    THREAD_PRIORITY_BELOW_NORMAL,
    THREAD_PRIORITY_LOWEST
};

DWORD WINAPI logger(LPVOID param) {
    int number = *static_cast<int*>(param);
    HANDLE thread = GetCurrentThread();
    SetThreadPriority(thread, priorities[number - 1]);

    DWORD id = GetCurrentThreadId();
    int priority = GetThreadPriority(thread);
    DWORD count = 0;
    std::srand(GetTickCount() + number);

    while (true) {
        if (CS) EnterCriticalSection(&cs);

        LONG index = buf.index;
        if (index >= MAX_LOGS) {
            if (CS) LeaveCriticalSection(&cs);
            break;
        }

        LogRecord& record = buf.records[index];
        record.threadId = id;
        record.priority = priority;
        record.tick = GetTickCount();
        ++count;
        std::snprintf(record.message, MAX_TEXT, "T%d, запись %lu",
            number, count);
        buf.index = index + 1;

        if (CS) LeaveCriticalSection(&cs);

        Sleep(10 + std::rand() % 91);
    }

    return count;
}

DWORD WINAPI observer(LPVOID) {
    SetThreadPriority(GetCurrentThread(), priorities[3]);

    while (true) {
        LogRecord last[5];

        if (CS) EnterCriticalSection(&cs);

        LONG total = buf.index;
        LONG first = total > 5 ? total - 5 : 0;
        LONG count = total - first;

        for (LONG i = 0; i < count; ++i) {
            last[i] = buf.records[first + i];
        }

        if (CS) LeaveCriticalSection(&cs);

        std::cout << "\nПоследние записи (" << total << "/" << MAX_LOGS << ")\n";
        for (LONG i = 0; i < count; ++i) {
            std::printf("ID %lu | приоритет %d | тик %lu | %.*s\n",
                last[i].threadId, last[i].priority, last[i].tick,
                MAX_TEXT, last[i].message);
        }

        if (total >= MAX_LOGS) break;
        Sleep(100);
    }

    return 0;
}


int main() {
    setlocale(NULL, "RUSSIAN");
  
    std::cout << "Практика 14 журнал событий\n"
        << "1. С критической секцией\n"
        << "2. Без критической секции\n"
        << "Выбери режим: ";

    int mode;
    if (!(std::cin >> mode) || (mode != 1 && mode != 2)) {
        std::cout << "Введи 1 или 2.\n";
        return 1;
    }

    if (!SetProcessAffinityMask(GetCurrentProcess(), 1)) {
        std::cout << "Не удалось выбрать первый логический процессор.\n";
        return 1;
    }

    CS = (mode == 1);
    if (CS) InitializeCriticalSection(&cs);

    HANDLE threads[4] = {};
    DWORD ids[4] = {};
    int numbers[3] = { 1, 2, 3 };
    int started = 0;

    for (int i = 0; i < 4; ++i) {
        if (i < 3) {
            threads[i] = CreateThread(nullptr, 0, logger, &numbers[i],
                0, &ids[i]);
        }
        else {
            threads[i] = CreateThread(nullptr, 0, observer, nullptr,
                0, &ids[i]);
        }

        if (!threads[i]) {
            std::cout << "Ошибка создания потока T" << i + 1 << '\n';
            break;
        }
        ++started;
    }

    if (started > 0) {
        WaitForMultipleObjects(started, threads, TRUE, INFINITE);
    }

    if (started == 4) {
        std::cout << "\nИтоги — "
            << (CS ? "с критической секцией" : "без критической секции")
            << '\n';

        DWORD totalCreated = 0;
        int totalSaved = 0;

        for (int i = 0; i < 3; ++i) {
            DWORD created = 0;
            GetExitCodeThread(threads[i], &created);

            int saved = 0;
            for (LONG j = 0; j < buf.index; ++j) {
                if (buf.records[j].threadId == ids[i]) ++saved;
            }

            std::cout << "T" << i + 1 << " | создано " << created
                << " | сохранилось " << saved << '\n';
            totalCreated += created;
            totalSaved += saved;
        }

        std::cout << "Всего создано: " << totalCreated
            << " | сохранилось: " << totalSaved
            << " | индекс журнала: " << buf.index << '\n';
    }

    for (int i = 0; i < started; ++i) {
        CloseHandle(threads[i]);
    }

    if (CS) DeleteCriticalSection(&cs);
    return started == 4 ? 0 : 1;
}

// Запуск программы: CTRL+F5 или меню "Отладка" > "Запуск без отладки"
// Отладка программы: F5 или меню "Отладка" > "Запустить отладку"

// Советы по началу работы 
//   1. В окне обозревателя решений можно добавлять файлы и управлять ими.
//   2. В окне Team Explorer можно подключиться к системе управления версиями.
//   3. В окне "Выходные данные" можно просматривать выходные данные сборки и другие сообщения.
//   4. В окне "Список ошибок" можно просматривать ошибки.
//   5. Последовательно выберите пункты меню "Проект" > "Добавить новый элемент", чтобы создать файлы кода, или "Проект" > "Добавить существующий элемент", чтобы добавить в проект существующие файлы кода.
//   6. Чтобы снова открыть этот проект позже, выберите пункты меню "Файл" > "Открыть" > "Проект" и выберите SLN-файл.
