#pragma once

#include <ny/include.hpp>
#include <ny/base/file.hpp>
#include <ny/draw/color.hpp>

#include <nytl/vec.hpp>

#include <string>
#include <istream>
#include <ostream>
#include <memory>

namespace ny
{

//TODO: some functions (e.g. size, (TODO: add scale function)) not implemented
//TODO: load from memory buffer
///Represents a single image which can be loaded from a file and on which can be rendered.
class Image : public File
{
public:
	enum class Format
	{
		rgba8888,
		bgra8888,
		argb8888,
		rgb888,
		a8
	};

	///Returns the size of the given format in bytes.
	///E.g. Format::rgba8888 would return 4, since one pixel of this format needs 4 bytes to
	///be stored.
	static unsigned int formatSize(Format f);

public:
    Image(const Vec2ui& size = {0u, 0u}, Format format = Format::rgba8888);
    Image(const std::string& path);
	Image(const std::uint8_t* data, const Vec2ui& size, Format format);
	Image(std::unique_ptr<std::uint8_t[]>&& data, const Vec2ui& size, Format format);

    virtual ~Image() = default;

    Image(const Image& other);
    Image& operator=(const Image& other);

	//noexcept?
	Image(Image&& other) = default;
	Image& operator=(Image&& other) = default;

	///Returns the total size of the stored data in bytes.
	///This is width * heigth * formatSize(format)
	std::size_t dataSize() const;

	///\{
	///Returns a pointer to the stored data.
	///The data is stored row major and without any gaps.
	std::uint8_t* data() { return data_.get(); }
	const std::uint8_t* data() const { return data_.get(); }
	///\}

	///\{
	///Changes the data, size and format of the new image.
	///If size and format should stay the same, just pass the old values.
    void data(const std::uint8_t* newdata, const Vec2ui& newsize, Format newFormat);
    void data(std::unique_ptr<std::uint8_t[]>&& newdata, const Vec2ui& newsize, Format newFormat);
	///\}

	///Returns a copy of the data as unique_ptr.
	std::unique_ptr<std::uint8_t[]> copyData() const;

	///Returns the format of the image.
    Format format() const { return format_; }

	///Changes the format of the image and converts the stored data.
	///Can be useful if native apis require different image formats.
	void format(Format format);

	///Retusn the number of bytes needed to store one byte, i.e. the formatSize of the used format.
	unsigned int pixelSize() const { return formatSize(format_); }

	///Returns the color at the given pixel.
	Color at(const Vec2ui& pos) const;

	///Returns the size of the imag.e
    const Vec2ui& size() const { return size_; }

	///Changes the size of the image and resized the stored data.
	///New pixels are initialized black.
	///\warning does NOT scale the image, but simply cut/extend it.
	void size(const Vec2ui& newSize);

	///Loads the image from a given input stream.
	///Note that this might also fail when the image format cannot be detected.
    bool load(std::istream& is);

	///Saves the image to the given output stream with the given image format.
    bool save(std::ostream& os, const std::string& type) const;

    //from file
	///Loads the image from a file.
	///Note that this version should be rather used that the version with an input stream,
	///since the image format can (if needed) be detected by file extension.
	virtual bool load(const std::string& path) override;

	///Saves the image to the given path.
	virtual bool save(const std::string& path) const override;

protected:
	std::unique_ptr<std::uint8_t[]> data_ {nullptr};
	Vec2ui size_ {0u, 0u};
	Format format_ {Format::rgba8888};
};


///Represents an animatedImage (e.g. gif file) which can hold multiple images with animation
///delays.
class AnimatedImage : public File
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
