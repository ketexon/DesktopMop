#pragma once

#include <functional>
#include <vector>

class OnExitScope {
private:
	std::vector<std::function<void()>> functions;
public:
	OnExitScope() = default;
	OnExitScope(std::function<void()>);
	~OnExitScope();

	void Add(std::function<void()>);
};