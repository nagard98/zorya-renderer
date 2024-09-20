#ifndef TREE_H_
#define TREE_H_

#include <vector>
#include <utility>

template <typename T>
struct Node2
{
	Node2() {}
	//Node2(T&& nodeValue) : value(std::forward<T&&>(nodeValue)), m_parent(nullptr) {}
	Node2(T&& nodeValue, Node2<T>* parent) : value(std::forward<T&&>(nodeValue)), m_parent(parent) {}
	Node2(const T& nodeValue) : value(nodeValue) {}

	T value;
	Node2<T>* m_parent;
	std::vector<Node2<T>> children;
};

#endif // !TREE_H_
