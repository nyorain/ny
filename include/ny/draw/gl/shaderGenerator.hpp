#pragma once

#include <ny/include.hpp>
#include <ny/draw/gl/context.hpp>
#include <string>

namespace ny
{

class ShaderGenerator
{
public:
	struct Version
	{
		GlContext::Api api;
		unsigned int major;
		unsigned int minor;
	};

protected:
	std::string code_;

	std::string versionString(const Version& version) const;
	std::string parseCode(const Version& version) const;

public:
	ShaderGenerator(const std::string& code = "") : code_(code) {};	
	
	void code(const std::string& text) { code_ = text; }
	std::string& code() { return code_; }
	const std::string& code() const { return code_; }

	virtual std::string generate(const Version& version) const = 0;
	virtual std::string generate() const;
};

class FragmentShaderGenerator : public ShaderGenerator
{
public:
	virtual std::string generate(const Version& version) const override;
};

class VertexShaderGenerator : public ShaderGenerator
{
public:
	virtual std::string generate(const Version& version) const override;
};

}
