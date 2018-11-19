#pragma once

#include <iterator>

namespace transm {

template<class enum_type>
class enum_iterator {
private:
	enum_type value;
    using under = typename std::underlying_type<enum_type>::type;
public:
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using value_type = enum_type;
	using reference = enum_type;
	using pointer = enum_type*;
    using iterator_category = std::random_access_iterator_tag;

	enum_iterator() : value() {}
	enum_iterator(const enum_iterator& rhs) : value(rhs.value) {}
	explicit enum_iterator(enum_type value_) : value(value_) {}
	~enum_iterator() {}
	enum_iterator& operator=(const enum_iterator& rhs) { value = rhs.valud; return *this; }
	enum_iterator& operator++() { value = (enum_type)(under(value) + 1); return *this; }
	enum_iterator operator++(int){ enum_iterator r(*this); ++*this; return r; }
	enum_iterator& operator+=(size_type o) { value = (enum_type)(under(value) + o); return *this; }
	friend enum_iterator operator+(const enum_iterator& it, size_type o) { return enum_iterator((enum_type)(under(it) + o)); }
	friend enum_iterator operator+(size_type o, const enum_iterator& it) { return enum_iterator((enum_type)(under(it) + o)); }
	enum_iterator& operator--() { value = (enum_type)(under(value) - 1); return *this; }
	enum_iterator operator--(int) { enum_iterator r(*this); --*this; return r; }
	enum_iterator& operator-=(size_type o) { value = (enum_type)(under(value) + o); return *this; }
	friend enum_iterator operator-(const enum_iterator& it, size_type o) { return enum_iterator((enum_type)(under(it) - o)); }
	friend difference_type operator-(enum_iterator lhs, enum_iterator rhs) { return under(lhs.value) - under(rhs.value); }
	reference operator*() const { return value; }
	reference operator[](size_type o) const { return (enum_type)(under(value) + o); }
	const enum_type* operator->() const { return &value; }
	friend bool operator==(const enum_iterator& lhs, const enum_iterator& rhs) { return lhs.value == rhs.value; }
	friend bool operator!=(const enum_iterator& lhs, const enum_iterator& rhs) { return lhs.value != rhs.value; }
	friend bool operator<(const enum_iterator& lhs, const enum_iterator& rhs) { return lhs.value<rhs.value; }
	friend bool operator>(const enum_iterator& lhs, const enum_iterator& rhs) { return lhs.value>rhs.value; }
	friend bool operator<=(const enum_iterator& lhs, const enum_iterator& rhs) { return lhs.value <= rhs.value; }
	friend bool operator>=(const enum_iterator& lhs, const enum_iterator& rhs) { return lhs.value >= rhs.value; }
	friend void swap(const enum_iterator& lhs, const enum_iterator& rhs) { std::swap(lhs.value, rhs.value); }
	enum_iterator begin() { return enum_iterator<enum_type>(enum_type::First); }
	enum_iterator end() { return enum_iterator<enum_type>(enum_type::Last); }
};

} // namespace transm
