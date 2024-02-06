#pragma once

#include "geom.h"
#include <bit>

namespace utl {

template <typename VecType, typename NodeDataType>
struct BoxTree {
	using Vec = VecType;
	using Num = typename VecTraits<Vec>::ElemType;
	static constexpr int Dim = VecTraits<Vec>::Length;
	using BoxV = Box<Vec>;

	using NodeData = NodeDataType;

	struct Node {
		NodeData _data;
		uint8_t _childMask = 0;
	};

	void Init(BoxV const &initialBox, Vec minNodeSize);
	void Clear() { _nodes.clear(); }

	uint32_t GetNodeIndex(BoxV const &box);
	BoxV GetNodeBox(uint32_t nodeIndex);

	static BoxV GetChildBox(BoxV const &box, uint8_t childIndex);

	Node *GetNode(uint32_t index, bool addMissing = false);
	bool RemoveNode(uint32_t index);

	template <typename FnEnum>
	utl::Enum EnumNodes(FnEnum fnEnum);
	template <typename FnEnum>
	utl::Enum EnumNodes(FnEnum fnEnum, uint32_t nodeIndex, BoxV const &nodeBox);


	BoxV _box;
	Vec _minNodeSize;
	std::unordered_map<uint32_t, std::unique_ptr<Node>> _nodes;
};

template<typename VecType, typename NodeDataType>
inline void BoxTree<VecType, NodeDataType>::Init(BoxV const &initialBox, Vec minNodeSize) {
	Clear();
	_box = initialBox;
	_minNodeSize = minNodeSize;
	ASSERT(all(greaterThan(_box.GetSize(), Vec(0))));
	ASSERT(all(greaterThanEqual(_minNodeSize, Vec(0))));
	ASSERT(all(lessThanEqual(_minNodeSize, _box.GetSize())));
	GetNode(1, true);
}

template<typename VecType, typename NodeDataType>
inline uint32_t BoxTree<VecType, NodeDataType>::GetNodeIndex(BoxV const &box)
{
	if (!box.Intersects(_box))
		return 0;
	uint32_t nodeIndex = 1;
	BoxV nodeBox = _box;

	while (all(lessThan(_minNodeSize, nodeBox.GetSize()))) {
		Vec center = nodeBox.GetCenter();
		uint32_t childIndex = 0;
		for (int d = 0; d < Dim; ++d) {
			if (box._max[d] <= center[d]) {
				nodeBox._max[d] = center[d];
			} else if (box._min[d] >= center[d]) {
				nodeBox._min[d] = center[d];
				childIndex |= 1 << d;
			} else {
				childIndex = ~0;
				break;
			}
		}
		if (childIndex == ~0)
			break;
		nodeIndex = (nodeIndex << Dim) | childIndex;
	}

	return nodeIndex;
}

template<typename VecType, typename NodeDataType>
inline auto BoxTree<VecType, NodeDataType>::GetNodeBox(uint32_t nodeIndex) -> BoxV
{
	ASSERT(nodeIndex > 0);

	BoxV nodeBox = _box;
	int indexShift = std::bit_width(nodeIndex);
	ASSERT(indexShift % Dim == 0);

	while (true) {
		indexShift -= Dim;
		if (indexShift < 0)
			break;
		nodeBox = GetChildBox(nodeBox, nodeIndex >> indexShift);
	}

	return nodeBox;
}

template<typename VecType, typename NodeDataType>
inline auto BoxTree<VecType, NodeDataType>::GetChildBox(BoxV const &box, uint8_t childIndex) -> BoxV
{
	BoxV childBox = box;
	Vec center = childBox.GetCenter();
	for (int d = 0; d < Dim; ++d) {
		if (childIndex & (1 << d))
			childBox._min[d] = center[d];
		else
			childBox._max[d] = center[d];
	}
	return childBox;
}

template<typename VecType, typename NodeDataType>
inline auto BoxTree<VecType, NodeDataType>::GetNode(uint32_t index, bool addMissing) -> Node*
{
	auto it = _nodes.find(index);
	if (it != _nodes.end())
		return it->second.get();
	if (!addMissing)
		return nullptr;
	Node *node = _nodes.emplace(index, std::make_unique<Node>()).first->second.get();
	uint32_t parentIndex = index >> Dim;
	if (parentIndex) {
		Node *parent = GetNode(parentIndex, true);
		uint32_t childIndex = index & ((1 << Dim) - 1);
		ASSERT((parent->_childMask & (1 << childIndex)) == 0);
		parent->_childMask |= 1 << childIndex;
	}
	return node;
}

template<typename VecType, typename NodeDataType>
inline bool BoxTree<VecType, NodeDataType>::RemoveNode(uint32_t index)
{
	auto it = _nodes.find(index);
	if (it == _nodes.end())
		return false;
	if (index == 1)
		return false; // we should always have a root node
	ASSERT(it->second->_childMask == 0);
	_nodes.erase(it);
	uint32_t parentIndex = index >> Dim;
	if (parentIndex) {
		Node *parent = GetNode(parentIndex);
		uint32_t childIndex = index & ((1 << Dim) - 1);
		ASSERT((parent->_childMask & (1 << childIndex)) != 0);
		parent->_childMask &= ~(1 << childIndex);
	}
	return true;
}

template<typename VecType, typename NodeDataType>
template<typename FnEnum>
inline utl::Enum BoxTree<VecType, NodeDataType>::EnumNodes(FnEnum fnEnum)
{
	return EnumNodes(fnEnum, 1, _box);
}

template<typename VecType, typename NodeDataType>
template<typename FnEnum>
inline utl::Enum utl::BoxTree<VecType, NodeDataType>::EnumNodes(FnEnum fnEnum, uint32_t nodeIndex, BoxV const &nodeBox)
{
	Node *node = GetNode(nodeIndex);
	ASSERT(node);
	utl::Enum enumRes = fnEnum(node, nodeIndex, nodeBox);
	if (enumRes != utl::Enum::Continue)
		return enumRes;
	for (uint8_t childIndex = 0; childIndex < (1 << Dim); ++childIndex) {
		if ((node->_childMask & (1 << childIndex)) == 0)
			continue;
		uint32_t childNodeIndex = (nodeIndex << Dim) | childIndex;
		BoxV childBox = GetChildBox(nodeBox, childIndex);
		if (EnumNodes(fnEnum, childNodeIndex, childBox) == utl::Enum::Stop)
			return utl::Enum::Stop;
	}
	return utl::Enum::Continue;
}

}