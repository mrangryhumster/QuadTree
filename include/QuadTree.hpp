#pragma once

#include <cassert>
#include <memory>
#include <cmath>
#include <iterator>
#include <deque>

// Primary template
template<class _T_ValueType,
	class _T_PositionType = int32_t,
	class _T_Allocator = std::allocator<_T_ValueType>,
	typename _T_Enable = void
> class QuadTree;


template<class _T_ValueType, class _T_PositionType, class _T_Allocator>
class QuadTree <_T_ValueType, _T_PositionType, _T_Allocator, typename std::enable_if<std::is_integral<_T_PositionType>::value>::type> {
	//Private default constructor for static_assert's
	QuadTree() {

		static_assert(!std::is_const<_T_ValueType>::value, "_TValueType must not be const-qualified");
		static_assert(!std::is_reference<_T_ValueType>::value, "_TValueType must not be a reference");

		static_assert(!std::is_const<_T_PositionType>::value, "_TPositionType must not be const-qualified");
		static_assert(!std::is_reference<_T_PositionType>::value, "_TPositionType must not be a reference");

		static_assert(
			std::is_copy_assignable<_T_ValueType>::value ||
			std::is_move_assignable<_T_ValueType>::value ||
			std::is_copy_constructible<_T_ValueType>::value ||
			std::is_move_constructible<_T_ValueType>::value,
			"_TValueType : "
			"not copy_constructible, "
			"not move_constructible, "
			"not copy_assignable, "
			"not move_assignable... "
			"And how do you think it should work ?");

	}

private:
	struct _TreeNode;
	struct _LinkNode;
	struct _ItemNode;

	typedef char _Td_ChildIdType;

	typedef typename std::allocator_traits<_T_Allocator>::template rebind_alloc<
		_LinkNode>
		_Td_LinkAllocType;
	typedef typename std::allocator_traits<_T_Allocator>::template rebind_alloc<
		_ItemNode>
		_Td_ItemAllocType;
	typedef typename std::allocator_traits<_T_Allocator>::template rebind_alloc<
		_Td_ChildIdType>
		_Td_ChildIdTAllocType;
		
	using _U_LinkNodeAllocTraits = std::allocator_traits<_Td_LinkAllocType>;
	using _U_ItemNodeAllocTraits = std::allocator_traits<_Td_ItemAllocType>;

public:
	struct QuadTreeIterator;
	using iterator = QuadTreeIterator;

public:

	QuadTree(_T_PositionType min_x, _T_PositionType min_y, _T_PositionType max_x, _T_PositionType max_y) :
		m_RegionMinX((std::min)(min_x, max_x)),
		m_RegionMinY((std::min)(min_y, max_y)),
		m_RegionMaxX((std::max)(min_x, max_x)),
		m_RegionMaxY((std::max)(min_y, max_y)),
		m_RootNode(nullptr)
	{
		//Sanity checks
		assert(((m_RegionMaxX - m_RegionMinX) > 0) && "Region width is zero.");
		assert(((m_RegionMaxY - m_RegionMinY) > 0) && "Region height is zero.");

	}
	~QuadTree() {
		//Cleanup all memory
		ImplClear();
	}
	inline void insert(_T_PositionType x, _T_PositionType y, const _T_ValueType& data) {
		_ItemNode* node = static_cast<_ItemNode*>(ImplInsert(x, y, data));
	}

	inline void insert(_T_PositionType x, _T_PositionType y, _T_ValueType&& data) {
		_ItemNode* node = static_cast<_ItemNode*>(ImplInsert(x, y, std::move(data)));
	}

	inline const _T_ValueType* find(_T_PositionType x, _T_PositionType y) const {
		_ItemNode* node = static_cast<_ItemNode*>(ImplFind(x, y));
		return (node != nullptr) ? &node->Data : nullptr;
	}

	inline _T_ValueType* find(_T_PositionType x, _T_PositionType y) {
		_ItemNode* node = static_cast<_ItemNode*>(ImplFind(x, y));
		return (node != nullptr) ? &node->Data : nullptr;
	}

	inline void erase(_T_PositionType x, _T_PositionType y) {
		ImplErase(x, y);
	}

	inline void clear() {
		ImplClear();
	}

	inline void is_empty() {
		return (m_RootNode == nullptr);
	}

	inline iterator begin() {
		return iterator(m_RootNode);
	}

	inline iterator end() {
		return iterator(nullptr);
	}

private:

