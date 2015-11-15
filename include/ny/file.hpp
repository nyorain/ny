#pragma once

#include <string>
#include <ostream>
#include <istream>

namespace ny
{

class file
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
