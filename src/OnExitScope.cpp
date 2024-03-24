#include <DesktopMop/OnExitScope.hpp>

OnExitScope::OnExitScope(std::function<void()> f) {
	Add(f);
}

OnExitScope::~OnExitScope(){
	for(auto it = functions.rbegin(); it != functions.rend(); ++it){
		(*it)();
	}
}

void OnExitScope::Add(std::function<void()> f) {
	functions.push_back(f);
}