	_T_PositionType m_RegionMinX;
	_T_PositionType m_RegionMinY;
	_T_PositionType m_RegionMaxX;
	_T_PositionType m_RegionMaxY;

	_LinkNode* m_RootNode;

	_Td_LinkAllocType m_LinkAllocator;
	_Td_ItemAllocType m_ItemAllocator;



private:

	template<typename _T_ImplDataType>
	_TreeNode* ImplInsert(const _T_PositionType x, const _T_PositionType y, _T_ImplDataType&& data) {
		//Check if we in valid range
		if (!IsInRange(x, y)) return nullptr;

		//Create root node if not present
		if (m_RootNode == nullptr)
			m_RootNode = new _LinkNode(nullptr);

		//Root node is always _LinkNode
		_LinkNode* link_node = (m_RootNode);

		//Create initial coordinates and bbox size of current quad
		_T_PositionType quad_x = m_RegionMinX;
		_T_PositionType quad_y = m_RegionMinY;
		_T_PositionType quad_w = m_RegionMaxX - m_RegionMinX;
		_T_PositionType quad_h = m_RegionMaxY - m_RegionMinY;

		while (link_node != nullptr) {
			//Check if it is a smllest quad
			if (quad_w <= 2 && quad_h <= 2) {
				//Get child_index from x and y
				const size_t child_index = ImplNextItemQuad(x, y, quad_x, quad_y);
				//Check if _ItemNode present
				if (link_node->Childs[child_index] == nullptr) {
					//Create _ItemNode
					_ItemNode* new_item_node =
						_U_ItemNodeAllocTraits::allocate(m_ItemAllocator, 1);
					//Construct _ItemNode
					_U_ItemNodeAllocTraits::construct(
						m_ItemAllocator,
						new_item_node,
						link_node, std::forward<_T_ImplDataType>(data)
					);
					//Assign new node to parent link as child
					link_node->Childs[child_index] = new_item_node;
				}
				else {
					ImplUpdateItem(link_node, child_index, std::forward<_T_ImplDataType>(data));
				}
				return link_node->Childs[child_index];
			}
			else {
				//Get child_index from x and y
				const size_t child_index = ImplNextLinkQuad(x, y, quad_x, quad_y, quad_w, quad_h);
				//Check if Link present
				if (link_node->Childs[child_index] == nullptr) {
					//Create _LinkNode
					_LinkNode* new_link_node =
						_U_LinkNodeAllocTraits::allocate(m_LinkAllocator, 1);
					//Construct _LinkNode
					_U_LinkNodeAllocTraits::construct(
						m_LinkAllocator,
						new_link_node,
						link_node
					);
					link_node->Childs[child_index] = new_link_node;
				}
				link_node = static_cast<_LinkNode*>(link_node->Childs[child_index]);
			}
		}
		return nullptr;
	}

	_TreeNode* ImplFind(const _T_PositionType x, const _T_PositionType y) {
		//Check if we in valid range
		if (!IsInRange(x, y)) return nullptr;

		//No root node, return instantly
		if (m_RootNode == nullptr)
			return nullptr;

		//Root node is always _LinkNode
		_LinkNode* link_node = (m_RootNode);

		//Create initial coordinates and bbox size of current quad
		_T_PositionType quad_x = m_RegionMinX;
		_T_PositionType quad_y = m_RegionMinY;
		_T_PositionType quad_w = m_RegionMaxX - m_RegionMinX;
		_T_PositionType quad_h = m_RegionMaxY - m_RegionMinY;

		while (link_node != nullptr) {
			//Check if it is a smllest quad
			if (quad_w <= 2 && quad_h <= 2) {
				//Get child_index from x and y
				const size_t child_index = ImplNextItemQuad(x, y, quad_x, quad_y);
				return link_node->Childs[child_index];
			}
			else {
				//Get child_index from x and y
				const size_t child_index = ImplNextLinkQuad(x, y, quad_x, quad_y, quad_w, quad_h);
				link_node = static_cast<_LinkNode*>(link_node->Childs[child_index]);
			}
		}
		return nullptr;
	}

