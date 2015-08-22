#pragma once

#include <string>

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

	bool hasChanged() const { return changed_; }

    virtual bool saveToFile(const std::string& path) const = 0;
    virtual bool loadFromFile(const std::string& path) = 0;

    virtual bool save() const { if(filePath_.empty()) return 0; return saveToFile(filePath_); };
    virtual bool reload() { if(filePath_.empty()) return 0; return loadFromFile(filePath_); };
};

}
