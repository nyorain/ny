#pragma once

namespace ny
{

constexpr const char* defaultShaderVS =
    R"SRC(

    #version 300 es

    in Vec2 position;
    uniform Vec2 vViewSize;
    uniform mat3 vTransform;

    void main()
    {
        Vec3 transpoint = Vec3(position, 1.f) * vTransform;
        transpoint.x = (transpoint.x / vViewSize.x) * 2.f - 1.f;
        transpoint.y = ((vViewSize.y - transpoint.y) / vViewSize.y) * 2.f - 1.f;
        gl_Position = Vec4(transpoint, 1.f);
    }

    )SRC";

constexpr const char* uvShaderVS =
    R"SRC(

    #version 300 es

    in Vec4 vertex; //2 position, 2 uv

	out Vec2 uv;
    uniform Vec2 vViewSize;
    uniform mat3 vTransform;

    void main()
    {
        Vec3 transpoint = Vec3(vertex.xy, 1.f) * vTransform;
        transpoint.x = (transpoint.x / vViewSize.x) * 2.f - 1.f;
        transpoint.y = ((vViewSize.y - transpoint.y) / vViewSize.y) * 2.f - 1.f;
        gl_Position = Vec4(transpoint, 1.f);
		uv = vertex.zw;
    }

    )SRC";


constexpr const char* modernColorShaderFS =
    R"SRC(

    #version 300 es
	precision mediump float;

    out Vec4 outColor;
    uniform Vec4 fColor;

    void main()
    {
        outColor = fColor; 
    }

    )SRC";

constexpr const char* modernTextureShaderRGBAFS =
	R"SRC(
	
	#version 300 es
	precision mediump float;

	out Vec4 outColor;
	uniform Vec2 vViewSize;
	uniform Vec2 fTexturePosition;
	uniform Vec2 fTextureSize;
	uniform sampler2D fTexture;

	void main()
	{
		Vec2 fragCoords = gl_FragCoord.xy;
		fragCoords.y = vViewSize.y - fragCoords.y;
		Vec2 texcoords = (fragCoords - fTexturePosition) / fTextureSize;
		outColor = texture(fTexture, texcoords);		
	} 
	
	)SRC";

constexpr const char* modernTextureShaderRGBFS =
	R"SRC(
	
	#version 300 es
	precision mediump float;

	out Vec4 outColor;
	uniform Vec2 vViewSize;
	uniform Vec2 fTexturePosition;
	uniform Vec2 fTextureSize;
	uniform sampler2D fTexture;

	void main()
	{
		Vec2 fragCoords = gl_FragCoord.xy;
		fragCoords.y = vViewSize.y - fragCoords.y;
		Vec2 texcoords = (fragCoords - fTexturePosition) / fTextureSize;
		outColor = Vec4(texture(fTexture, texcoords).rgb, 1.);		
	} 
	
	)SRC";

constexpr const char* modernColorShaderTextureAFS =
	R"SRC(
	
	#version 300 es
	precision mediump float;

	in Vec2 uv;
	out Vec4 outColor;

	uniform Vec4 fColor;
	uniform sampler2D fTexture;

	void main()
	{
		outColor = fColor;
		outColor.a *= texture(fTexture, uv).r;	
	} 
	
	)SRC";

namespace gl
{
constexpr const char* defaultShaderVS =
    R"SRC(

    #version 330 core

    layout (location = 0) in Vec2 position;
    uniform Vec2 vViewSize;
    uniform mat3 vTransform;

    void main()
    {
        //Vec3 transpoint = Vec3(position, 1.f) * vTransform;
        //transpoint.x = (transpoint.x / vViewSize.x) * 2.f - 1.f;
        //transpoint.y = ((vViewSize.y - transpoint.y) / vViewSize.y) * 2.f - 1.f;
        gl_Position = Vec4(position, 1.f, 1.f);
    }

    )SRC";


constexpr const char* modernColorShaderFS =
    R"SRC(

    #version 330 core

    out Vec4 outColor;
    uniform Vec4 fColor;

    void main()
    {
        outColor = fColor;
    }

    )SRC";
}

}
