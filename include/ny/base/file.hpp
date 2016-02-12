#pragma once

#include <ny/include.hpp>

#include <string>
#include <iosfwd>

namespace ny
{

///Inline File abstract base class which can be used as abstract base for basically any 
///serialized type. Keeps track of the file path this file is saved to and 
///loaded from as well as the changed
///state of the file. When deriving from this class you have to implement the load from and
///save to stream functions and if you want to corRectly keep tracked of the changed state
///you have to call setChanged() everytime something at your object was changed.
///File is an inline, header-only class since it is pretty lightweight and classes like
///ny::image derive from it which are not linked against the ny-app library.
class File
{
protected:
    std::string filePath_ {};
	bool changed_ {0};

	void change(bool ch = 1){ changed_ = ch; }

public:
	File() = default;
	File(const std::string& path) : filePath_(path) {}
	virtual ~File() = default;

	bool changed() const { return changed_; }

    virtual bool save() const { if(filePath_.empty()) return 0; return save(filePath_); };
    virtual bool load() { if(filePath_.empty()) return 0; return load(filePath_); };

    virtual bool save(const std::string& path) const = 0;
    virtual bool load(const std::string& path) = 0;
};

}
