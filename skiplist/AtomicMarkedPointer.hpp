#pragma once

#include <atomic>
#include <cassert>

namespace parprog {
template <typename T> class AtomicMarkedPointer {
public:
  struct MarkedPointer {
    MarkedPointer(T *ptr, bool marked) : ptr_(ptr), marked_(marked) {}

    bool operator==(const MarkedPointer &that) const {
      return (ptr_ == that.ptr_) && (marked_ == that.marked_);
    }

    T *ptr_;
    bool marked_;
  };

  using PackedMarkedPointer = size_t;

  AtomicMarkedPointer(T *ptr = nullptr) : packed_ptr_{Pack({ptr, false})} {
    assert(Unpack(packed_ptr_.load()) == (MarkedPointer{ptr, false}));
  }

  MarkedPointer LoadMarked() const { return Unpack(packed_ptr_.load()); }

  T *Load() const {
    const auto marked_ptr = LoadMarked();
    return marked_ptr.ptr_;
  }

  void Store(MarkedPointer marked_ptr) { packed_ptr_.store(Pack(marked_ptr)); }

  T *load() const { return Load(); }

  void store(T *ptr) { Store(ptr); }

  void Store(T *ptr) { Store(MarkedPointer{ptr, false}); }

  bool TryMark(T *ptr) { return CompareAndSet({ptr, false}, {ptr, true}); }

  bool Marked() const {
    const auto curr_ptr = Load();
    return curr_ptr.marked_;
  }

  bool CompareAndSet(MarkedPointer expected, MarkedPointer desired) {
    auto expected_packed = Pack(expected);
    const auto desired_packed = Pack(desired);
    return packed_ptr_.compare_exchange_strong(expected_packed, desired_packed);
  }

private:
  static PackedMarkedPointer Pack(MarkedPointer marked_ptr) {
    return reinterpret_cast<size_t>(marked_ptr.ptr_) ^
           (marked_ptr.marked_ ? 1 : 0);
  }

  static MarkedPointer Unpack(PackedMarkedPointer packed_marked_ptr) {
    const auto marked = static_cast<bool>(packed_marked_ptr & 1);
    const auto ptr = reinterpret_cast<T *>(packed_marked_ptr ^ marked);
    return {ptr, marked};
  }

  std::atomic<PackedMarkedPointer> packed_ptr_;
};
} // namespace parprog
