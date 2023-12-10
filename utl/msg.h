#pragma once

#include <cstring>

namespace utl {

template <typename Fn>
struct Runner {
	Runner(Fn &&fn) { fn(); }
};

template <typename Fn>
struct OnDestroy {
	Fn _fn;
	OnDestroy(Fn fn) : _fn(fn) {}
	~OnDestroy() {
		_fn();
	}
};

template <typename... Args>
struct Msg {
	using Func = std::function<void(Args...)>;

	struct Handler {
		char const *_id = nullptr;
		Func _func;
		bool operator<(Handler const &other) const { return strcmp(_id, other._id) < 0; }
		bool operator<(char const *id) const { return strcmp(_id, id) < 0; }
		friend bool operator<(char const *id, Handler const &h) { return strcmp(id, h._id) < 0; }
	};

	std::vector<Handler> _handlers;
	bool _sorted = false;

	void Clear() {
		_handlers.clear();
	}

	auto Register(char const *id, Func &&fn) {
		return Runner([this, id, fn = std::move(fn)] { Add(id, std::move(fn)); });
	}

	void Add(char const *id, Func fn) {
		_handlers.push_back({ ._id = id, ._func = fn });
		_sorted = false;
	}

	bool Remove(char const *id) {
		return EraseSorted(_handlers, id);
	}

	void Call(Args... args) {
		if (!_sorted) {
			std::sort(_handlers.begin(), _handlers.end());
			_sorted = true;
		}
		for (auto &h : _handlers) {
			h._func(args...);
		}
		ASSERT(_sorted);
	}
};

}