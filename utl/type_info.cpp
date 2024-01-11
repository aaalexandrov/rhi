#include "type_info.h"
#include "msg.h"

namespace utl {

static auto s_regTypes = TypeInfo::AddInitializer("type_info", [] {
	TypeInfo::Register<void>().Name("void");
	TypeInfo::Register<bool>().Name("bool");
	TypeInfo::Register<wchar_t>().Name("wchar_t");
	TypeInfo::Register<std::string>().Name("std::string");

	TypeInfo::Register<uint8_t>().Name("uint8_t");
	TypeInfo::Register<uint16_t>().Name("uint16_t");
	TypeInfo::Register<uint32_t>().Name("uint32_t");
	TypeInfo::Register<uint64_t>().Name("uint64_t");

	TypeInfo::Register<int8_t>().Name("int8_t");
	TypeInfo::Register<int16_t>().Name("int16_t");
	TypeInfo::Register<int32_t>().Name("int32_t");
	TypeInfo::Register<int64_t>().Name("int64_t");

	TypeInfo::Register<float>().Name("float");
	TypeInfo::Register<double>().Name("double");

	TypeInfo::Register<Any>().Name("Any");
	TypeInfo::Register<AnyValue>().Name("AnyValue");
});

static bool s_testTypes = [] {

	struct Base1 {
		int _i;
	};

	struct Base2 {
		float _f;
	};

	struct Derived1 : public Base1, public Base2 {
		uint32_t _u;
	};

	struct Derived2 : public Derived1 {
		char _c;
	};

	TypeInfo::Register<Base1>().Name("Base1")
		.Member("_i", &Base1::_i);

	TypeInfo::Register<Base2>().Name("Base2")
		.Member("_f", &Base2::_f);

	TypeInfo::Register<Derived1>().Name("Derived1")
		.Base<Base1>()
		.Base<Base2>()
		.Member("_u", &Derived1::_u);

	TypeInfo::Register<Derived2>().Name("Derived2")
		.Base<Derived1>()
		.Member("_c", &Derived2::_c);

	auto tBase1 = TypeInfo::Get<Base1>();
	auto tBase2 = TypeInfo::Get<Base2>();
	auto tDerived1 = TypeInfo::Get<Derived1>();
	auto tDerived2 = TypeInfo::Get<Derived2>();

	Derived2 d2;
	auto m_i = tDerived2->GetMemberData("_i");
	auto m_f = tDerived2->GetMemberData("_f");
	auto m_u = tDerived2->GetMemberData("_u");
	auto m_c = tDerived2->GetMemberData("_c");

	auto d2IsB2 = tDerived2->IsKindOf<Base2>();
	auto d2IsAny = tDerived2->IsKindOf<Any>();
	auto b2IsD1 = tBase2->IsKindOf(tDerived1);


	auto tInt = TypeInfo::Get<int>();
	auto tInt2 = TypeInfo::Get<float[2]>();
	auto tpDouble = TypeInfo::Get<double *>();
	auto tpArrInt2 = TypeInfo::Get<std::array<int, 2>>();
	auto tVoid = TypeInfo::Get<void>();

	auto tvec4 = TypeInfo::Get<glm::vec4>();

	AnyValue aInt = AnyValue::New<int>(5);

	return true;
}();


void *AnyRef::Get(TypeInfo const *type)
{
	if (!_type || !_instance)
		return nullptr;
	if (!type)
		type = _type;
	ptrdiff_t typeOffs = _type->GetBaseOffset(type);
	if (typeOffs < 0)
		return nullptr;
	return (uint8_t *)_instance + typeOffs;
}

size_t AnyRef::GetArraySize() const
{
	if (!_type || !_instance)
		return 0;
	return _type->GetArraySize();
}

AnyRef AnyRef::GetArrayElement(size_t index)
{
	if (!_type || !_instance)
		return AnyRef();
	if (index >= _type->GetArraySize())
		return AnyRef();
	ASSERT(_type->_bases[0]._offset == 0);
	TypeInfo const *elemType = _type->_bases[0]._type;
	return AnyRef{
		._type = elemType,
		._instance = (uint8_t*)_instance + index * elemType->_size,
	};
}

AnyRef AnyRef::GetMember(std::string name)
{
	if (!_type || !_instance)
		return AnyRef();
	TypeInfo::Variable member = _type->GetMemberData(name);
	if (!member._type)
		return AnyRef();
	return AnyRef{
		._type = member._type,
		._instance = (uint8_t*)_instance + member._offset,
	};
}

void AnyValue::Clear()
{
	if (!_type)
		return;
	void *ptr = Get();
	_type->Destruct(ptr, ptr != _storage.data());
	_type = nullptr;
	memset(_storage.data(), 0, _storage.size());
}

void *AnyValue::Get(TypeInfo const *type)
{
	static_assert(StorageSize >= sizeof(uint8_t *));
	if (!_type)
		return nullptr;
	if (!type)
		type = _type;
	ptrdiff_t typeOffs = _type->GetBaseOffset(type);
	if (typeOffs < 0)
		return nullptr;
	uint8_t *ptr = nullptr;
	if (CanUseInternalStorage())
		ptr = _storage.data();
	else
		ptr = *(uint8_t **)_storage.data();
	return ptr + typeOffs;
}

TypeInfo const *TypeInfo::GetElementType() const
{
	if (_isArray || _isPointer) {
		ASSERT(_bases.size() == 1);
		return _bases[0]._type;
	}
	return nullptr;
}

ptrdiff_t TypeInfo::GetBaseOffset(TypeInfo const *base) const
{
	if (base == this)
		return 0;
	if (!base || _isPointer != base->_isPointer || _isArray != base->_isArray)
		return -1;
	if (_isArray) // arrays need to match exactly
		return -1;
	if (_isPointer && _bases[0]._type->IsKindOf(base->_bases[0]._type))
		return 0;
	for (auto &b : _bases) {
		ptrdiff_t baseOffs = b._type->GetBaseOffset(base);
		if (baseOffs >= 0)
			return baseOffs + b._offset;
	}
	return -1;
}

auto TypeInfo::GetMemberData(std::string name) const -> Variable
{
	for (auto &member : _members) {
		if (member._name == name)
			return member._var;
	}
	if (_isArray || _isPointer)
		return {};
	for (auto &base : _bases) {
		if (Variable var = base._type->GetMemberData(name); var._type) {
			return { ._type = var._type, ._offset = var._offset + base._offset };
		}
	}
	return {};
}

std::vector<AnyValue> const *TypeInfo::GetMemberMetadata(std::string name) const
{
	for (auto &member : _members) {
		if (member._name == name)
			return &member._metadata;
	}
	if (_isArray || _isPointer)
		return nullptr;
	for (auto &base : _bases) {
		if (auto *meta = base._type->GetMemberMetadata(name)) 
			return meta;
	}
	return nullptr;
}

AnyValue const *TypeInfo::GetMemberMetadata(std::string name, TypeInfo const *metaType) const
{
	std::vector<AnyValue> const *metaArr = GetMemberMetadata(name);
	if (!metaArr)
		return nullptr;
	for (AnyValue const &val : *metaArr) {
		if (val._type->IsKindOf(metaType))
			return &val;
	}
	return nullptr;
}

AnyValue const *TypeInfo::GetMetadata(std::string name) const
{
	AnyValue const *val = _metadata.Get(name);
	if (val)
		return val;
	if (_isArray || _isPointer)
		return nullptr;
	for (auto &base : _bases) {
		val = base._type->GetMetadata(name);
		if (val)
			return val;
	}
	return nullptr;
}

TypeInfo const *TypeInfo::Get(std::string typeName)
{
	return s_registry.Get(typeName);
}

void *TypeInfo::Construct(void *at, bool allocate) const
{
	ASSERT(allocate == !at);
	ASSERT(_constructor);
	if (!_constructor)
		return nullptr;
	if (allocate) {
		at = malloc(_size);
	}
	ASSERT(size_t(at) % _align == 0);
	_constructor(at);
	return at;
}

void TypeInfo::Destruct(void *at, bool deallocate) const
{
	ASSERT(_destructor);
	if (!at || !_destructor)
		return;
	ASSERT(size_t(at) % _align == 0);
	_destructor(at);
	if (deallocate) {
		free(at);
	}
}

Enum TypeInfo::ForDerivedTypes(TypesEnum fn) const
{
	for (auto derived : _derived) {
		Enum res = fn(derived);
		if (res == Enum::SkipChildren)
			continue;
		if (res == Enum::Stop ||
			derived->ForDerivedTypes(fn) == Enum::Stop)
			return Enum::Stop;
	}
	return Enum::Continue;
}

void TypeInfo::Init()
{
	s_registry.CallMsgInit();
}

void TypeInfo::Done()
{
	s_registry.ClearTypes();
}

inline TypeInfo *TypeInfo::Registry::Get(std::string typeName) 
{
	std::shared_lock lock(_rwTypes);
	auto it = _typesByName.find(typeName);
	return it != _typesByName.end() ? it->second : nullptr;
}

inline void TypeInfo::Registry::RegisterType(TypeInfo *type) 
{
	std::unique_lock lock(_rwTypes);
	bool inserted = _typesByName.insert({ type->_name, type }).second;
	ASSERT(inserted);
}

inline void TypeInfo::Registry::CallMsgInit() 
{
	std::unique_lock lock(_mutMsg);
	_msgInit.Call();
	_msgInit.Clear();
}

inline void TypeInfo::Registry::ClearTypes() 
{
	std::unique_lock lock(_rwTypes);
	// clear all AnyValues from all registered types so they won't crash on destruction when the types they reference have already been destroyed
	for (auto &[name, type] : _typesByName) {
		type->_metadata._values.clear();
	}
	_typesByName.clear();
}


}