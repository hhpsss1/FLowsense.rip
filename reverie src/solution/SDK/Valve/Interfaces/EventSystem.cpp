#include <shared.h>

void CGameEventListener::Init(const std::deque<const char*>& events) {
	if (events.empty())
		return;

	for (auto& event_name : events)
		GameEventManager->AddListener(this, event_name);
}

void CGameEventListener::Reset() {
	GameEventManager->RemoveListener(this);
}

void CGameEventListener::FireGameEvent(CGameEvent* event) {
	if (!event)
		return;

	Events->OnFireGameEvent(event);
}