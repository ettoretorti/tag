#include <geom/Polygon.hpp>

#include <cassert>
#include <cmath>

namespace geom {

Polygon::Polygon(int nPoints, const Vec2* points)
	: ps_(new Vec2[nPoints]), n_(nPoints)
{
	assert(n_ > 2);

	for(int i = 0; i < n_; i++) {
		ps_[i] = points[i];
	}
}

Polygon::Polygon(std::initializer_list<Vec2> points)
	: ps_(new Vec2[points.size()]), n_(points.size())
{
	assert(n_ > 2);
	
	int i = 0;
	for(auto& p : points) {
		ps_[i++] = p;
	}
}

Polygon::Polygon(const Circle& ci, int sides)
	: ps_(new Vec2[sides]), n_(sides)
{
	float theta = (2.0 * 3.141592) / sides;
	float s = sin(theta);
	float c = cos(theta);

	float x = ci.radius();
	float y = 0.0;

	for(int i = 0; i < sides; i++) {
		ps_[i] = Vec2(x, y) + ci.center();
		float tmp = x;
		x = x * c - y * s;
		y = tmp * s + y * c;
	}
}

Polygon::Polygon(const Polygon& o) : ps_(new Vec2[o.n_]), n_(o.n_)
{
	for(int i = 0; i < n_; i++) {
		ps_[i] = o.ps_[i];
	}
}

Polygon::Polygon(Polygon&& o) : ps_(o.ps_), n_(o.n_)
{
	o.ps_ = nullptr;
	o.n_ = 0;	
}

Polygon& Polygon::operator=(const Polygon& o) {
	if(!ps_ || n_ != o.n_) {
		delete[] ps_;
		ps_ = new Vec2[o.n_];
		n_ = o.n_;
	}
	
	for(int i = 0; i < n_; i++) {
		ps_[i] = o.ps_[i];
	}

	return *this;
}

Polygon& Polygon::operator=(Polygon&& o) {
	auto t1 = ps_;
	auto t2 = n_;

	ps_ = o.ps_;
	n_ = o.n_;

	o.ps_ = t1;
	o.n_ = t2;

	return *this;
}

Polygon::~Polygon() {
	delete[] ps_;
}

int Polygon::size() const {
	return n_;
}

Vec2 Polygon::point(int i) const {
	assert(i < n_);

	return ps_[i];
}

Line Polygon::edge(int i) const {
	assert(i < n_);

	if(i + 1 == n_) {
		return Line(ps_[i], ps_[0]);
	}

	return Line(ps_[i], ps_[i + 1]);
}

double Polygon::area() const {
	double acc = 0;
	
	for(int i = 0; i < n_ - 1; i++) {
		acc += ps_[i].x() * ps_[i + 1].y();
	}
	acc += ps_[n_ - 1].x() * ps_[0].y();
	
	for(int i = 0; i < n_ - 1; i++) {
		acc -= ps_[i].y() * ps_[i + 1].x();
	}
	acc -= ps_[n_ - 1].y() * ps_[0].x();

	return acc / 2;
}

Vec2 Polygon::centroid() const {
	Vec2 acc(0.0f, 0.0f);

	for(int i = 0; i < n_; i++) {
		acc += ps_[i];
	}

	return acc / n_;
}

Circle Polygon::boundingCircle() const {
	Vec2 c = centroid();

	float max = (ps_[0] - c).LengthSquared();
	
	for(int i = 1; i < n_; i++) {
		max = std::max(max, (ps_[i] - c).LengthSquared());
	}

	return Circle(c, max);
}

AABB Polygon::boundingBox() const {
	Vec2 min = ps_[0];
	Vec2 max = ps_[0];

	for(int i = 1; i < n_; i++) {
		min.x() = std::min(min.x(), ps_[i].x());
		min.y() = std::min(min.y(), ps_[i].y());
		max.x() = std::max(max.x(), ps_[i].x());
		max.y() = std::max(max.y(), ps_[i].y());
	}

	Vec2 avg = (min + max) / 2;

	return AABB(avg, avg - min);
}

}
