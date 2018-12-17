#pragma once

#include <atomic>

namespace parprog {
/**
 * A very simple lock-free list. Doesn't have ABA because elements can't be
 * deleted without destroying the list. Implementation is the same as a
 * typical lock-free stack, but without a pop operation.
 */
template <class T> class NoPopLinkedList {
public:
  struct ListNode {
    ListNode *next;
    T value;
  };

  NoPopLinkedList() = default;

  // Destructor should be called when it is guranteed that nobody is going
  // to access its nodes.
  ~NoPopLinkedList() {
    while (head_ != nullptr) {
      ListNode *to_delete = head_;
      head_ = head_.load()->next;
      delete to_delete;
    }
  }

  /**
   * The iterator is not thread-safe, and is meant to use within a single
   * thread.
   */
  class Iterator {
  public:
    using value_type = T;
    using reference = T &;
    using pointer = T *;

    Iterator() = default;
    Iterator(const Iterator &) = default;

    bool operator==(const Iterator &rhs) { return cursor_ == rhs.cursor_; }

    bool operator!=(const Iterator &rhs) { return cursor_ != rhs.cursor_; }

    Iterator &operator++() {
      cursor_ = cursor_->next;
      return *this;
    }

    Iterator operator++(int) {
      Iterator to_return(cursor_);
      (*this)++;
      return to_return;
    }

    pointer operator->() { return &(cursor_->value); }

    reference operator*() { return cursor_->value; }

  private:
    Iterator(ListNode *cursor) : cursor_(cursor) {}
    friend class NoPopLinkedList;
    ListNode *cursor_{nullptr};
  };

  Iterator Insert(const T &value) {
    ListNode *new_node = new ListNode{head_.load(), value};
    while (!head_.compare_exchange_strong(new_node->next, new_node))
      ;
    size_++;
    return {new_node};
  }

  size_t size() const { return size_; }

  using iterator = Iterator;
  iterator begin() { return {head_}; }
  iterator end() { return {nullptr}; }

private:
  std::atomic<ListNode *> head_{nullptr};
  std::atomic<size_t> size_{0};
};
} // namespace parprog
