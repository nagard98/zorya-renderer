#ifndef SCENE_GRAPH_H_
#define SCENE_GRAPH_H_

#include <vector>
#include <algorithm>
#include <utility>



namespace zorya
{
	//struct Generic_Entity_Handle;

	template <typename T>
	struct Node
	{
		Node(const T& nodeValue) : value(nodeValue) {}
		~Node()
		{
			for (auto node : children){
				delete node; 
				node = nullptr;
			}
		}

		void swap(Node<T>& other)
		{
			std::swap(this->value, other.value);
			std::swap(this->children, other.children);
		}

		T value;
		std::vector<Node<T>*> children;
	};


	template <typename T>
	class Scene_Graph
	{
	public:
		Scene_Graph(const T& root_value) : root_node(new Node<T>(root_value)) {}

		Node<T>* root_node;
		
		void insertNode(const T& parent, const T& new_value)
		{
			Node<T>* parent_node = nullptr;

			Node<T>* node_at_which_to_append = find_node(root_node, parent, &parent_node);
			Node<T>* new_node = new Node<T>(new_value);
			node_at_which_to_append->children.push_back(new_node);
		}


		void remove_node(Node<T>* node_to_remove, Node<T>* parent_of_node)
		{
			Node<T>& node_in_vec = (*node_to_remove);
			Node<T>& last_node = *parent_of_node->children.at(parent_of_node->children.size() - 1);
			std::swap(node_in_vec, last_node);

			parent_of_node->children.pop_back();

			delete &last_node;
		}


		Node<T>* find_node(Node<T>* start_node, const T& node_value, Node<T>** node_parent, Node<T>* current_parent = nullptr) const
		{
			if (start_node == nullptr) return nullptr;

			if (start_node->value == node_value)
			{
				*node_parent = current_parent;
				return start_node;
			}
			else
			{
				for (auto node : start_node->children)
				{
					Node<T>* foundNode = find_node(node, node_value, node_parent, start_node);

					if (foundNode != nullptr)
					{
						return foundNode;
					}
				}
			}

			return nullptr;
		}
	};
}


namespace std
{
	template <typename T>
	void swap(zorya::Node<T>& lhs, zorya::Node<T>& rhs)
	{
		lhs.swap(rhs);
	}
}


#endif // !SCENE_GRAPH_H


