#include <vector>
#include <cassert>
#include <cstdlib>
#include <map>


class HandleAllocator {
  public:
    using RefCounts = std::vector<int>;

    class Handle {
      public:
        Handle(HandleAllocator &table, size_t index)
        : _table(table), _index(index)
        {
          acquire();
        }

        ~Handle() { release(); }

        Handle(const Handle &arg)
        : _table(arg._table), _index(arg._index)
        {
          acquire();
        }

        Handle& operator=(const Handle &arg)
        {
          if (&arg != this) {
            release();
            _table = arg._table;
            _index = arg._index;
            acquire();
          }

          return *this;
        }

        friend bool operator==(const Handle &a, const Handle &b)
        {
          return a._index == b._index;
        }

        friend bool operator!=(const Handle &a, const Handle &b)
        {
          return !operator==(a, b);
        }

        friend bool operator<(const Handle &a, const Handle &b)
        {
          return a._index < b._index;
        }

        size_t index() const { return _index; };

      private:
        HandleAllocator &_table;
        size_t _index;

        void acquire() { ++_table._ref_counts[_index]; }

        void release()
        {
          if (--_table._ref_counts[_index] == 0) {
            _table._free_indices.push_back(_index);
          }
        }
    };

    Handle allocate()
    {
      if (!_free_indices.empty()) {
        size_t index = _free_indices.back();
        _free_indices.pop_back();
        return Handle(*this, index);
      }
      
      size_t n = _ref_counts.size();
      _ref_counts.push_back(0);
      return Handle(*this, n);
    }

    const RefCounts &refCounts() const { return _ref_counts; }

  private:
    std::vector<int> _ref_counts;
    std::vector<size_t> _free_indices;  // Free list to track unused indices
};


static void testBasics()
{
  HandleAllocator allocator;
  using Handle = HandleAllocator::Handle;

  {
    const Handle h1 = allocator.allocate();

    {
      const Handle h1a = h1;
      assert(h1a == h1);
      assert(allocator.refCounts()[h1.index()] == 2);
    }

    assert(allocator.refCounts()[h1.index()] == 1);
    const Handle h2 = allocator.allocate();
    assert(h2 != h1);
  }

  assert(allocator.refCounts().size() == 2);
  assert(allocator.refCounts()[0] == 0);
  assert(allocator.refCounts()[1] == 0);

  {
    const Handle h1 = allocator.allocate();
    assert(h1.index() == 0);
    const Handle h2 = allocator.allocate();
    assert(h2.index() == 1);
  }
}


static void testMap()
{
  HandleAllocator allocator;
  using Handle = HandleAllocator::Handle;
  std::map<Handle, int> m;
  const Handle h1 = allocator.allocate();
  m[h1] = 5;
  const Handle h2 = allocator.allocate();
  m[h2] = 7;
  assert(m[h1] == 5);
  assert(m[h2] == 7);
}


int main()
{
  testBasics();
  testMap();
}