	void ImplErase(const _T_PositionType x, const _T_PositionType y) {
		//Check if we in valid range
		if (!IsInRange(x, y)) return;

		//No root node, return instantly
		if (m_RootNode == nullptr)
			return;

		//Root node is always _LinkNode
		_LinkNode* link_node = (m_RootNode);

		//Create initial coordinates and bbox size of current quad
		_T_PositionType quad_x = m_RegionMinX;
		_T_PositionType quad_y = m_RegionMinY;
		_T_PositionType quad_w = m_RegionMaxX - m_RegionMinX;
		_T_PositionType quad_h = m_RegionMaxY - m_RegionMinY;

		//We store here all children indexes along the way to the node
		_TreeNode* node_to_erase = nullptr;
		std::deque<_Td_ChildIdType, _Td_ChildIdTAllocType> path_to_node;

		while (link_node != nullptr) {
			//Check if it is a smallest quad
			if (quad_w <= 2 && quad_h <= 2) {
				//Get child_index from x and y
				const size_t child_index = ImplNextItemQuad(x, y, quad_x, quad_y);
				node_to_erase = link_node->Childs[child_index];
				path_to_node.push_back((_Td_ChildIdType)child_index);
				break; //Exit the cycle
			}
			else {
				//Get child_index from x and y
				const size_t child_index = ImplNextLinkQuad(x, y, quad_x, quad_y, quad_w, quad_h);
				link_node = static_cast<_LinkNode*>(link_node->Childs[child_index]);
				path_to_node.push_back((_Td_ChildIdType)child_index);
			}

			if (path_to_node.size() >= 16) {
				return;
			}
		}

		//if node_to_erase is not the null - delete it!
		if (node_to_erase != nullptr) {

			//NOTE: We reuse link_node variable here
			link_node = static_cast<_LinkNode*>(node_to_erase->Parent);

			//Deallocate memory for item
			_U_ItemNodeAllocTraits::deallocate(
				m_ItemAllocator,
				static_cast<_ItemNode*>(node_to_erase),
				1
			);

			while (link_node != nullptr) {

				//Pop index from stack
				const size_t child_index =
					path_to_node.back();
				path_to_node.pop_back();

				//Set child to nullptr
				link_node->Childs[child_index] = nullptr;

				//Count nullptrs in Childs array
				for (size_t i = 0; i < (size_t)4; i++) {
					if (link_node->Childs[i] != nullptr) {
						//WE FOUND SOMETHING
						//ABORT THE MISSION!
						return;
					}
				}
				//If we are here - we didnt find valid ptr in childs
				//we delete whole link node

				//Store ptr to parent separetly
				_LinkNode* parent = static_cast<_LinkNode*>(link_node->Parent);

				//Deallocate memory for link
				_U_LinkNodeAllocTraits::deallocate(
					m_LinkAllocator,
					static_cast<_LinkNode*>(link_node),
					1
				);
				//We deleting root node
				if (parent == nullptr) {
					m_RootNode = nullptr;
				}

				link_node = parent;
			}
		}
	}

	inline size_t ImplNextLinkQuad(
		const _T_PositionType& x,
		const _T_PositionType& y,
		_T_PositionType& quad_x,
		_T_PositionType& quad_y,
		_T_PositionType& quad_w,
		_T_PositionType& quad_h) const {
		size_t child_index = 0;

		const _T_PositionType center_x =
			quad_x + (quad_w / 2) + (quad_w % 2);
		if (x >= center_x) {
			quad_w = (quad_w / 2);
			quad_x = center_x;
			child_index += 1;
		}
		else {
			quad_w = center_x - quad_x;
		}

		const _T_PositionType center_y =
			quad_y + (quad_h / 2) + (quad_h % 2);
		if (y >= center_y) {
			quad_h = (quad_h / 2);
			quad_y = center_y;
			child_index += 2;
		}
		else {
			quad_h = center_y - quad_y;
		}
		return child_index;
	}

	inline size_t ImplNextItemQuad(
		const _T_PositionType& x,
		const _T_PositionType& y,
		const _T_PositionType& quad_x,
		const _T_PositionType& quad_y) const {
		size_t child_index = 0;
		if (x > quad_x)	child_index += 1;
		if (y > quad_y)	child_index += 2;
		return child_index;
	}

	bool IsInRange(
		const _T_PositionType& x,
		const _T_PositionType& y) const {
		return (x >= m_RegionMinX || x < m_RegionMaxX) &&
			(y >= m_RegionMinY || y < m_RegionMaxY);
	}

	void ImplClear() {
		ImplClear(m_RootNode);
		m_RootNode = nullptr;
	}

