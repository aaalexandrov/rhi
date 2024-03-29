#pragma once

namespace utl {

enum class Enum {
	Stop,
	Continue,
	SkipChildren,
};

template<typename T>
struct specialization_of {
	using base_type = T;
};

template<template<typename ...> class X, typename ...Args>
struct specialization_of<X<Args...>> {
	template<typename... Ps> using base_type = X<Ps...>;
};

template <typename T>
struct fixed_array_traits {
	static constexpr bool is_array = false;
	static constexpr size_t size = 0;
	using value_type = void;
};

template <typename T, size_t N>
struct fixed_array_traits<T[N]> {
	static constexpr bool is_array = true;
	static constexpr size_t size = N;
	using value_type = T;
};

template <typename T, size_t N>
struct fixed_array_traits<std::array<T, N>> {
	static constexpr bool is_array = true;
	static constexpr size_t size = N;
	using value_type = T;
};

template <typename T>
size_t ArraySize(T const &) { return fixed_array_traits<T>::size; }

template <typename T, typename Cmp = std::less<void>>
bool AddSorted(std::vector<T> &vec, T val, Cmp cmp = Cmp())
{
	auto it = std::lower_bound(vec.begin(), vec.end(), val, cmp);
	if (it != vec.end() && !(val < *it))
		return false;
	vec.insert(it, val);
	return true;
}

template <typename T, typename K, typename Cmp = std::less<void>>
T *FindSorted(std::vector<T> const &vec, K key, Cmp cmp = Cmp())
{
	auto it = std::lower_bound(vec.begin(), vec.end(), key);
	if (it == vec.end() || cmp(key, *it))
		return nullptr;
	return &*it;
}

template <typename T, typename K, typename Cmp = std::less<void>>
bool EraseSorted(std::vector<T> &vec, K key, Cmp cmp = Cmp())
{
	auto it = std::lower_bound(vec.begin(), vec.end(), key, cmp);
	if (it == vec.end() || cmp(key, *it))
		return false;
	vec.erase(it);
	return true;
}

template <typename Map, typename K, typename Fn>
auto &GetFromMap(Map &map, K const &k, Fn fn) {
	auto it = map.find(k);
	if (it != map.end())
		return it->second;
	return map.insert({ k, fn() }).first->second;
}

template <typename Vec>
auto &GetFromVec(Vec &vec, size_t ind) {
	if (ind >= vec.size())
		vec.resize(ind + 1);
	return vec[ind];
}

template <typename Vector>
void RemoveElement(Vector &v, int i)
{
	std::swap(v[i], v.back());
	v.resize(v.size() - 1);
}

template <typename Type>
size_t GetHash(Type const &val, size_t prevHash = 0)
{
	return 31 * prevHash + std::hash<Type>()(val);
}

} // utl

namespace std {

template <typename ElemType, size_t Size>
struct hash<array<ElemType, Size>> {
	auto operator() (array<ElemType, Size> const &key) const
	{
		std::hash<ElemType> hasher;
		size_t result = 0;
		for (size_t i = 0; i < Size; ++i) {
			result = result * 31 + hasher(key[i]);
		}
		return result;
	}
};

template <typename ElemType>
struct hash<vector<ElemType>> {
	auto operator() (vector<ElemType> const &key) const
	{
		std::hash<ElemType> hasher;
		size_t result = 0;
		for (size_t i = 0; i < key.size(); ++i) {
			result = result * 31 + hasher(key[i]);
		}
		return result;
	}
};

template <typename A, typename B>
struct hash<pair<A, B>> {
	size_t operator()(pair<A, B> const &p) const {
		size_t h = hash<A>()(p.first);
		h = 31 * h + hash<B>()(p.second);
		return h;
	}
};

} // std