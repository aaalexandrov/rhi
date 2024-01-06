#pragma once

#include "rhi/rhi.h"
#include "rhi/resource.h"

namespace eng {

struct World;

struct Sys {



	std::shared_ptr<rhi::Rhi> _rhi;
	std::shared_ptr<rhi::Swapchain> _swapchain;
	std::unique_ptr<World> _world;

	static bool Init();
	static void Done();
	static std::unique_ptr<Sys> _instance;
};

}