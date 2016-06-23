#pragma once

#include <ny/include.hpp>
#include <ny/draw/gl/context.hpp>
#include <string>

namespace ny
{

class ShaderGenerator
{
public:
	using Version = GlContext::Version;
public:
	ShaderGenerator() = default;
	ShaderGenerator(const std::string& code) : code_(code) {};

	void code(const std::string& text) { code_ = text; }
	std::string& code() { return code_; }
	const std::string& code() const { return code_; }

	virtual std::string generate(const Version& version) const = 0;
	virtual std::string generate() const;

protected:
	std::string code_;

	std::string versionString(const Version& version) const;
	std::string parseCode(const Version& version) const;

	virtual const char* inputAttribName() const = 0;
};

class FragmentShaderGenerator : public ShaderGenerator
{
public:
	using ShaderGenerator::ShaderGenerator;

	using ShaderGenerator::generate;
	virtual std::string generate(const Version& version) const override;

protected:
	virtual const char* inputAttribName() const override { return "varying"; }
};

class VertexShaderGenerator : public ShaderGenerator
{
public:
	using ShaderGenerator::ShaderGenerator;

	using ShaderGenerator::generate;
	virtual std::string generate(const Version& version) const override;

protected:
	virtual const char* inputAttribName() const override { return "attribute"; }
};

}
