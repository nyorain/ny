#pragma once

#include <ny/app/include.hpp>
#include <nytl/nonCopyable.hpp>

#include <string>
#include <ostream>
#include <istream>

namespace ny
{

///Inline File abstract base class which can be used as abstract base for basically any serialized type.
///Keeps track of the file path this file is saved to and loaded from as well as the changed
///state of the file. When deriving from this class you have to implement the load from and
///save to stream functions and if you want to correctly keep tracked of the changed state
///you have to call setChanged() everytime something at your object was changed.
///File is an inline, header-only class since it is pretty lightweight and classes like
///ny::image derive from it which are not linked against the ny-app library.
class File : public nonCopyable
{
protected:
    std::string filePath_ {};
	bool changed_ {0};

	void setChanged(bool ch){ changed_ = ch; }

public:
	file() = default;
	file(const std::string& path);
	virtual ~file() = default;

	bool hasChanged() const { return changed_; }

    virtual bool save() const { if(filePath_.empty()) return 0; return save(filePath_); };
    virtual bool load() { if(filePath_.empty()) return 0; return load(filePath_); };

    virtual bool save(const std::string& path) const;
    virtual bool load(const std::string& path);

    virtual bool save(std::ostream& os) const = 0;
    virtual bool load(std::istream& is) = 0;
};

}
