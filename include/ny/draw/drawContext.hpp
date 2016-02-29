#pragma once

#include <ny/include.hpp>
#include <ny/draw/brush.hpp>
#include <ny/draw/shape.hpp>

#include <nytl/vec.hpp>
#include <nytl/rect.hpp>
#include <nytl/nonCopyable.hpp>

namespace ny
{

///RAII guard for using DrawContexts.
///Simplex Assures that the DrawContext is initialized before using it and will be applied
///on guard destruction.
class DrawGuard
{
protected:
	DrawContext& drawContext_;

public:
	DrawGuard(DrawContext& ctx);
	~DrawGuard();

	///Returns the associated DrawContext.
	///Note that the function signature specifies that this function can be only called on lvalue
	///instances which should prevent to abuse DrawGuard in expressions mistakenly instead of
	///storing them.
	DrawContext& dc() & { return drawContext_; }
};

///The DrawContext is the abstract base class interface which defines how to draw on a surface.
///The surface can be e.g. a window, an image or a svg document.
///To draw with a given DrawContext you can either clear/paint the entire
///surface with a given Brush, or you specifiy a Mask which should then be
///filled with a given Brush or stroked with a given Pen.
class DrawContext : public NonCopyable
{
public:
	DrawContext() = default;
	virtual ~DrawContext() = default;

	///Assures that the DrawContext is ready to be drawn on.
	///This should be called before starting to draw.
	///Note that this function might throw if the implementation is unable to initialize.
	///See the different implementations for more information on that (many do not actually need
	///this function at all).
	///Especially needed for more complex (e.g. hardware-acclerated) implementations, e.g. to
	///make the needed opengl context current.
	virtual void init(){}

	///Applies the pending DrawContext state to the surface.
	///This should always be called when finishing drawing, altough some implementations
	///may automatically apply any shape that was drawn. 
	virtual void apply(){}


	///Clears the contents of the surface with the given brush.
	virtual void clear(const Brush& b = Brush::none);

	///Uses the alphaMask parameter to paint the surface with the given brush.
	virtual void paint(const Brush& alphaMask, const Brush& brush) = 0;

	///Adds a Mask object to the DrawContexts mask state.
	virtual void mask(const std::vector<PathBase>& obj);

	///Adds a Path object to the DrawContexts mask state
	virtual void mask(const PathBase& obj);

	///Resets the current mask state i.e. clears all stored mask paths.
	virtual void resetMask() = 0;


	///Adds a CustomPath object to the DrawContexts mask state.
    virtual void mask(const Path& obj) = 0;

	///Adds a Text objecdt to the DrawContexts mask state.
	virtual void mask(const Text& obj) = 0;

	///Adds a Rectangle object to the DrawContexts mask state
	virtual void mask(const Rectangle& obj);

	///Adds a Circle to the DrawContexts mask state
	virtual void mask(const Circle& obj);


	///Fills the current mask with the given Brush and resets the mask state.
	virtual void fill(const Brush& col);

	///Strokes the current mask with the given Pen and resets the mask state.
	virtual void stroke(const Pen& col);

	///Fills the current mask without resetting it.
	virtual void fillPreserve(const Brush& col) = 0;

	///Strokes the current mask without resetting it.
	virtual void strokePreserve(const Pen& col) = 0;


	///Clears the current mask and draws the given shape.
	virtual void draw(const Shape& obj);


	///Returns if this DrawContext suppports mask clipping. You should always check this before
	///using mask clipping function calls, since they will have no effect and raise a warning
	///if it is not. The OpenGL implementations which deal with a context without stencil buffers
	///are for example not able to support mask clipping. If you just need to clip a Rectangle
	///you should use clipRectangle().
	virtual bool maskClippingSupported() const { return 0; }

	///Adds the current mask to the clipping area and rests the current mask state.
	///This means that everything outside the current mask will not be drawn anymore.
	///This function will only work if the DrawContext object supports mask clipping. Else it
	///will ave no effect on DrawContext and surface, but it will raise a warning.
    virtual void clipMask();

	///Adds the current mask to the clipping area without resrtting the current mask state.
	///This function will only work if the DrawContext object supports mask clipping. Else it
	///will have no effect on DrawContext and surface, but it will raise a warning.
	virtual void clipMaskPreserve();

	///Returns the current mask clip area. Will return an empty mask vector and rasie a warning
	///if mask clipping is not supported.
	virtual std::vector<PathBase> maskClip() const;

	///Resets the current clip mask area. Will only work if the DrawContext supports mask
	///clipping. Else it will have no effect on DrawContext and surface, but it
	///will raise a warning.
	virtual void resetMaskClip();


