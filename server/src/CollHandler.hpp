#pragma once

#include <Box2D/Box2D.h>

#include <functional>
#include <utility>

class CollHandler : public b2ContactListener {
public:
	using CollPair = std::pair<unsigned, unsigned>;
	
	CollHandler(std::function<void(CollPair)> cb) : _cb(cb) {}

	virtual void BeginContact(b2Contact* contact) {
		if(contact->IsTouching()) {
			b2Body* a = contact->GetFixtureA()->GetBody();
			b2Body* b = contact->GetFixtureB()->GetBody();

			if(a->GetType() == b2_dynamicBody && b->GetType() == b2_dynamicBody) {
				unsigned fst = (unsigned) (uintptr_t) a->GetUserData();
				unsigned snd = (unsigned) (uintptr_t) b->GetUserData();

				_cb(std::make_pair(fst, snd));
			}
		}
	}

private:
	std::function<void(CollPair)> _cb;
};
