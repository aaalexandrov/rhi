#pragma once

namespace rhi {
struct Rhi;
}

namespace eng {

struct Window;
struct Ui {
	~Ui();

	bool Init();

	void Done();
};

}