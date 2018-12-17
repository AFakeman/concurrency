#pragma once

#include <limits>
#include <memory>

#include "AtomicMarkedPointer.hpp"
#include "HazardPointerController.hpp"

namespace parprog {
template <typename T> struct ElementTraits {
  constexpr static T Min() { return std::numeric_limits<T>::min(); }
  constexpr static T Max() { return std::numeric_limits<T>::max(); }
};

template <class T, size_t thread_pointers = 16, size_t threads = 128> class LockFreeList {
public:
  struct Node {
    Node(Node *next, const T &element)
        : next_(next), element_(element){};
    AtomicMarkedPointer<Node*> next_;
    T element_;
  };

  using HazardControllerType =
      HazardPointerController<Node, thread_pointers, threads>;
  using HazardNodePointer = typename HazardControllerType::HazardPointer;
  using ThreadDataType = typename HazardControllerType::ThreadDataIterator;

  LockFreeList() : size_(0) {
    Node *last = new Node(nullptr, ElementTraits<T>::Max());
    first_ = new Node(last, ElementTraits<T>::Min());
  }

  /**
   * A function that should be called when a thread wants to access the list
   * for the first time. Call once per thread.
   * @return {ThreadDataType} Thread data object that should be passed to
   * this list's member functions. It is guranteed to be copy-constructable.
   */
  ThreadDataType InitializeThread() {
    return ctrl_.InitializeThread();
  }

  /**
   * Removes an element from the list if it exists.
   * @return {bool} Whether the element was deleted.
   */
  bool Remove(const T &element, const ThreadDataType &it) {
    Edge edge;
    HazardNodePointer second_next;
    do {
      edge = Locate(element, it);
      if (edge.second->element_ != element || edge.second->next_.Marked()) {
        return false;
      }
    } while (!edge.second->next_.TryMark(edge.second->next_.load()));
    size_--;
    return true;
  }

  size_t Size() const { return size_; }

  /**
   * Finds an element if it is there.
   * @return {HazardNodePointer} A hazard pointer to the node containing the
   * element, or an empty one if it could not be found.
   */
  HazardNodePointer Find(const T &element,
                            const ThreadDataType &it) {
    HazardNodePointer node = Locate(element, it).second;
    if (node->element_ == element && !node->next_.Marked()) {
      return {std::move(node)};
    } else {
      return {};
    }
  }

  /**
   * Checks if the element is contained in the list
   * @return {bool} true if it was found in the list.
   */
  bool Contains(const T &element,
                            const ThreadDataType &it) {
    return Find(element, it).get() != nullptr;
  }

  /**
   * Inserts the element in the list.
   * @return {HazardNodePointer} A hazard pointer to the node containing the
   * element.
   */
  HazardNodePointer Insert(const T &value, const ThreadDataType &it) {
    Edge edge;
    Node *to_insert = new Node(nullptr, value);
    do {
      edge = Locate(to_insert->element_, it);
      to_insert->next_.Store(edge.second.get());
      if ((edge.second->element_ == to_insert->element_) &&
          !edge.second->next_.Marked()) {
        return {std::move(edge.second)};
      }
    } while (!edge.first->next_.CompareAndSet(ptr(edge.second.get(), false),
                                              ptr(to_insert, false)));
    size_++;
    return {};
  }

private:
  using Edge = std::pair<HazardNodePointer, HazardNodePointer>;
  using ptr = typename AtomicMarkedPointer<Node*>::MarkedPointer;

  Edge Locate(const T &element, const ThreadDataType &it) {
    for (;;) {
      HazardNodePointer first;
      HazardNodePointer second(first_, it, nullptr);
      bool bad = false;
      do {
        first = std::move(second);
        second = HazardNodePointer(first->next_, it, &ctrl_);
        if (second->next_.Marked()) {
          if (!first->next_.CompareAndSet(ptr(second.get(), false),
                                          ptr(second->next_.Load(), false))) {
            bad = true;
            break;
          } else {
            second.Delete();
            second = HazardNodePointer(first->next_, it, &ctrl_);
          }
        }
      } while (second->element_ < element);

      if (!bad) {
        assert(first != second);
        assert(first);
        assert(second);
        return {std::move(first), std::move(second)};
      }
    }
  }

  Node *first_;
  std::atomic<size_t> size_;
  HazardControllerType ctrl_;
};
} // namespace parprog
