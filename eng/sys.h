#pragma once

#include "ui/ui.h"
#include "ui/window.h"
#include "rhi/rhi.h"
#include "rhi/resource.h"
#include "utl/update_queue.h"
#include <chrono>

namespace eng {

enum class UpdateType {
	None,
	Sys,
	World,
};

struct World;
struct Renderer;
struct Scene;
struct Ui;

struct Sys {

	~Sys();

	bool Init();
	bool InitRhi(std::shared_ptr<Window> const &window, int32_t deviceIndex = 0);

	std::shared_ptr<rhi::Texture> LoadTexture(std::string path, bool genMips);

	void UpdateTime();

	utl::UpdateQueue::Time _timeScale = 1.0;
	std::chrono::steady_clock::time_point _lastUpdateTime;
	std::shared_ptr<rhi::Rhi> _rhi;
	std::unique_ptr<Ui> _ui;
	std::unique_ptr<World> _world;
	std::unique_ptr<Scene> _scene;
	utl::UpdateQueue _updateQueue;

	static Sys *Get() { return s_instance.get(); }
	static bool InitInstance();
	static void DoneInstance();
	static inline std::unique_ptr<Sys> s_instance;
};

}