#include <geom/Line.hpp>

namespace geom {

Line::Line(Vec2 s, Vec2 e) : a_(s), b_(e)
{}

Line::Line(const Line& o) : a_(o.a_), b_(o.b_)
{}

Line::Line(Line&& o) : a_(o.a_), b_(o.b_)
{}

Line& Line::operator=(const Line& o) {
	a_ = o.a_;
	b_ = o.b_;
	
	return *this;
}

Line& Line::operator=(Line&& o) {
	a_ = o.a_;
	b_ = o.b_;

	return *this;
}

Vec2 Line::start() const {
	return a_;
}

Vec2 Line::end() const {
	return b_;
}

double Line::len() const {
	return (b_ - a_).Length();
}

double Line::lenSquared() const {
	return (b_ - a_).LengthSquared();
}

}
