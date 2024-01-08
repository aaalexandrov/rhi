#pragma once

#include "ui/ui.h"
#include "ui/window.h"
#include "rhi/rhi.h"
#include "rhi/resource.h"

namespace eng {

struct World;
struct Ui;

struct Sys {

	~Sys();

	bool Init();
	bool InitRhi(std::shared_ptr<Window> const &window, int32_t deviceIndex = 0);


	std::shared_ptr<rhi::Rhi> _rhi;
	std::unique_ptr<Ui> _ui;
	std::unique_ptr<World> _world;


	static Sys *Get() { return s_instance.get(); }
	static bool InitInstance();
	static void DoneInstance();
	static inline std::unique_ptr<Sys> s_instance;
};

}