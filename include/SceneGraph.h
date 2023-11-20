#ifndef SCENE_GRAPH_H_
#define SCENE_GRAPH_H_

#include <vector>

template <typename T>
struct Node {
	Node(const T& nodeValue) : value(nodeValue) {}

	T value;
	std::vector<Node<T>*> children;
};

template <typename T>
class SceneGraph {
public:
	SceneGraph(const T& rootValue) : rootNode(new Node<T>(rootValue)) {}

	Node<T>* rootNode;

	void insertNode(const T& parent, const T& newValue) {
		Node<T>* parentNode = findNode(rootNode, parent);
		Node<T>* newNode = new Node<T>(newValue);
		parentNode->children.push_back(newNode);
	}

	Node<T>* findNode(Node<T>* startNode, const T& nodeValue) const {
		if (startNode == nullptr) return nullptr;

		if (startNode->value == nodeValue) return startNode;
		else {
			for (int i = 0; i < startNode->children.size(); i++) {
				Node<T>* nodeFound = findNode(startNode->children.at(i), nodeValue);
				if (nodeFound != nullptr) return nodeFound;
			}
		}

		return nullptr;
	}
};

#endif // !SCENE_GRAPH_H


