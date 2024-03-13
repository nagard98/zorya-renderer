#ifndef SCENE_GRAPH_H_
#define SCENE_GRAPH_H_

#include <vector>
#include <algorithm>

struct GenericEntityHandle;

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
		Node<T>* parentNode = nullptr;

		Node<T>* nodeAtWhichToAppend = findNode(rootNode, parent, &parentNode);
		Node<T>* newNode = new Node<T>(newValue);
		nodeAtWhichToAppend->children.push_back(newNode);
	}

	void removeNodeRecursive(std::vector<Node<T>*>& children, std::vector<GenericEntityHandle>& genericHandlesToRemove) {
		for (Node<T>* node : children) {
			GenericEntityHandle genericEntityHnd;
			genericEntityHnd.tag = node->value.tag;
			switch (genericEntityHnd.tag) {
			case EntityType::LIGHT:
				genericEntityHnd.lightHnd = node->value.lightHnd;
				break;
			case EntityType::MESH:
				genericEntityHnd.submeshHnd = node->value.submeshHnd;
				break;
			default:
				break;
			}
			genericHandlesToRemove.push_back(genericEntityHnd);

			removeNodeRecursive(node->children, genericHandlesToRemove);
			delete node;
		}
		children.clear();
	}

	std::vector<GenericEntityHandle> removeNode(T& nodeToRemoveValue) {
		std::vector<GenericEntityHandle> genericHandlesToRemove;
		
		Node<T>* parentNode = nullptr;
		Node<T>* nodeToRemove = findNode(rootNode, nodeToRemoveValue, &parentNode);
		
		if (nodeToRemove != nullptr) {
			
			GenericEntityHandle genericEntityHnd;
			genericEntityHnd.tag = nodeToRemove->value.tag;
			switch (genericEntityHnd.tag) {
			case EntityType::LIGHT:
				genericEntityHnd.lightHnd = nodeToRemove->value.lightHnd;
				genericHandlesToRemove.push_back(genericEntityHnd);
				break;
			case EntityType::MESH:
				genericEntityHnd.submeshHnd = nodeToRemove->value.submeshHnd;
				genericHandlesToRemove.push_back(genericEntityHnd);
				break;
			default:
				break;
			}
			
			removeNodeRecursive(nodeToRemove->children, genericHandlesToRemove);
			
			if (parentNode != nullptr) {
				for (auto& refNodeToRemove : parentNode->children) {
					if (refNodeToRemove->value == nodeToRemove->value) {
						std::swap(refNodeToRemove, parentNode->children.back());
						break;
					}				
				}

				//Node<T>* tmpNode = nodeToRemove;
				//*(&nodeToRemove) = parentNode->children.at(parentNode->children.size() - 1);

				//std::swap(nodeToRemove, parentNode->children.at(parentNode->children.size() - 1));
				delete nodeToRemove;
				parentNode->children.pop_back();
			}
		}

		return genericHandlesToRemove;
	}


	Node<T>* findNode(Node<T>* startNode, const T& nodeValue, Node<T>** nodeParent, Node<T>* currentParent = nullptr) const {
		if (startNode == nullptr) return nullptr;

		if (startNode->value == nodeValue) {
			*nodeParent = currentParent;
			return startNode;
		}else {
			for (auto node : startNode->children) {
				Node<T>* foundNode = findNode(node, nodeValue, nodeParent, startNode);

				if (foundNode != nullptr) {
					return foundNode;
				}
			}
		}

		return nullptr;
	}
};

#endif // !SCENE_GRAPH_H