	void ImplClear(_TreeNode* node) {
		//Yep, another slow piece of code...
		if (node != nullptr) {
			if (node->IsLeaf) {
				//Deallocate memory for item
				_U_ItemNodeAllocTraits::deallocate(
					m_ItemAllocator,
					static_cast<_ItemNode*>(node),
					1
				);
			}
			else {
				ImplClear(static_cast<_LinkNode*>(node)->Childs[0]);
				ImplClear(static_cast<_LinkNode*>(node)->Childs[1]);
				ImplClear(static_cast<_LinkNode*>(node)->Childs[2]);
				ImplClear(static_cast<_LinkNode*>(node)->Childs[3]);
				//Deallocate memory for link
				_U_LinkNodeAllocTraits::deallocate(
					m_LinkAllocator,
					static_cast<_LinkNode*>(node),
					1
				);
			}
		}
	}

	//A bunch of code which should speedup overwiting already existing elements
	//	in the nutshell : we just prioritize assign operator over constructor if available
	template <
		typename _T_ImplDataType,
		typename std::enable_if<
		std::is_copy_assignable<_T_ImplDataType>::value
		, bool>::type = true>
	inline void ImplUpdateItem(_LinkNode* parent,
		size_t child_index,
		const _T_ImplDataType& data) {
		static_cast<_ItemNode*>(parent->Childs[child_index])->Data = data;
	}

	template <
		typename _T_ImplDataType,
		typename std::enable_if<
		std::is_move_assignable<_T_ImplDataType>::value
		, bool>::type = true>
	inline void ImplUpdateItem(_LinkNode* parent,
		size_t child_index,
		_T_ImplDataType&& data) {
		static_cast<_ItemNode*>(parent->Childs[child_index])->Data = std::move(data);
	}

	template <
		typename _T_ImplDataType,
		typename std::enable_if<
		!std::is_copy_assignable<_T_ImplDataType>::value &&
		!std::is_move_assignable<_T_ImplDataType>::value&&
		std::is_copy_constructible<_T_ImplDataType>::value
		, bool>::type = true>
	inline void ImplUpdateItem(_LinkNode* parent,
		size_t child_index,
		const _T_ImplDataType& data) {
		//Deallocate memory for previous node
		_U_ItemNodeAllocTraits::deallocate(
			m_ItemAllocator,
			static_cast<_ItemNode*>(parent->Childs[child_index]),
			1
		);
		//Create _ItemNode
		_ItemNode* new_item_node =
			_U_ItemNodeAllocTraits::allocate(m_ItemAllocator, 1);
		//Construct _ItemNode
		_U_ItemNodeAllocTraits::construct(
			m_ItemAllocator,
			new_item_node,
			parent, data
		);
		//Assign new node to parent link as child
		parent->Childs[child_index] = new_item_node;
	}

	template <
		typename _T_ImplDataType,
		typename std::enable_if<
		!std::is_copy_assignable<_T_ImplDataType>::value &&
		!std::is_move_assignable<_T_ImplDataType>::value&&
		std::is_move_constructible<_T_ImplDataType>::value
		, bool>::type = true>
	inline void ImplUpdateItem(_LinkNode* parent,
		size_t child_index,
		_T_ImplDataType&& data) {
		//Deallocate memory for previous node
		_U_ItemNodeAllocTraits::deallocate(
			m_ItemAllocator,
			static_cast<_ItemNode*>(parent->Childs[child_index]),
			1
		);
		//Create _ItemNode
		_ItemNode* new_item_node =
			_U_ItemNodeAllocTraits::allocate(m_ItemAllocator, 1);
		//Construct _ItemNode
		_U_ItemNodeAllocTraits::construct(
			m_ItemAllocator,
			new_item_node,
			parent, std::move(data)
		);
		//Assign new node to parent link as child
		parent->Childs[child_index] = new_item_node;
	}


public:
	//Minimal forward iterator for quadtree class
	// very crude and limited
	struct QuadTreeIterator
	{
		using iterator_category = std::forward_iterator_tag;
		using difference_type = std::ptrdiff_t;

		QuadTreeIterator(_TreeNode* current_node) :
			m_CurrentNode(current_node),
			m_PathStack({ -1 }) {
			//Immediatly advance to first node
			ImplAdvance();
		};

		~QuadTreeIterator() {
		}

		_T_ValueType& operator*() {
			if (m_CurrentNode != nullptr) {
				return static_cast<_ItemNode*>(m_CurrentNode)->Data;
			}
			return *(reinterpret_cast<_T_ValueType*>(m_DummyBuffer));
		}

		QuadTreeIterator& operator++() {
			ImplAdvance();
			return (*this);
		}

