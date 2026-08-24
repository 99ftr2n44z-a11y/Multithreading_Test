#include <logger.hpp>
#include <functional>
#include <mutex>
#include <thread>
#include <cstdio>

static std::mutex mutex;

void (*print)(std::string_view message) = [](std::string_view message) {
  std::lock_guard lock{ mutex };
  std::fwrite(message.data(), sizeof(char), message.size(), stdout);
  std::fputc('\n', stdout);
  std::fflush(stdout);
};

int main(int argc, char* argv[]) {
  logger log;
  const std::jthread thread{ std::bind_front(&logger::run, &log) };
  log.post("string literal");
  log.post("string format: {}", argc);
}
