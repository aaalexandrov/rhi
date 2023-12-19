#pragma once

#include "algo.h"
#include "msg.h"

namespace utl {

struct TypeInfo;
struct alignas(std::max_align_t) AnyValue {
	static constexpr size_t StorageSize = 16;
	std::array<uint8_t, StorageSize> _storage = {};
	TypeInfo const *_type = nullptr;

	AnyValue() = default;
	AnyValue(AnyValue &&other) { 
		std::swap(_type, other._type); 
		std::swap(_storage, other._storage); 
	}
	~AnyValue() { Clear(); }

	AnyValue &operator=(AnyValue &&other)  { 
		Clear(); 
		std::swap(_type, other._type); 
		std::swap(_storage, other._storage); 
		return *this;
	}

	void Clear();

	void *Get(TypeInfo const *type = nullptr);
	template <typename T>
	T *Get();

	void const *Get(TypeInfo const *type = nullptr) const { return const_cast<AnyValue *>(this)->Get(type); }
	template <typename T>
	T const *Get() const { return const_cast<AnyValue *>(this)->Get<T>(); }

	template <typename T>
	AnyValue &Set(T &&val);

	template <typename T>
	static AnyValue New(T &&val) {
		AnyValue anyVal;
		anyVal.Set(std::move(val));
		return anyVal;
	}
};

struct AnyValueMap {
	AnyValueMap() = default;
	AnyValueMap(AnyValueMap &&) = default;

	std::unordered_map<std::string, AnyValue> _values;

	AnyValue *Get(std::string name) {
		auto it = _values.find(name);
		return it != _values.end() ? &it->second : nullptr;
	}
	void *Get(std::string name, TypeInfo const *type) {
		AnyValue *anyVal = Get(name);
		return anyVal ? anyVal->Get(type) : nullptr;
	}
	template <typename T>
	T *Get(std::string name) {
		AnyValue *anyVal = Get(name);
		return anyVal ? anyVal->Get<T>() : nullptr;
	}

	AnyValue const *Get(std::string name) const {
		return const_cast<AnyValueMap *>(this)->Get(name);
	}
	void const *Get(std::string name, TypeInfo const *type) const {
		return const_cast<AnyValueMap *>(this)->Get(name, type);
	}
	template <typename T>
	T const *Get(std::string name) const {
		return const_cast<AnyValueMap *>(this)->Get<T>(name);
	}

	void Set(std::string name, AnyValue &&anyVal) {
		_values.insert({ name, std::move(anyVal) });
	}
	template <typename T>
	void Set(std::string name, T &&val) {
		Set(name, AnyValue::New(std::move(val)));
	}
};

template <typename T>
struct TypeInitializer;

struct TypeInfo {
	struct Variable {
		TypeInfo const *_type = nullptr;
		size_t _offset = 0;
	};

	struct Member {
		std::string _name;
		Variable _var;
		std::vector<AnyValue> _metadata;
	};

	using Constructor = std::function<void(void *)>;
	using Destructor = std::function<void(void *)>;
	using Initializer = std::function<void()>;
	using TypesEnum = std::function<Enum(TypeInfo const *)>;

	size_t _size = 0, _align = 0;
	std::string _name;
	size_t _arraySize = 0;
	std::vector<Variable> _bases;
	std::vector<TypeInfo const *> _derived;
	std::vector<Member> _members;
	AnyValueMap _metadata;
	Constructor _constructor;
	Destructor _destructor;
	union {
		struct {
			uint32_t _isInitialized : 1;
			uint32_t _isRegistered : 1;
			uint32_t _isPointer : 1;
			uint32_t _isArray : 1;
			uint32_t _isTrivial : 1;
			uint32_t _isAbstract : 1;
		};
		uint32_t _flags = 0;
	};

	TypeInfo const *GetElementType() const;

	ptrdiff_t GetBaseOffset(TypeInfo const *base) const;
	Variable GetMemberData(std::string name) const;

