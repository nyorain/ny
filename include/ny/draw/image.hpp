#pragma once

#include <ny/draw/include.hpp>
#include <ny/app/file.hpp>
#include <nytl/vec.hpp>

#include <vector>
#include <string>
#include <istream>
#include <ostream>

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
	std::vector<unsigned char> data_;
	vec2ui size_ {0u, 0u};
	Format format_ {Format::rgba8888};

public:
    Image(const vec2ui& size = {0u, 0u}, Format format = Format::rgba8888);
    Image(const std::string& path);
    virtual ~Image() = default;

    Image(const Image& other) = default;
    Image& operator=(const Image& other) = default;

	//noexcept? :o
	Image(Image&& other) = default;
	Image& operator=(Image&& other) = default;

	//
	std::vector<unsigned char>& data() { return data_; }
	const std::vector<unsigned char>& data() const { return data_; }
    void data(const std::vector<unsigned char>& newdata, const vec2ui& newsize);

	std::vector<unsigned char> copyData() const;

    unsigned int pixelSize() const;
    Format format() const { return format_; }

    const vec2ui& size() const { return size_; }
	void size(const vec2ui& newSize);

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
	vec2ui size_;

public:
    Image* image(std::size_t i);
    Image* operator[](std::size_t i) { return image(i); }

    size_t imageCount() const { return images_.size(); }
    const vec2ui& size() const { return size_; };

    //from file
    virtual bool load(const std::string& path) override;
    virtual bool save(const std::string& path) const override;
};

}
