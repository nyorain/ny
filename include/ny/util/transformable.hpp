#pragma once

#include <ny/include.hpp>

#include <nyutil/mat.hpp>
#include <nyutil/vec.hpp>
#include <nyutil/rect.hpp>

namespace ny
{

template<size_t dim, class prec> class transformable
{
protected:
	typedef vec<dim, prec> pVec;
	typedef squareMat<dim + 1, prec> pMat;
	typedef rect<dim, prec> pRect;

    prec rotation_ {0}; //todo, must be vector, too?
    pVec scale_ {};
    pVec translation_ {};

	pVec origin_ {}; //relative origin of the object.

public:
    transformable(){ scale_.setAllTo(1); translation_.setAllTo(0); origin_.setAllTo(0); }
    template<size_t odim, class oprec> transformable(const transformable<odim, oprec>& other) :rotation_(other.rotation_), scale_(other.scale_), translation_(other.translation_), origin_(other.origin_) {};

    void rotate(prec rotation){ rotation_ += rotation; }
    void translate(const pVec& translation){ translation_ += translation; }
    void scale(const pVec& pscale){ scale_ *= pscale; }

    void setRotation(prec rotation){ rotation_ = rotation; }
    void setTranslation(const pVec& translation){ translation_ = translation; }
    void setScale(const pVec& pscale){ scale_ = pscale; }

    const prec& getRotation() const { return rotation_; }
    const pVec& getTranslation() const { return translation_; }
    const pVec& getScale() const { return scale_; }

    prec& getRotation() { return rotation_; }
    pVec& getTranslation() { return translation_; }
    pVec& getScale() { return scale_; }

	const pVec& getOrigin() const { return origin_; }
	void setOrigin(const pVec& origin) { origin_ = origin; }
	void moveOrigin(const pVec& m) { origin_ += m; };

	void copyTransform(const transformable<dim, prec>& other){ rotation_ = other.rotation_; scale_ = other.scale_; translation_ = other.translation_; origin_ = other.origin_; };

    //todo
    pMat getTransformMatrix() const { pMat ret; return ret; }

	virtual pRect getExtents() const { return rect<dim, prec>(); }
	pRect getTransformedExtents() const { return getExtents(); } //todo: sth like getExtents() * getTransformMatrix()
};

}
