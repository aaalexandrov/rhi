#pragma once

#include "resource.h"
#include "pass.h"
#include "submit.h"

namespace rhi {

struct Rhi : public std::enable_shared_from_this<Rhi>, public utl::Any {

	struct DeviceDescription {
		std::string _name;
		uint32_t _vendorId = 0;
		uint32_t _deviceId = 0;
		uint32_t _driverVersion = 0;
		bool _isIntegrated = false;
	};

	struct Settings {
		char const *_appName = nullptr;
		glm::uvec3 _appVersion{ 0 };
		bool _enableValidation = false;
		std::shared_ptr<WindowData> _window;
	};

	virtual ~Rhi();

	virtual std::vector<DeviceDescription> GetDevices(Settings const &settings) = 0;
	DeviceDescription GetInitializedDevice();

	virtual bool Init(Settings const &settings, int32_t deviceIndex = 0);

	bool InitTypes();

	std::shared_ptr<RhiOwned> Create(TypeInfo const *type, std::string name = "");
	template <typename R>
	std::shared_ptr<R> Create(std::string name = "") {
		return std::static_pointer_cast<R>(Create(TypeInfo::Get<R>(), name));
	}

	template <typename R, typename... ARGS>
	std::shared_ptr<R> New(std::string name, ARGS &&... args) {
		auto obj = std::static_pointer_cast<R>(Create(TypeInfo::Get<R>(), name));
		if (!obj)
			return std::shared_ptr<R>();
		if (!obj->Init(std::forward<ARGS>(args)...)) {
			ASSERT(0);
			return std::shared_ptr<R>();
		}
		return obj;
	}

	std::shared_ptr<Submission> Submit(std::vector<std::shared_ptr<Pass>> &&passes, std::string name = "");

	virtual bool WaitIdle() = 0;

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Rhi>(); }

	TypeInfo const *GetDerivedTypeWithTag(TypeInfo const *base);
	template <typename T>
	TypeInfo const *GetDerivedTypeWithTag() {
		return GetDerivedTypeWithTag(TypeInfo::Get<T>());
	}

	Settings _settings;
	int32_t _deviceIndex = -1;
protected:
	std::shared_mutex _rwLock;
	std::unordered_map<TypeInfo const *, TypeInfo const *> _derivedTypes;
};

struct RhiVk;

}