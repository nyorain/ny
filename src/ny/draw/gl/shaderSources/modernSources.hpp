#pragma once

namespace ny
{

constexpr const char* defaultShaderVS =
    R"SRC(

    #version 300 es

    in vec2 position;
    uniform vec2 vViewSize;
    uniform mat3 vTransform;

    void main()
    {
        vec3 transpoint = vec3(position, 1.f) * vTransform;
        transpoint.x = (transpoint.x / vViewSize.x) * 2.f - 1.f;
        transpoint.y = ((vViewSize.y - transpoint.y) / vViewSize.y) * 2.f - 1.f;
        gl_Position = vec4(transpoint, 1.f);
    }

    )SRC";


constexpr const char* modernColorShaderFS =
    R"SRC(

    #version 300 es
	precision mediump float;

    out vec4 outColor;
    uniform vec4 fColor;

    void main()
    {
        outColor = fColor;
    }

    )SRC";

constexpr const char* modernTextureShaderFS =
	R"SRC(
	
	#version 300 es
	precision mediump float;

	out vec4 outColor;
	uniform vec2 vViewSize;
	uniform vec2 fTexturePosition;
	uniform vec2 fTextureSize;
	uniform sampler2D fTexture;

	void main()
	{
		vec2 fragCoords = gl_FragCoord.xy;
		fragCoords.y = vViewSize.y - fragCoords.y;
		vec2 texcoords = (fragCoords - fTexturePosition) / fTextureSize;
		outColor = texture(fTexture, texcoords);		
	} 
	
	)SRC";

}
