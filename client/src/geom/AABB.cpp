#include <geom/AABB.hpp>

namespace geom {

AABB::AABB(Vec2 c, Vec2 e) : c_(c), e_(e)
{}

AABB::AABB(const AABB& o) : c_(o.c_), e_(o.e_)
{}

AABB::AABB(AABB&& o) : c_(o.c_), e_(o.e_)
{}

AABB& AABB::operator=(const AABB& o) {
	c_ = o.c_;
	e_ = o.e_;

	return *this;
}

AABB& AABB::operator=(AABB&& o) {
	c_ = o.c_;
	e_ = o.e_;
	
	return *this;
}

Vec2 AABB::center() const {
	return c_;
}

Vec2 AABB::extents() const {
	return e_;
}

double AABB::area() const {
	auto tmp = e_ + e_;
	return tmp.x() * tmp.y();
}

}
