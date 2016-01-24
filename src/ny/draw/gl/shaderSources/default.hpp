#pragma once

namespace ny
{

namespace shaderSources
{

constexpr auto* defaultVS =
R"SRC(
%i vec2 position;
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

constexpr auto* uvVS =
R"SRC(
%i vec2 position;
%i vec2 vUv;
%o vec2 fUv;
uniform vec2 vViewSize;
uniform mat3 vTransform;

void main()
{
	vec3 transpoint = vec3(position, 1.f) * vTransform;
	transpoint.x = (transpoint.x / vViewSize.x) * 2.f - 1.f;
	transpoint.y = ((vViewSize.y - transpoint.y) / vViewSize.y) * 2.f - 1.f;
	gl_Position = vec4(transpoint, 1.f);

	fUv = vUv;
}
)SRC";

constexpr auto* textureVS =
R"SRC(
%i vec2 position;
%o vec2 fUv;
uniform vec2 vViewSize;
uniform mat3 vTransform;
uniform vec2 vTexturePosition;
uniform vec2 vTextureSize; 

void main()
{
	vec3 transpoint = vec3(position, 1.f) * vTransform;
	fUv = (transpoint.xy - fTexturePosition) / fTextureSize;
	transpoint.x = (transpoint.x / vViewSize.x) * 2.f - 1.f;
	transpoint.y = ((vViewSize.y - transpoint.y) / vViewSize.y) * 2.f - 1.f;
	gl_Position = vec4(transpoint, 1.f);

}
)SRC";

constexpr auto* colorFS = 
R"SRC(
uniform vec4 fColor;

void main()
{
    %fragColor = fColor; 
}
)SRC";

constexpr auto* textureRGBAFS = 
R"SRC(
%i vec2 fUv;
uniform sampler2D fTexture;

void main()
{
    %fragColor = %texture2D(fTexture, fUv); 
}
)SRC";

constexpr auto* textureRGBFS = 
R"SRC(
%i vec2 fUv;
uniform sampler2D fTexture;

void main()
{
    %fragColor = vec4(%texture2D(fTexture, fUv).rgb, 1.); 
}
)SRC";

constexpr auto* textureRColorFS = 
R"SRC(
%i vec2 fUv;
uniform vec4 fColor;
uniform sampler2D fTexture;

void main()
{
    %fragColor = fColor;
	%fragColor.a *= %texture2D(fTexture, fUv).r;
}
)SRC";

}

}
