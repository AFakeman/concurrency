#include <cassert>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

#include "LockFreeList.hpp"

void test_list_thread_task(
    parprog::LockFreeList<int> *list, int threadno, size_t thread_count) {
  auto it = list->InitializeThread();
  const size_t kElementsToInsert = 1024;
  std::vector<int> to_insert;
  std::random_device rd;
  std::mt19937 g(rd());
  for (size_t i = 0; i < kElementsToInsert; ++i) {
    int num = g() * thread_count + threadno;
    to_insert.push_back(num);
    list->Insert(num, it);
  }

  std::shuffle(to_insert.begin(), to_insert.end(), g);

  for (auto i : to_insert) {
    assert(list->Contains(i, it));
  }

  std::shuffle(to_insert.begin(), to_insert.end(), g);

  for (auto i : to_insert) {
    assert(list->Remove(i, it));
  }

  std::shuffle(to_insert.begin(), to_insert.end(), g);

  for (auto i : to_insert) {
    assert(!list->Contains(i, it));
  }
}

void test_list_single_thread() {
  parprog::LockFreeList<int> lst;
  decltype(lst)::HazardControllerType ctrl;
  test_list_thread_task(&lst, 0, 1);
}

void test_list() {
  const size_t kNumThreads = 8;
  parprog::LockFreeList<int> lst;
  std::vector<std::thread> runner_threads;
  for (size_t i = 0; i < kNumThreads; ++i) {
    runner_threads.emplace_back(test_list_thread_task, &lst, (int)i,
                                kNumThreads);
  }

  for (auto &i : runner_threads) {
    i.join();
  }
}

int main() {
  test_list_single_thread();
  test_list();
}
