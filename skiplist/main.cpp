#include <cassert>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

#include "LockFreeList.hpp"

void test_list_thread_task(
    typename parprog::LockFreeList<int>::HazardControllerType *ctrl,
    parprog::LockFreeList<int> *list, int threadno, size_t thread_count) {
  auto it = ctrl->InitializeThread();
  const size_t kElementsToInsert = 1024;
  std::vector<int> to_insert;
  std::random_device rd;
  std::mt19937 g(rd());
  for (size_t i = 0; i < kElementsToInsert; ++i) {
    int num = g() * thread_count + threadno;
    to_insert.push_back(num);
    list->Insert(num, it, *ctrl);
  }

  std::shuffle(to_insert.begin(), to_insert.end(), g);

  for (auto i : to_insert) {
    assert(list->Contains(i, it, *ctrl));
  }

  std::shuffle(to_insert.begin(), to_insert.end(), g);

  for (auto i : to_insert) {
    assert(list->Remove(i, it, *ctrl));
  }

  std::shuffle(to_insert.begin(), to_insert.end(), g);

  for (auto i : to_insert) {
    assert(!list->Contains(i, it, *ctrl));
  }
}

void test_list_single_thread() {
  parprog::LockFreeList<int> lst;
  decltype(lst)::HazardControllerType ctrl;
  test_list_thread_task(&ctrl, &lst, 0, 1);
}

void test_list() {
  const size_t kNumThreads = 8;
  parprog::LockFreeList<int> lst;
  decltype(lst)::HazardControllerType ctrl;
  std::vector<std::thread> runner_threads;
  for (size_t i = 0; i < kNumThreads; ++i) {
    runner_threads.emplace_back(test_list_thread_task, &ctrl, &lst, (int)i,
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
