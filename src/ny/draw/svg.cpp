#include <ny/draw/svg.hpp>
#include <ny/base/log.hpp>

#include <fstream>

#define PUGIXML_HEADER_ONLY
#include "pugixml/pugixml.hpp"

namespace ny
{

//TODO
bool SvgImage::save(const std::string& path) const
{
	pugi::xml_document doc;
	auto svgNode = doc.append_child("svg");

	for(auto& shape : shapes_)
	{
		switch(shape.pathBase().type())
		{
			case PathBase::Type::text:
			{
				auto& txt = shape.pathBase().text();

				auto node = svgNode.append_child("text");
				node.append_attribute("x") = std::to_string(txt.position().x).c_str();
				node.append_attribute("y") = std::to_string(txt.position().y).c_str();
				node.set_value(txt.string().c_str());

				break;
			}
			case PathBase::Type::Rectangle:
			{
				auto node = svgNode.append_child("Rect");
				break;
			}
			case PathBase::Type::path:
			{
				auto node = svgNode.append_child("path");
				break;
			}
			case PathBase::Type::circle:
			{
				auto node = svgNode.append_child("circle");
				break;
			}
		}	
	}

	if(!doc.save_file(path.c_str()))
	{
		sendWarning("SvgImage::save: failed to save svg file");
		return 0;
	}

	return 1;	
}

bool SvgImage::load(const std::string& path)
{
	pugi::xml_document doc;
	if(!doc.load_file(path.c_str()))
	{
		sendWarning("SvgImage::load: failed to load/parse svg file");
		return 0;
	}

	for(auto& node : doc)
	{
		if(std::string(node.name()) == "circle")
		{
		}
		else if(std::string(node.name()) == "Rect")
		{
		}
		else if(std::string(node.name()) == "ellipse")
		{
		}
		else if(std::string(node.name()) == "line")
		{
		}
		else if(std::string(node.name()) == "polyline")
		{
		}
		else if(std::string(node.name()) == "polygon")
		{
		}
		else if(std::string(node.name()) == "path")
		{
		}
		else
		{
			sendWarning("SvgImage::load: invalid node type ", node.name());
		}
	}

	return 1;
}

void SvgImage::draw(DrawContext& dc)
{
	for(auto& shape : shapes_)
	{
		dc.draw(shape);
	}
}

}