	AnyValue const *GetMetadata(std::string name) const;
	void const *GetMetadata(std::string name, TypeInfo const *type) const {
		AnyValue const *val = GetMetadata(name);
		return val ? val->Get(type) : nullptr;
	}
	template <typename T>
	T const *GetMetadata(std::string name) const {
		AnyValue const *val = GetMetadata(name);
		return val ? val->Get<T>() : nullptr;
	}

	bool IsKindOf(TypeInfo const *other) const { return GetBaseOffset(other) >= 0; }
	template<typename T>
	bool IsKindOf() const { return IsKindOf(Get<T>()); }

	template <typename T>
	static TypeInfo const *Get() { return Storage<std::remove_cvref_t<T>>(); }
	static TypeInfo const *Get(std::string typeName);

	template <typename T>
	static TypeInitializer<T> Register();

	void *CastAs(TypeInfo const *dstType, void *instance) const {
		ptrdiff_t dstOffs = GetBaseOffset(dstType);
		return instance && dstOffs >= 0 ? (uint8_t *)instance + dstOffs : nullptr;
	}
	template <typename T>
	T *CastAs(void *instance) const {
		return (T*)CastAs(Get<T>(), instance);
	}

	void *Construct(void *at, bool allocate) const;
	void Destruct(void *at, bool deallocate) const;

	template <typename T>
	T *ConstructAs(void *at, bool allocate) const {
		return CastAs<T>(Construct(at, allocate));
	}

	Enum ForDerivedTypes(TypesEnum fn) const;

	static auto AddInitializer(char const *id, Initializer fnInitializer); 
	static void Init();
	static void Done();
private:

	struct Registry {
		std::shared_mutex _rwTypes;
		std::mutex _mutMsg;
		Msg<> _msgInit;
		std::unordered_map<std::string, TypeInfo *> _typesByName;

		TypeInfo *Get(std::string typeName);
		auto AddInitializer(char const *id, Initializer fnInitializer) {
			std::unique_lock lock(_mutMsg);
			return _msgInit.Register(id, std::move(fnInitializer));
		}
		void RegisterType(TypeInfo *type);
		void CallMsgInit();
		void ClearTypes();
	};

	static inline Registry s_registry;

	template <typename T>
	static TypeInfo *Storage() {
		static TypeInfo s_info;
		if (!s_info._isInitialized) {
			s_info._isInitialized = true;
			if constexpr (requires { sizeof(T); })
				s_info._size = sizeof(T);
			if constexpr (requires { alignof(T); })
				s_info._align = alignof(T);
			s_info._name = typeid(T).name();
			s_info._isTrivial = std::is_trivial_v<T>;
			s_info._isAbstract = std::is_abstract_v<T>;
			if constexpr (std::is_constructible_v<T>) {
				s_info._constructor = [](void *at) { return new(at) T(); };
			}
			if constexpr (std::is_destructible_v<T>) {
				if constexpr (std::is_array_v<T>) {
					s_info._destructor = [](void *at) { 
						using Elem = std::remove_extent_t<T>;
						for (size_t i = 0; i < std::extent_v<T>; ++i) 
							((Elem *)((uint8_t*)at + i * sizeof(Elem)))->~Elem();
					};
				} else {
					s_info._destructor = [](void *at) { ((T *)at)->~T(); };
				}
			}
			if constexpr (fixed_array_traits<T>::is_array) {
				s_info._isArray = 1;
				s_info._arraySize = fixed_array_traits<T>::size;
				s_info._bases.push_back({ ._type = Get<typename fixed_array_traits<T>::value_type>() });
			}
			if constexpr (std::is_pointer_v<T>) {
				s_info._isPointer = true;
				s_info._bases.push_back({ ._type = Get<std::remove_pointer_t<T>>() });
			}
			s_registry.RegisterType(&s_info);
		}
		return &s_info;
	}
};

struct Any {
	virtual TypeInfo const *GetTypeInfo() const = 0;
};

inline void *Cast(TypeInfo const *dstType, TypeInfo const *valType, void *val) {
	if (!val)
		return nullptr;
	ptrdiff_t offs = valType->GetBaseOffset(dstType);
	return offs >= 0 ? (uint8_t*)val + offs : nullptr;
}

template <typename T> 
T *Cast(TypeInfo const *valType, void *val) {
	return (T*)Cast(TypeInfo::Get<T>(), valType, val);
}

template <typename T>
T *Cast(Any *any) {
	return any ? Cast<T>(any->GetTypeInfo(), any) : nullptr;
}

template <typename T>
std::shared_ptr<T> Cast(std::shared_ptr<Any> const &anyShared) {
	return anyShared && anyShared->GetTypeInfo()->IsKindOf<T>() ? std::static_pointer_cast<T>(anyShared) : std::shared_ptr<T>();
}

template <typename T>
struct TypeInitializer {
	TypeInfo *_type;
	int32_t _lastMember = -1;