	///Uses the given Rectangle as additional clip area. If there is already a mask clip
	///area it will remain the same. Everything outside this Rectangle (or outside the
	///mask clipping area) will not be drawn. If there was already set a Rectangle clip before
	///it will be changed to the new one, you can always just clip one Rectangle with Rectangle
	///clipping. This is intented as some kind of fallback if mask clipping is not supported
	///(e.g. gl implementation without stencil buffer), it must be supported by every
	///implementation.
	virtual void clipRectangle(const Rect2f& rectangle) = 0;

	///Returns the current clipped Rectangle. By default, this is a Rectangle with the
	///position(0,0) and the same size as the surface, since everything outside the clipping
	///Rectangle is not be drawn. For more information see clipRectangle().
	virtual Rect2f rectangleClip() const = 0;

	///Resets the current clipping Rectangle, so everything drawn on the surface
	///will be visible and nothing will be clipped. For more information see clipRectangle().
	virtual void resetRectangleClip() = 0;
};



///The RedirectDrawContext is one DrawContext implementation
///can be used to draw on just a part of another DrawContext.
///It is basically able to redirect its draw calls to a certain area in another drawContext, it
///will translate every mask call and clip the draw context it redirects to.
///Useful for e.g. drawing child gui elements on a parent window
///RAII class that directly influences the redirected context (i.e. clips) during its lifetime.
class RedirectDrawContext : public DrawContext
{
protected:
	Rect2f rectangleClipSave_ {};
	std::vector<PathBase> maskClipSave_ {};

	Vec2f size_ {};
    Vec2f position_ {};

    DrawContext* redirect_ = nullptr;

protected:
	///This function prepares the redirected draw context for drawing. To call any DrawContext
	///function of this object before startDrawing is called is undefined behaviour.
    void startDrawing();

	///This function cleans up the redirected DrawContext. To call any DrawContext
	///function of this object after endDrawing is called is undefined behaviour.
	void endDrawing();

public:
	///Constructs the RedirectDrawContext with a given original DrawContext, to which all
	///drawings should be redirected in the rectangular area that the position and size
	///describe.
	RedirectDrawContext(DrawContext& redirect, const Vec2f& position, const Vec2f& size);
	virtual ~RedirectDrawContext();

	virtual void apply() override;
	virtual void clear(const Brush& b = Brush()) override;
	virtual void paint(const Brush& alphaMask, const Brush& brush) override;

	virtual void mask(const Path& obj) override;
	virtual void mask(const Circle& obj) override;
	virtual void mask(const Rectangle& obj) override;
	virtual void mask(const Text& obj) override;
	virtual void mask(const PathBase& b) override;
	virtual void resetMask() override;

	virtual void fill(const Brush& col) override;
	virtual void stroke(const Pen& col) override;

	virtual void fillPreserve(const Brush& col) override;
	virtual void strokePreserve(const Pen& col) override;

	virtual bool maskClippingSupported() const override;
	virtual void clipMask() override;
	virtual void clipMaskPreserve() override;
	virtual std::vector<PathBase> maskClip() const override;
	virtual void resetMaskClip() override;

	virtual void clipRectangle(const Rect2f& obj) override;
    virtual Rect2f rectangleClip() const override;
	virtual void resetRectangleClip() override;

	///Changes the size of the area that should be drawn on.
	void size(const Vec2f& size);

	///Changes the position of the area that should be drawn on.
	void position(const Vec2f& position);

	///Returns the size of the virtual surface (the area this DrawContext draws on).
	const Vec2f& size() const { return size_; }

	///Returns the position of the area all drawings are redirected to.
	const Vec2f& position() const { return position_; }

	///Returns the extents of the area all drawing are redirected to.
	Rect2f extents() const { return Rect2f(position(), size()); }

	///Returns the DrawContext that all drawings are redirected to.
	DrawContext& redirect() const { return *redirect_; }
};


///Abstract base class for implementations that use a backend without real masking system (e.g. gl).
///All mask calls will be stored in a vector<PathBase> and will only be applied to the
///surface when some fill or stroke method is called (or clear).
///Does not differ the masking functions from its DrawContext base class.
///The pure virtual stroke and fill functions from DrawContext must be implemented in 
///derived classes (as well as masking-related functions), they can just use the storedMask()
///function to get the current mask.
class DelayedDrawContext : public DrawContext
{
protected:
	std::vector<PathBase> mask_;

public:
	virtual void mask(const Path& obj) override;
	virtual void mask(const Circle& obj) override;
	virtual void mask(const Rectangle& obj) override;
	virtual void mask(const Text& obj) override;
	virtual void mask(const PathBase& b) override;
	virtual void resetMask() override;

	///Returns the current (stored) mask of this context.
	const std::vector<PathBase>& storedMask() const { return mask_; }
};

}
