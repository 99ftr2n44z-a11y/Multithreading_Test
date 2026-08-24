#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include "logger.hpp"

// Определяем глобальный указатель print (объявлен в logger.hpp)
void (*print)(std::string_view) = nullptr;

std::atomic<int> printed_count{0};

void my_print(std::string_view message) {
    printed_count.fetch_add(1);
}

int main() {
    const int num_producers = 8;
    const int messages_per_producer = 2000;

    logger log;
    print = my_print;  // назначаем функцию вывода

    std::stop_source stop_source;
    std::stop_token stop_token = stop_source.get_token();

    std::thread consumer([&] {
        log.run(stop_token);
    });

    std::vector<std::thread> producers;
    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back([&, i] {
            for (int j = 0; j < messages_per_producer; ++j) {
                log.post("Producer {} message {}", i, j);
            }
        });
    }

    for (auto& t : producers) {
        t.join();
    }

    stop_source.request_stop();
    consumer.join();

    auto end = std::chrono::steady_clock::now();
    double elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();

    int expected = num_producers * messages_per_producer;
    int actual = printed_count.load();

    std::cout << "Ожидалось сообщений: " << expected << "\n";
    std::cout << "Выведено сообщений:  " << actual << "\n";
    std::cout << "Затраченное время:    " << elapsed_ms << " мс\n";

    if (actual == expected) {
        std::cout << "Тест пройден: все сообщения обработаны.\n";
        return 0;
    } else {
        std::cerr << "Тест провален: потеряно или продублировано сообщений: "
                  << expected - actual << "\n";
        return 1;
    }
}