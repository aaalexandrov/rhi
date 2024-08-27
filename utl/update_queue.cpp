#include "update_queue.h"

namespace utl {

void UpdateQueue::Schedule(Updatable *updatable, Time updateTime, uintptr_t userData)
{
	ASSERT(updatable);
	// on updating data that of an updatable that's already scheduled, we only update the UpdataData's updatable, but leave it in the queue in order to avoid a linear search through it
	// then we create a different UpdateData for the same updatable and add that to the queue as well
	// the old data will be removed from the queue during the execution of UpdateToTime()

	auto lock = std::unique_lock(_rwlock);
	auto itRegistered = _updatables.find(updatable);
	if (itRegistered != _updatables.end()) {
		ASSERT(itRegistered->second->_updatable == updatable);
		if (updateTime == itRegistered->second->_time) {
			itRegistered->second->_userData = userData;
			return;
		}
		itRegistered->second->_updatable = nullptr;
	}
	if (updateTime == TimeNever) {
		if (itRegistered != _updatables.end())
			_updatables.erase(itRegistered);
		return;
	}
	if (itRegistered == _updatables.end()) 
		itRegistered = _updatables.insert(std::make_pair(updatable, std::shared_ptr<UpdateData>())).first;
	itRegistered->second = std::make_shared<UpdateData>(updatable, userData, updateTime);
	_queue.push(itRegistered->second);
}

void UpdateQueue::UpdateToTime(Time delta)
{
	auto lock = std::unique_lock(_rwlock);
	Time time = _lastUpdateTime + delta;
	ASSERT(time > _lastUpdateTime);
	while (!_queue.empty() && _queue.top()->_time <= time) {
		std::shared_ptr<UpdateData> update = _queue.top();
		_queue.pop();
		if (!update->_updatable)
			continue;
		update->_time = std::max(time + 0.00001, update->_updatable->Update(time, update->_userData));
		ASSERT(update->_time > time);
		if (update->_time == TimeNever)
			continue;
		_queue.push(std::move(update));
	}
	_lastUpdateTime = time;
}

}