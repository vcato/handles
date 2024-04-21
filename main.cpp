#include <vector>
#include <cassert>
#include <cstdlib>
#include <map>


class HandleAllocator {
  public:
    using RefCounts = std::vector<int>;
    using Index = size_t;

    class Handle {
      public:
        Handle(HandleAllocator &allocator)
        : _allocator(allocator), _index(allocator._acquireNewIndex())
        {
        }

        ~Handle() { _allocator._release(_index); }

        Handle(const Handle &arg)
        : _allocator(arg._allocator), _index(arg._index)
        {
          _allocator._acquire(_index);
        }

        Handle& operator=(const Handle &arg) = delete;

        friend bool operator==(const Handle &a, const Handle &b)
        {
          if (&a._allocator != &b._allocator) return false;
          return a._index == b._index;
        }

        friend bool operator!=(const Handle &a, const Handle &b)
        {
          return !operator==(a, b);
        }

        friend bool operator<(const Handle &a, const Handle &b)
        {
          assert(&a._allocator == &b._allocator);
          return a._index < b._index;
        }

        Index index() const { return _index; };

      private:
        HandleAllocator &_allocator;
        Index _index;
    };

    const RefCounts &refCounts() const { return _ref_counts; }

  private:
    std::vector<int> _ref_counts;
    std::vector<Index> _free_indices;  // Free list to track unused indices

    void _acquire(Index index) { ++_ref_counts[index]; }

    void _release(Index index)
    {
      if (--_ref_counts[index] == 0) {
        _free_indices.push_back(index);
      }
    }

    Index _acquireNewIndex()
    {
      HandleAllocator &allocator = *this;

      if (!allocator._free_indices.empty()) {
        Index index = allocator._free_indices.back();
        allocator._free_indices.pop_back();
        return index;
      }

      Index n = allocator._ref_counts.size();
      allocator._ref_counts.push_back(1);
      return n;
    }
};


static void testBasics()
{
  HandleAllocator allocator;
  using Handle = HandleAllocator::Handle;

  {
    const auto h1 = Handle(allocator);
    assert(allocator.refCounts()[h1.index()] == 1);

    {
      const Handle h1a = h1;
      assert(h1a == h1);
      assert(allocator.refCounts()[h1.index()] == 2);
    }

    assert(allocator.refCounts()[h1.index()] == 1);
    const auto h2 = Handle(allocator);
    assert(h2 != h1);
  }

  assert(allocator.refCounts().size() == 2);
  assert(allocator.refCounts()[0] == 0);
  assert(allocator.refCounts()[1] == 0);

  {
    const auto h1 = Handle(allocator);
    assert(h1.index() == 0);
    const auto h2 = Handle(allocator);
    assert(h2.index() == 1);
  }
}


static void testMap()
{
  HandleAllocator allocator;
  using Handle = HandleAllocator::Handle;
  std::map<Handle, int> m;
  auto h1 = Handle(allocator);
  m[h1] = 5;
  auto h2 = Handle(allocator);
  m[h2] = 7;
  assert(m[h1] == 5);
  assert(m[h2] == 7);
}


int main()
{
  testBasics();
  testMap();
}
