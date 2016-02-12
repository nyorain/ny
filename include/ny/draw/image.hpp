#pragma once

#include <ny/include.hpp>
#include <ny/app/file.hpp>
#include <nytl/vec.hpp>

#include <string>
#include <istream>
#include <ostream>
#include <memory>

namespace ny
{

///Represnts a single image which can be loaded from a file and on which can be rendered.
class Image : public File
{
public:
	enum class Format
	{
		rgba8888,
		rgb888,
		xrgb8888,
		a8
	};

	static unsigned int formatSize(Format f);

protected:
	std::unique_ptr<std::uint8_t[]> data_ {nullptr};
	Vec2ui size_ {0u, 0u};
	Format format_ {Format::rgba8888};

public:
    Image(const Vec2ui& size = {0u, 0u}, Format format = Format::rgba8888);
    Image(const std::string& path);
	Image(const std::uint8_t* data, const Vec2ui& size, Format format);
	Image(std::unique_ptr<std::uint8_t[]>&& data, const Vec2ui& size, Format format);

    virtual ~Image() = default;

    Image(const Image& other);
    Image& operator=(const Image& other);

	//noexcept? :o
	Image(Image&& other) = default;
	Image& operator=(Image&& other) = default;

	//
	std::size_t dataSize() const;

	std::uint8_t* data() { return data_.get(); }
	const std::uint8_t* data() const { return data_.get(); }
    void data(const std::uint8_t* newdata, const Vec2ui& newsize, Format newFormat);
    void data(std::unique_ptr<std::uint8_t[]>&& newdata, const Vec2ui& newsize, Format newFormat);
	std::unique_ptr<std::uint8_t[]> copyData() const;

    unsigned int pixelSize() const;
    Format format() const { return format_; }

    const Vec2ui& size() const { return size_; }
	void size(const Vec2ui& newSize);

    bool load(std::istream& is);
    bool save(std::ostream& os, const std::string& type) const;

    //from file
	virtual bool load(const std::string& path) override;
	virtual bool save(const std::string& path) const override;
};


///Represents an animatedImage (e.g. gif file) which can hold multiple images with animation
///delays.
class animatedImage : public File
{
protected:
	std::vector<std::pair<Image, unsigned int>> images_;
	Vec2ui size_;

public:
    Image* image(std::size_t i);
    Image* operator[](std::size_t i) { return image(i); }

    size_t imageCount() const { return images_.size(); }
    const Vec2ui& size() const { return size_; };

    //from file
    virtual bool load(const std::string& path) override;
    virtual bool save(const std::string& path) const override;
};

}
