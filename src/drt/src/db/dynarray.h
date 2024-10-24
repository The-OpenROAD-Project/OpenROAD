#pragma once
#include <vector>

template <typename T>
class dynarray
{
 public:
  dynarray() = default;
  dynarray(std::vector<T>&& data) : data(std::move(data)) {}
  dynarray(size_t size) : data(size) {}
  T& operator[](size_t idx) { return data[idx]; }
  const T& operator[](size_t idx) const { return data[idx]; }
  auto begin() { return data.begin(); }
  auto begin() const { return data.begin(); }
  auto end() { return data.end(); }
  auto end() const { return data.end(); }
  size_t size() const { return data.size(); }
  void clear() { data.clear(); }
  T& front() { return data.front(); }
  const T& front() const { return data.front(); }
  T& back() { return data.back(); }
  const T& back() const { return data.back(); }

 private:
  std::vector<T> data;
};
