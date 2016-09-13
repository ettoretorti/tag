#include <geom/Circle.hpp>
#include <geom/Polygon.hpp>

namespace geom {

Circle::Circle(Vec2 c, float r) : c_(c), r_(r)
{}

Circle::Circle(const Circle& o) : c_(o.c_), r_(o.r_)
{}

Circle::Circle(Circle&& o) : c_(o.c_), r_(o.r_)
{}

Circle& Circle::operator=(const Circle& o) {
	c_ = o.c_;
	r_ = o.r_;

	return *this;
}

Circle& Circle::operator=(Circle&& o) {
	c_ = o.c_;
	r_ = o.r_;

	return *this;
}

Vec2 Circle::center() const {
	return c_;
}

float Circle::radius() const {
	return r_;
}

double Circle::area() const {
	return (r_ * r_) * 3.141592653589793;
}

}
