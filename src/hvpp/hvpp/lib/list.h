#pragma once
#include <iterator>
#include <utility>   // std::move

#pragma warning(push)
#pragma warning(disable: 4458)

//
// Basic implementation.
//

struct list_item
{
  void push_front(list_item* item) noexcept
  {
    item->head = this->head;
    item->tail = this;

    this->head->tail = item;
    this->head = item;
  }

  void push_back(list_item* item) noexcept
  {
    item->head = this;
    item->tail = this->tail;

    this->tail->head = item;
    this->tail = item;
  }

  list_item* pop_front() noexcept
  {
    list_item* item = this->head;

    this->head = item->head;
    item->head->tail = this;

    return item;
  }

  list_item* pop_back() noexcept
  {
    list_item* item = this->tail;

    this->tail = item->tail;
    item->tail->head = this;

    return item;
  }

  bool remove() noexcept
  {
    auto head = this->head;
    auto tail = this->tail;

    tail->head = head;
    head->tail = tail;

    return head == tail;
  }

  list_item* head;
  list_item* tail;
};

struct list_head : list_item
{
  list_head()
  {
    clear();
  }

  int size() const noexcept
  {
    int result = 0;

    list_item* item = head;
    while (item->head != head)
    {
      ++result;

      item = item->head;
    }

    return result;
  }

  void clear()
  {
    this->head = this;
    this->tail = this;
  }

  bool empty() const noexcept
  {
    return head == this;
  }
};

//
// Templated implementation.
//

template <
  typename T
>
struct list_item_wrapper : list_item
{
  list_item_wrapper() noexcept = default;
  list_item_wrapper(const T& value) noexcept : value(value) { }
  list_item_wrapper(T&& value) noexcept : value(std::move(value)) { }

  T value;
};

template <
  typename T
>
struct list : list_head
{
  struct iterator
  {
    using value_type        = T;
    using difference_type   = std::ptrdiff_t;
    using pointer           = T*;
    using reference         = T&;
    using iterator_category = std::bidirectional_iterator_tag;

    iterator& operator++()                 noexcept {                   item = (list_item_wrapper<T>*)item->head; return *this; }
    iterator  operator++(int)              noexcept { auto tmp = *this; item = (list_item_wrapper<T>*)item->head; return tmp;   }
    iterator& operator--()                 noexcept {                   item = (list_item_wrapper<T>*)item->tail; return *this; }
    iterator  operator--(int)              noexcept { auto tmp = *this; item = (list_item_wrapper<T>*)item->tail; return tmp;   }

    bool operator==(const iterator& other) noexcept { return item == other.item; }
    bool operator!=(const iterator& other) noexcept { return item != other.item; }

    reference operator*()                       noexcept { return  item->value; }
    pointer   operator->()                      noexcept { return &item->value; }

    list_item_wrapper<T>* item;
  };

  using item_type         = list_item_wrapper<T>;

  list() = default;
  ~list() { clear(); }

  iterator begin() noexcept { return iterator{ (item_type*)head }; }
  iterator end()  noexcept  { return iterator{ (item_type*)this }; }

  void clear()
  {
    while (!empty())
    {
      pop_back();
    }
  }

  void push_front(const T& value) noexcept
  {
    item_type* item = new item_type(value);
    list_head::push_front(item);
  }

  void push_front(T&& value) noexcept
  {
    item_type* item = new item_type(std::move(value));
    list_head::push_front(item);
  }

  void push_back(const T& value) noexcept
  {
    item_type* item = new item_type(value);
    list_head::push_back(item);
  }

  void push_back(T&& value) noexcept
  {
    item_type* item = new item_type(std::move(value));
    list_head::push_back(item);
  }

  void pop_front() noexcept
  {
    auto item = list_head::pop_front();
    delete item;
  }

  void pop_back() noexcept
  {
    auto item = list_head::pop_back();
    delete item;
  }

  iterator erase(iterator it)
  {
    it.item->remove();
    delete (it++).item;

    return it;
  }

  int remove(const T& value)
  {
    return remove_if([&value](const T& other) { return value == other; });
  }

  template <class UnaryPredicate>
  int remove_if(UnaryPredicate p)
  {
    int result = 0;

    auto it = begin();
    while (it != end())
    {
      if (p(*it))
      {
        it = erase(it);
        ++result;
      }
      else
      {
        ++it;
      }
    }

    return result;
  }
};

#pragma warning(pop)
