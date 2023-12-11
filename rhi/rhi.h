#pragma once

#include "resource.h"
#include "pass.h"
#include "submit.h"

namespace rhi {

struct Rhi : public std::enable_shared_from_this<Rhi>, public utl::Any {

	struct Settings {
		char const *_appName = nullptr;
		glm::uvec3 _appVersion{ 0 };
		bool _enableValidation = false;
		std::shared_ptr<WindowData> _window;
	};

	virtual ~Rhi();

	virtual bool Init(Settings const &settings);

	bool InitTypes();

	std::shared_ptr<RhiOwned> Create(TypeInfo const *type, std::string name = "");
	template <typename R>
	std::shared_ptr<R> Create(std::string name = "") {
		return std::static_pointer_cast<R>(Create(TypeInfo::Get<R>()));
	}

	std::shared_ptr<Submission> Submit(std::vector<std::shared_ptr<Pass>> &&passes, std::string name = "");

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Rhi>(); }

	TypeInfo const *GetDerivedTypeWithTag(TypeInfo const *base);
	template <typename T>
	TypeInfo const *GetDerivedTypeWithTag() {
		return GetDerivedTypeWithTag(TypeInfo::Get<T>());
	}

	Settings _settings;
protected:
	std::shared_mutex _rwLock;
	std::unordered_map<TypeInfo const *, TypeInfo const *> _derivedTypes;
};

struct RhiVk;

}