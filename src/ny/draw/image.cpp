#include <ny/draw/image.hpp>
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
		case Format::rgba8888: case Format::argb8888: case Format::bgra8888: return 4;
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
		return stream->eof();
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
	data_ = std::make_unique<std::uint8_t[]>(dataSize());
}

Image::Image(const std::string& path) : File(path)
{
	//problematic since virtual...
	load(path);
}

Image::Image(const std::uint8_t* data, const Vec2ui& size, Format format)
	: File(), size_(size), format_(format)
{
	data_ = std::make_unique<std::uint8_t[]>(dataSize());
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

	data_ = std::make_unique<std::uint8_t[]>(dataSize());
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
	auto ret = std::make_unique<std::uint8_t[]>(dataSize());
	std::memcpy(ret.get(), data_.get(), dataSize());
	return ret;
}

std::size_t Image::dataSize() const
{
	return size_.x * size_.y * pixelSize();
}

void Image::format(Format newformat)
{
	auto newsize = formatSize(newformat);
	auto newdata = std::make_unique<std::uint8_t[]>(size_.x * size_.y * newsize);

	for(auto y(0u); y < size_.y; ++y)
	{
		for(auto x(0u); x < size_.x; ++x)
		{
			auto i = y * size_.x + x;
			auto color = at({x, y});

			switch(newformat)
			{
				case Format::bgra8888:
				{

					newdata[i * 4 + 0] = color.b;
					newdata[i * 4 + 1] = color.g;
					newdata[i * 4 + 2] = color.r;
					newdata[i * 4 + 3] = color.a;
				}
			}
		}
	}

	data_ = std::move(newdata);
	format_ =  newformat;
}

Color Image::at(const Vec2ui& pos) const
{
	auto i = pos.y * size_.x + pos.x;
	auto d = &data_[i * pixelSize()];

	std::uint8_t d1, d2, d3, d4;

	if(pixelSize() > 0) d1 = *(d + 0);
	if(pixelSize() > 1) d2 = *(d + 1);
	if(pixelSize() > 2) d3 = *(d + 2);
	if(pixelSize() > 3) d4 = *(d + 3);

	switch(format())
	{
		case Format::bgra8888: return Color(d3, d2, d1, d4);
		case Format::argb8888: return Color(d4, d1, d2, d3);
		case Format::rgba8888: return Color(d1, d2, d3, d4);
		case Format::rgb888: return Color(d1, d2, d3);
		case Format::a8: return Color(0, 0, 0, d1);
	}
}

//todo
bool Image::save(const std::string& path) const
{
	if(!data_) return 0;

    const std::size_t dot = path.find_last_of('.');
    std::string ext = (dot != std::string::npos) ? path.substr(dot + 1) : "";
	if(ext.empty()) ext = "png";

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
	//
	std::ifstream ifs(path, std::ios_base::binary);
	if(!ifs.is_open())
	{
		sendWarning("Image::load: failed to open file ", path);
		return false;
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

	stbi_io_callbacks callbacks;
    callbacks.read = &read;
    callbacks.skip = &skip;
    callbacks.eof  = &eof;

    int width, height, channels;
    unsigned char* ptr = stbi_load_from_callbacks(&callbacks, &is, &width,
			&height, &channels, STBI_rgb_alpha);

    if (ptr && width && height)
    {
		std::size_t dataSize = width * height * STBI_rgb_alpha;

        size_.x = width;
        size_.y = height;

		//copy data, it cannot be used directly (without custom deleter)
		data_ = std::make_unique<std::uint8_t[]>(dataSize);
		std::memcpy(data_.get(), ptr, dataSize);

		stbi_image_free(ptr);

		format_ = Format::rgba8888;

        return true;
    }

	sendWarning("Image::load: failed to load image from stream:\n\t", stbi_failure_reason());
	return false;
}

}
