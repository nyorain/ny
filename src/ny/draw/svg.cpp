#include <ny/draw/svg.hpp>
#include <nytl/log.hpp>

#include <fstream>

#define PUGIXML_HEADER_ONLY
#include "pugixml/pugixml.hpp"

namespace ny
{

bool SvgImage::save(const std::string& path) const
{
	pugi::xml_document doc;
	for(auto& shape : shapes_)
	{
		switch(shape.pathBase().type())
		{
			case PathBase::Type::text:
			{
				break;
			}
			case PathBase::Type::rectangle:
			{
				break;
			}
			case PathBase::Type::path:
			{
				break;
			}
			case PathBase::Type::circle:
			{
				break;
			}
		}	
	}

	if(!doc.save_file(path.c_str()))
	{
		nytl::sendWarning("SvgImage::save: failed to save svg file");
		return 0;
	}

	return 1;	
}

bool SvgImage::load(const std::string& path)
{
	pugi::xml_document doc;
	if(!doc.load_file(path.c_str()))
	{
		nytl::sendWarning("SvgImage::load: failed to load/parse svg file");
		return 0;
	}

	for(auto node : doc)
	{
		if(std::string(node.name()) == "circle")
		{
		}
		else if(std::string(node.name()) == "rect")
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
		else if(std::string(node.name()) == "path")
		{
		}
		else
		{
			nytl::sendWarning("SvgImage::load: invalid node type ", node.name());
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