	TypeInitializer(TypeInfo *type) : _type{ type } { ASSERT(!_type->_isRegistered); _type->_isRegistered = true; }

	TypeInitializer &Name(std::string name) {
		_type->_name = name;
		return *this;
	}

	TypeInitializer &Constructor(TypeInfo::Constructor ctor) {
		_type->_constructor = ctor;
		return *this;
	}

	TypeInitializer &Destructor(TypeInfo::Destructor dtor) {
		_type->_destructor = dtor;
		return *this;
	}

	template <typename B>
	TypeInitializer &Base() {
		static_assert(std::is_base_of_v<B, T>);
		TypeInfo *base = const_cast<TypeInfo *>(TypeInfo::Get<B>());
		_type->_bases.push_back({ ._type = base, ._offset = (size_t)(B *)(T *)0x1000 - 0x1000 });
		base->_derived.push_back(_type);
		return *this;
	}

	TypeInitializer &Member(std::string name, TypeInfo const *memberType, size_t memberOffset) {
		_lastMember = _type->_members.size();
		_type->_members.push_back({ ._name = name, ._var = { ._type = memberType, ._offset = memberOffset } });
		return *this;
	}
	template <typename M>
	TypeInitializer &Member(std::string name, size_t memberOffset) {
		return Member(name, TypeInfo::Get<M>(), memberOffset);
	}
	template <typename M>
	TypeInitializer &Member(std::string name, M memberPtr) {
		auto *ptr = &(((T *)nullptr)->*memberPtr);
		return Member<decltype(*ptr)>(name, (size_t)ptr);
	}
	TypeInitializer &MemberMetadata(AnyValue &&val) {
		ASSERT(0 <= _lastMember && _lastMember < _type->_members.size());
		_type->_members[_lastMember]._metadata.push_back(std::move(val));
	}
	template <typename V>
	TypeInitializer &MemberMetadata(V &&val) {
		return MemberMetadata(AnyValue::New(std::move(val)));
	}

	TypeInitializer &Metadata(std::string name, AnyValue &&val) {
		_type->_metadata.Set(name, std::move(val));
		return *this;
	}
	template <typename V>
	TypeInitializer &Metadata(std::string name, V &&val) {
		return Metadata(name, AnyValue::New(std::move(val)));
	}
};

template<typename T>
inline TypeInitializer<T> TypeInfo::Register()
{
	return TypeInitializer<T>(const_cast<TypeInfo*>(Get<T>()));
}

inline auto TypeInfo::AddInitializer(char const *id, Initializer fnInitializer)
{
	return s_registry.AddInitializer(id, fnInitializer);
}

template<typename T>
inline T *AnyValue::Get()
{
	return (T*)Get(TypeInfo::Get<T>());
}

template<typename T>
inline AnyValue &AnyValue::Set(T &&val)
{
	ASSERT((size_t)_storage.data() % alignof(std::max_align_t) == 0);
	Clear();
	_type = TypeInfo::Get<T>();
	T *ptr = nullptr;
	if (_type->_size <= _storage.size() && ((size_t)_storage.data() % _type->_align) == 0) {
		ptr = (T *)_storage.data();
	} else {
		ptr = (T*)malloc(_type->_size);
		*(T **)_storage.data() = ptr;
	}
	ASSERT((size_t)ptr % _type->_align == 0);
	*ptr = std::move(val);
	return *this;
}

}