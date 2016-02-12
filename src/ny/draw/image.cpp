#include <ny/draw/image.hpp>
#include <nytl/make_unique.hpp>
#include <ny/base/log.hpp>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_write.h"

#include <cstring>
#include <fstream>

namespace ny
{

//static
unsigned int Image::formatSize(Image::Format f)
{
	switch(f)
	{
		case Format::rgba8888: case Format::xrgb8888: return 4;
		case Format::rgb888: return 3;
		case Format::a8: return 1;
		default: return 0;
	}
}

//stbi Callbacks/util
namespace
{    
	int read(void* user, char* data, int size)
    {
        std::istream* stream = static_cast<std::istream*>(user);
        stream->read(data, size);
		return stream->gcount();
    }
    void skip(void* user, int size)
    {
        std::istream* stream = static_cast<std::istream*>(user);
        stream->seekg(static_cast<int>(stream->tellg()) + size, stream->beg);
    }
    int eof(void* user)
    {
        std::istream* stream = static_cast<std::istream*>(user);

		auto curr = stream->tellg();
		stream->seekg(0, stream->end);
		std::size_t size = static_cast<std::size_t>(stream->tellg());
        stream->seekg(curr, stream->beg);

        return static_cast<std::size_t>(stream->tellg()) >= size;
    }

	void write(void* user, void* data, int size)
	{
		std::ostream* stream = static_cast<std::ostream*>(user);
		stream->write(static_cast<char*>(data), size);
	}
}

//Image
Image::Image(const Vec2ui& size, Format format) : File(), size_(size), format_(format)
{
	data_ = make_unique<std::uint8_t[]>(dataSize());
}

Image::Image(const std::string& path) : File(path)
{
	//problematic since virtual...
	load(path);
}

Image::Image(const std::uint8_t* data, const Vec2ui& size, Format format)
	: File(), size_(size), format_(format)
{
	data_ = make_unique<std::uint8_t[]>(dataSize());
	std::memcpy(data_.get(), data, dataSize());
}

Image::Image(std::unique_ptr<std::uint8_t[]>&& data, const Vec2ui& size, Format format)
	: File(), data_(std::move(data)), size_(size), format_(format)
{
}

Image::Image(const Image& other)
	: File(other), size_(other.size_), format_(other.format_)
{
	data_ = other.copyData();
}

Image& Image::operator=(const Image& other)
{
	File::operator=(other);
	size_ = other.size_;
	format_ = other.format_;
	data_ = other.copyData();

	return *this;
}

void Image::data(const std::uint8_t* newdata, const Vec2ui& newsize, Format newFormat)
{
	format_ = newFormat;
	size_ = newsize;

	data_ = make_unique<std::uint8_t[]>(dataSize());
	std::memcpy(data_.get(), newdata, dataSize());

	change();
}

void Image::data(std::unique_ptr<std::uint8_t[]>&& newdata, const Vec2ui& newsize, Format newFormat)
{
	format_ = newFormat;
	size_ = newsize;
	data_ = std::move(newdata);

	change();
}

std::unique_ptr<std::uint8_t[]> Image::copyData() const
{
	auto ret = make_unique<std::uint8_t[]>(dataSize());
	std::memcpy(ret.get(), data_.get(), dataSize());
	return ret;
}

unsigned int Image::pixelSize() const
{
	return formatSize(format_);
}

std::size_t Image::dataSize() const
{
	return size_.x * size_.y * pixelSize();
}

//todo
bool Image::save(const std::string& path) const
{
	if(!data_) return 0;

    const std::size_t dot = path.find_last_of('.');
    std::string ext = (dot != std::string::npos) ? path.substr(dot + 1) : "";	   
	if(ext.empty()) ext = "plain";

	for(auto& c : ext) c = std::tolower(c);

	std::ofstream ofs(path);
	if(!ofs.is_open())
	{
		sendWarning("Image::save: failed to open file ", path);
		return 0;
	}

	return save(ofs, ext);
}

bool Image::load(const std::string& path)
{
	std::ifstream ifs(path);
	if(!ifs.is_open())
	{
		sendWarning("Image::load: failed to open file ", path);
		return 0;
	}

	return load(ifs);
}

bool Image::save(std::ostream& os, const std::string& type) const
{ 
	if(type == "bmp")
	{
		if(stbi_write_bmp_to_func(&write, &os, size_.x, size_.y, 4, data_.get()))
			return 1;
	}
	else if(type == "tga")
	{
		if(stbi_write_tga_to_func(&write, &os, size_.x, size_.y, 4, data_.get()))
			return 1;
	}
	else if(type == "png")
	{
		if(stbi_write_png_to_func(&write, &os, size_.x, size_.y, 4, data_.get(), 0))
			return 1;
	}
	else
	{
		sendWarning("Image::save(stream): unknown/unsupported type ", type);
		return 0;
	}

	sendWarning("Image::save: failed to load from stream with type ", type);
	return 0;
}

bool Image::load(std::istream& is)
{
	//TODO: format loading!
	data_.reset();

	stbi_io_Callbacks Callbacks;
    Callbacks.read = &read;
    Callbacks.skip = &skip;
    Callbacks.eof  = &eof;

    int width, height, channels;
    unsigned char* ptr = stbi_load_from_Callbacks(&Callbacks, &is, &width, 
			&height, &channels, STBI_rgb_alpha);

    if (ptr && width && height)
    {
        size_.x = width;
        size_.y = height;
		data_.reset(ptr);

		//no free_image since we just use the data

        return true;
    }

	sendWarning("Image::load: failed to load image from stream:\n\t", stbi_failure_reason());
	return 0;
}

}
