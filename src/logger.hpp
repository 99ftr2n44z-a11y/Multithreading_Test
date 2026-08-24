#pragma once

#include <condition_variable>
#include <deque>
#include <format>
#include <mutex>
#include <stop_token>
#include <string>
#include <string_view>
#include <utility>

extern void (*print)(std::string_view message);

class logger {
public:
    /// Добавляет сообщение в очередь. Вызывается из многих потоков.
    void post(std::string_view message) {
        // По условию сообщение не превышает 256 символов, но добавим защиту.
        if (message.size() > max_message_size) {
            message = message.substr(0, max_message_size);
        }

        {
            std::lock_guard lock(mutex_);
            queue_.emplace_back(message);
        }
        cv_.notify_one();
    }

    /// Форматирует и добавляет сообщение в очередь.
    template <class... Args>
    void post(std::format_string<Args...> fmt, Args&&... args) {
        post(std::format(fmt, std::forward<Args>(args)...));
    }

    /// Потребитель: вызывается на одном потоке, выводит сообщения.
    void run(std::stop_token stop) {
        std::unique_lock lock(mutex_);

        while (true) {
            // Ждём, пока очередь не станет непустой или не придёт stop.
            cv_.wait(lock, stop, [&] { return !queue_.empty(); });

            if (queue_.empty()) {
                // stop запрошен и очередь пуста — выходим.
                break;
            }

            // Извлекаем сообщение.
            std::string msg = std::move(queue_.front());
            queue_.pop_front();

            // Выводим вне блокировки, чтобы не задерживать производителей.
            lock.unlock();
            print(msg);
            lock.lock();
        }
    }

private:
    static constexpr std::size_t max_message_size = 256;
    std::mutex mutex_;
    std::condition_variable_any cv_;
    std::deque<std::string> queue_;
};