#include "tests.hpp"
#include <algorithm>
#include <charconv>
#include <chrono>
#include <mutex>
#include <numeric>
#include <ranges>
#include <stdexcept>
#include <unordered_map>
#include <cstdio>

test_entry tests{ nullptr, "root" };

struct print_data {
  unsigned long long index{};
  std::chrono::steady_clock::time_point first;
  std::chrono::steady_clock::time_point last;
};

static std::mutex print_mutex;
static std::unordered_map<std::size_t, print_data> print_info;
static std::thread::id print_thread_id;
static int print_name_size{};

static std::size_t invalid_thread_error{};
static std::size_t invalid_index_error{};
static std::size_t parse_message_error{};
static std::size_t parse_thread_error{};
static std::size_t parse_index_error{};

void reset_print(const char* name, std::thread::id id) {
  std::lock_guard lock{ print_mutex };
  std::fprintf(stderr, "+ %-*s", print_name_size, name);
  print_info.clear();
  print_thread_id = id;
  invalid_thread_error = 0;
  invalid_index_error = 0;
  parse_message_error = 0;
  parse_thread_error = 0;
  parse_index_error = 0;
  std::fflush(stderr);
}

void check_print(const char* name, std::size_t threads, std::size_t messages, bool managed_thread, double divisor) {
  std::lock_guard lock{ print_mutex };
  if (print_info.empty()) {
    std::fprintf(stderr, "\n  0/%zu messages printed\n", messages * threads);
    std::fflush(stderr);
    return;
  }
  const auto values = print_info | std::views::values;
  const auto indices = values | std::views::transform(&print_data::index);
  const auto total = std::reduce(begin(indices), end(indices), std::size_t{ 0 });
  const auto first = std::ranges::min(values, {}, &print_data::first).first;
  const auto last = std::ranges::max(values, {}, &print_data::last).last;
  const auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(last - first).count() * divisor;
  std::fprintf(stderr, "  %7.3lfs\n", duration);
  if (total != messages * threads) {
    std::fprintf(stderr, "  %zu/%zu messages printed\n", total, messages * threads);
  }
  if (print_info.size() != threads) {
    std::fprintf(stderr, "  %zu/%zu threads posted messages\n", print_info.size(), threads);
  }
  if (invalid_thread_error && !managed_thread) {
    std::fprintf(stderr, "  %zu messages from invalid threads\n", invalid_thread_error);
  }
  if (invalid_index_error) {
    std::fprintf(stderr, "  %zu messages with invalid order\n", invalid_index_error);
  }
  if (parse_message_error) {
    std::fprintf(stderr, "  %zu messages with invalid format\n", parse_message_error);
  }
  if (parse_thread_error) {
    std::fprintf(stderr, "  %zu messages with invalid thread format\n", parse_thread_error);
  }
  if (parse_index_error) {
    std::fprintf(stderr, "  %zu messages with invalid index format\n", parse_index_error);
  }
  std::fflush(stderr);
}

void (*print)(std::string_view message) = [](std::string_view message) {
  const auto begin = message.data();
  const auto end = begin + message.size();
  auto it = begin;
  std::size_t thread{ std::numeric_limits<std::size_t>::max() };
  if (auto [ptr, ec] = std::from_chars(it, end, thread); ec != std::errc{} || ptr >= end) {
    parse_thread_error++;
    return;
  } else if (*ptr++ != ':') {
    parse_message_error++;
    return;
  } else {
    it = ptr;
  }
  std::size_t index{ std::numeric_limits<std::size_t>::max() };
  if (auto [ptr, ec] = std::from_chars(it, end, index); ec != std::errc{}) {
    parse_index_error++;
    return;
  }
  std::lock_guard lock{ print_mutex };
  if (const auto id = std::this_thread::get_id(); id != print_thread_id) {
    invalid_thread_error++;
  }
  auto& data = print_info[thread];
  if (index != data.index + 1) {
    invalid_index_error++;
  }
  data.last = std::chrono::steady_clock::now();
  if (!data.index) {
    data.first = data.last;
  }
  data.index++;
};

int main() {
  std::vector<test_entry*> entries;
  for (auto entry = tests.next; entry && entry->run; entry = entry->next) {
    print_name_size = std::max(print_name_size, static_cast<int>(std::string_view{ entry->name }.size()));
    entries.emplace_back(entry);
  }
  std::ranges::sort(entries, {}, [](const test_entry* entry) {
    return std::string_view{ entry->name };
  });
  for (auto entry : entries) {
    entry->run(entry->name, 32,
#ifdef NDEBUG
      1'024 * 64
#else
      1'024
#endif
    );
  }
}
