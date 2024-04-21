#ifndef SCENE_GRAPH_H_
#define SCENE_GRAPH_H_

#include <vector>
#include <algorithm>

namespace zorya
{
	struct Generic_Entity_Handle;

	template <typename T>
	struct Node
	{
		Node(const T& nodeValue) : value(nodeValue) {}

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

		void remove_node_recursive(std::vector<Node<T>*>& children, std::vector<Generic_Entity_Handle>& generic_handles_to_remove)
		{
			for (Node<T>* node : children)
			{

				Generic_Entity_Handle hnd_generic_entity;
				hnd_generic_entity.tag = node->value.tag;

				switch (hnd_generic_entity.tag)
				{
				case Entity_Type::LIGHT:
					hnd_generic_entity.hnd_light = node->value.hnd_light;
					break;
				case Entity_Type::MESH:
					hnd_generic_entity.hnd_submesh = node->value.hnd_submesh;
					break;
				default:
					break;
				}
				generic_handles_to_remove.push_back(hnd_generic_entity);

				remove_node_recursive(node->children, generic_handles_to_remove);
				delete node;
			}
			children.clear();
		}

		std::vector<Generic_Entity_Handle> remove_node(T& value_node_to_remove)
		{

			std::vector<Generic_Entity_Handle> generic_handles_to_remove;

			Node<T>* parent_node = nullptr;
			Node<T>* m_node_to_remove = find_node(root_node, value_node_to_remove, &parent_node);

			if (m_node_to_remove != nullptr)
			{

				Generic_Entity_Handle hnd_generic_entity;
				hnd_generic_entity.tag = m_node_to_remove->value.tag;

				switch (hnd_generic_entity.tag)
				{
				case Entity_Type::LIGHT:
					hnd_generic_entity.hnd_light = m_node_to_remove->value.hnd_light;
					generic_handles_to_remove.push_back(hnd_generic_entity);
					break;
				case Entity_Type::MESH:
					hnd_generic_entity.hnd_submesh = m_node_to_remove->value.hnd_submesh;
					generic_handles_to_remove.push_back(hnd_generic_entity);
					break;
				default:
					break;
				}

				remove_node_recursive(m_node_to_remove->children, generic_handles_to_remove);

				if (parent_node != nullptr)
				{
					for (auto& ref_node_to_remove : parent_node->children)
					{
						if (ref_node_to_remove->value == m_node_to_remove->value)
						{
							std::swap(ref_node_to_remove, parent_node->children.back());
							break;
						}
					}

					//Node<T>* tmpNode = nodeToRemove;
					//*(&nodeToRemove) = parentNode->children.at(parentNode->children.size() - 1);

					//std::swap(nodeToRemove, parentNode->children.at(parentNode->children.size() - 1));
					delete m_node_to_remove;
					parent_node->children.pop_back();
				}
			}

			return generic_handles_to_remove;
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

#endif // !SCENE_GRAPH_H


