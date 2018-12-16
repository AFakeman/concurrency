#include <algorithm>
#include <atomic>
#include <unordered_set>
#include <thread>
#include <vector>

#include "NoPopLinkedList.hpp"

namespace parprog {
struct ThreadData {
  static const size_t kPointersPerThread = 16;
  static const size_t kDeleteListCapacity = 128;
  std::thread::id thread_id{std::this_thread::get_id()};
  std::atomic<void*> hazard_pointers[kPointersPerThread];
  std::unordered_set<void*> delete_list;
};

template <class T>
class HazardPointerController {
public:
  HazardPointerController() = delete;

  using ThreadDataIterator = NoPopLinkedList<ThreadData>::Iterator;

  /**
   * Every thread that wants to use hazard pointers needs to call this function.
   * Returns the list iterator to the thread data of the caller thread.
   */
  ThreadDataIterator InitializeThread() {
    return thread_list_.Insert(ThreadData());
  }

  /**
   * A function to get a pointer from an atomic variable and mark it as
   * a hazard pointer. As long as the pointer is marked as a hazard,
   * it will not be deleted. After using the pointer,
   * RemoveHazardLabel or DeleteHazardPointer must be called.
   */
  T* GetHazardPointer(const std::atomic<T*> &var, ThreadDataIterator &it) {
    auto iter = std::find(std::begin(it->hazard_pointers),
                          std::end(it->hazard_pointers), nullptr);
    if (iter == std::end(it->hazard_pointers)) {
      throw std::runtime_error("Hit hazard point limit");
    };

    T *ptr;
    do {
      ptr = var.load();
      *iter = ptr;
    } while (ptr != var.load());
    return ptr;
  }

  /**
   * Unhazards the pointer, signifying that it is no longer used by this thread.
   */
  void RemoveHazardLabel(T* ptr, ThreadDataIterator &it) {
    auto iter = std::find(std::begin(it->hazard_pointers),
                          std::end(it->hazard_pointers), ptr);
    if (iter == std::end(it->hazard_pointers)) {
      throw std::runtime_error("No such pointer");
    };

    *iter = nullptr;
  }

  /**
   * Marks the hazard pointer for deletion. This should be called after
   * replacing the atomic variable value.
   */
  void DeleteHazardPointer(T* ptr, ThreadDataIterator &it) {
    RemoveHazardLabel(ptr, it);
    it->delete_list.insert(ptr);
    if (it->delete_list.size() == ThreadData::kDeleteListCapacity) {
      Cleanup(it);
    }
  }

  /**
   * Deletes the remaining pointers marked for deletion.
   * No threads are allowed to access the object at this point.
   */
  ~HazardPointerController() {
    std::unordered_set<T*> to_delete;
    for (auto &i : thread_list_) {
      for (void *ptr : i.delete_list) {
        to_delete.insert(static_cast<T*>(ptr));
      }
    }
    for (T* ptr : to_delete) {
      delete ptr;
    }
  }
private:
  /**
   * Deletes pointers marked for deletion by this thread.
   */
  void Cleanup(ThreadDataIterator &it) {
    std::unordered_set<void*> global_hazards;
    for (auto &i : thread_list_) {
      for (void *ptr : i.hazard_pointers) {
        global_hazards.insert(ptr);
      }
    }

    for (auto delete_iter = it->delete_list.begin();
         delete_iter != it->delete_list.end();) {
      void* to_delete = *delete_iter;
      if (global_hazards.find(to_delete) == global_hazards.end()) {
        delete static_cast<T*>(to_delete);
        delete_iter = it->delete_list.erase(delete_iter);
      } else {
        delete_iter++;
      }
    }
  }

  NoPopLinkedList<ThreadData> thread_list_;
};
} // namespace parprog