		QuadTreeIterator operator++(int) {
			auto tmp = *this;
			ImplAdvance();
			return tmp;
		}

		friend bool operator== (const QuadTreeIterator& a, const QuadTreeIterator& b) {
			return a.m_CurrentNode == b.m_CurrentNode;
		};
		friend bool operator!= (const QuadTreeIterator& a, const QuadTreeIterator& b) {
			return a.m_CurrentNode != b.m_CurrentNode;
		};

	private:

		inline void ImplAdvance() {

			//Check if m_CurrentNode is not null
			// we treat nullptr m_CurrentNode as iterator end
			if (m_CurrentNode == nullptr) {
				return;
			}

			_LinkNode* link_node = nullptr;
			_Td_ChildIdType child_path = -1;

			//Check if we are at root node
			if (m_CurrentNode->Parent == nullptr) {
				//We are at the root - root is always a link node
				// - cast current_node to link_node and assign result
				link_node = static_cast<_LinkNode*>(m_CurrentNode);
				//root doesnt require path_stack pop
			}
			else {
				//We are at the item node - all item nodes have a parent to link node
				// - cast parent to link_node and assign result
				link_node = static_cast<_LinkNode*>(m_CurrentNode->Parent);
				//path stack pop is required to keep track of which node already traversed
				child_path = m_PathStack.back();
				m_PathStack.pop_back();
			}


			//Begin traversing...
			while (link_node != nullptr) {

				//Pick next path
				child_path += 1;
				//NOTE: 
				// if we at root the value is set to -1
				// it is totally fine because after increment
				// the value will be set to 0 - first child of root_link

				//Check if child_path is out of bounds...
				if (child_path == 4) {
					//Yep, we finished traversing current link
					// going up...

					//Set current link node to current->parent
					link_node = static_cast<_LinkNode*>(link_node->Parent);
					//Pop path stack
					child_path = m_PathStack.back();
					m_PathStack.pop_back();
					continue; //Repeat cycle
				}

				//We assume the path is valid

				//Check if child link is valid...
				if (link_node->Childs[child_path] != nullptr) {
					//Yep, it is...

					//Check if child node is leaf...
					if (link_node->Childs[child_path]->IsLeaf) {
						//Yep, we found item_node

						//Update current
						m_CurrentNode = link_node->Childs[child_path];
						//Push taken path to the stack
						m_PathStack.push_back(child_path);
						break; // Exit cycle
					}
					else {
						//if it is not a leaf

						//Update current link node 
						link_node = static_cast<_LinkNode*>(link_node->Childs[child_path]);
						//Push taken path to the stack
						m_PathStack.push_back(child_path);
						//Reset child_path to -1
						child_path = -1;
						continue; // Repeat cycle
					}
				}
			}
			//If we exited from cycle by link_node == nullptr condition
			//then we probably found the end of tree or smth went horribly wrong
			// anyway set m_CurrentNode to nullptr, reset m_PathStack and we done here
			if (link_node == nullptr) {
				m_CurrentNode = nullptr;
				m_PathStack.clear();
			}
		}

		//Current node
		_TreeNode* m_CurrentNode;
		//Path to m_CurrentNode
		std::deque<_Td_ChildIdType,
			_Td_ChildIdTAllocType>
			m_PathStack;
		//Dummy buffer which are represent as dummy _T_ValueType
		alignas(_T_ValueType)
			char m_DummyBuffer[sizeof(_T_ValueType)];
	};
public:

	struct _TreeNode {
		_TreeNode(_TreeNode* parent, bool isleaf) :
			Parent(parent),
			IsLeaf(isleaf) {}

		bool	  IsLeaf;
		_TreeNode* Parent;
	};

	struct _LinkNode : public _TreeNode {
		_LinkNode() :
			_LinkNode::_LinkNode(nullptr) {}
		_LinkNode(_TreeNode* parent) :
			_TreeNode::_TreeNode(parent, false),
			Childs{ nullptr } {}

		_TreeNode* Childs[4];
	};

	struct _ItemNode : public _TreeNode {
		_ItemNode(_TreeNode* parent, const _T_ValueType& data) :
			_TreeNode::_TreeNode(parent, true),
			Data(data) {}
		_ItemNode(_TreeNode* parent, _T_ValueType&& data) :
			_TreeNode::_TreeNode(parent, true),
			Data(std::move(data)) {}

		_T_ValueType Data;
	};
};