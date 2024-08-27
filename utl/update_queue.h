#pragma once

#include <queue>
#include <unordered_map>

namespace utl {

struct UpdateQueue {
	using Time = double;
	static constexpr Time TimeNever = std::numeric_limits<Time>::max();

	struct Updatable {
		virtual ~Updatable() {}
		virtual Time Update(Time now, uintptr_t userData) = 0;
	};

	struct UpdateData {
		Updatable *_updatable = nullptr;
		uintptr_t _userData = 0;
		Time _time = 0;

		bool operator <(UpdateData const &rhs) const {
			return _time > rhs._time;
		}
	};

	std::shared_mutex _rwlock;
	std::priority_queue<std::shared_ptr<UpdateData>> _queue;
	std::unordered_map<Updatable *, std::shared_ptr<UpdateData>> _updatables;
	Time _lastUpdateTime = 0;

	void Schedule(Updatable *updatable, Time updateTime, uintptr_t userData);
	void UpdateToTime(Time delta);

};

}