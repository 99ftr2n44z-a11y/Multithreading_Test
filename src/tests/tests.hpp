#pragma once
#include <format>
#include <atomic>
#include <latch>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#include <cassert>

struct test_entry {
  void (*run)(const char* name, std::size_t threads_count, std::size_t messages_count){ nullptr };
  const char* name{ "uninitialized" };
  test_entry* next{ nullptr };
};

extern test_entry tests;

void reset_print(const char* name, std::thread::id id);
void check_print(const char* name, std::size_t threads, std::size_t messages, bool managed_thread, double divisor);

template <class Logger>
class test {
public:
  template <unsigned N>
  test(const char (&name)[N]) noexcept :
  entry_{ run, name } {
    entry_.next = tests.next;
    tests.next = &entry_;
  }

  test(test&& other) = delete;
  test& operator=(test&& other) = delete;

  test(const test& other) = delete;
  test& operator=(const test& other) = delete;

private:
  static void run(const char* name, std::size_t threads_count, std::size_t messages_count) {
    if constexpr (requires { requires Logger::tests_divisor > 1; }) {
      messages_count = std::max(std::size_t{ 16 }, messages_count / Logger::tests_divisor);
    }

    assert(threads_count > 0 && threads_count % 2 == 0 && messages_count > 0);
    static constexpr const char* string_literal_text{ "string literal test message" };
    static constexpr std::size_t string_literal_size{
      std::numeric_limits<std::size_t>::digits10 + 1 +  // std::size_t thread
      std::size_t{ 1 } +                                // ':'
      std::numeric_limits<std::size_t>::digits10 + 1 +  // std::size_t index
      std::size_t{ 1 } +                                // ' '
      std::string_view{ string_literal_text }.size() +  // message text
      std::size_t{ 1 }                                  // \0
    };
    const std::size_t threads_block{ threads_count / 2 };

    Logger logger;
    std::latch ready{ static_cast<std::ptrdiff_t>(threads_count) + 1 };
    std::jthread thread([&](std::stop_token stop) {
      reset_print(name, std::this_thread::get_id());
      ready.count_down();
      logger.run(stop);
    });
    std::vector<std::string> strings;
    strings.reserve(threads_block * messages_count);
    for (std::size_t thread = 0; thread < threads_block; thread++) {
      for (std::size_t index = 1; index <= messages_count; index++) {
        strings.emplace_back(std::format("{}:{} {}", thread, index, string_literal_text));
      }
    }
    std::atomic_flag start;
    std::vector<std::jthread> threads;
    threads.reserve(threads_count);
    for (std::size_t thread = 0; thread < threads_block; thread++) {
      threads.emplace_back([&, thread] {
        ready.count_down();
        start.wait(false);
        for (std::size_t index = 1; index <= messages_count; index++) {
          logger.post(strings[thread * messages_count + (index - 1)]);
        }
      });
    }
    for (std::size_t thread = threads_block; thread < threads_block * 2; thread++) {
      threads.emplace_back([&, thread] {
        ready.count_down();
        start.wait(false);
        for (std::size_t index = 1; index <= messages_count; index++) {
          if constexpr (requires { requires Logger::tests_templated_post; }) {
            logger.template post<"{}:{} format test message">(thread, index);
          } else {
            logger.post("{}:{} format test message", thread, index);
          }
        }
      });
    }
    ready.wait();
    start.test_and_set();
    start.notify_all();
    threads.clear();
    thread.request_stop();
    thread.join();
    auto managed_thread = false;
    if constexpr (requires { requires Logger::tests_managed_thread; }) {
      managed_thread = Logger::tests_managed_thread;
    }
    auto divisor = 1.0;
    if constexpr (requires { requires Logger::tests_divisor > 1; }) {
      divisor = static_cast<double>(Logger::tests_divisor);
    }
    check_print(name, threads_count, messages_count, managed_thread, divisor);
  }

  test_entry entry_;
};
