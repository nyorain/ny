#pragma once

#include <ny/draw/include.hpp>

namespace ny
{

///Texture is the abstract base class of DrawContext-specific textures. 
///Draw Contexts use their own implementations (derived classes) to store texture
///objects they are able to render. So if you e.g. want to render an image
///with openGL multiple times, you can create an openGL texture for it and then use this
///texture object for rendering instead of always using the raw image (as ImageTexture)
///which can speed up the rednering. 
///So Textures as basically context-specific pixel buffers that can be transformed into
///an ny::Image, but using a texture is usually more efficient. 
///If a DrawContext gets a texture object they can not deal with (use a openGL texture to
///draw on a cairo surface e.g.) then it will just use the image() function of the texture
///object to redner it.
class Texture 
{
public:
	virtual ~Texture() = default;

	virtual Image* image() const = 0;
};

///Default Texture implementation which just uses a Image to represent its contents.
///With this class, all Images can automatically be expressed as Texture type which makes
///it possible to just use plain images for brushes.
class ImageTexture : public Texture
{
protected:
	Image* image_ = nullptr;

public:
	ImageTexture() = default;
	ImageTexture(Image& img) : image_(&img) {}
	virtual ~ImageTexture() = default;

	virtual Image* image() const override { return image_; }
	void image(Image& img) { image_ = &img; }
};

}
