#pragma once

namespace ny
{

namespace shaderSources
{

constexpr auto* defaultVS =
R"SRC(
%i Vec2 position;
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

constexpr auto* uvVS =
R"SRC(
%i Vec4 vertex;
%o Vec2 fUv;
uniform Vec2 vViewSize;
uniform mat3 vTransform;

void main()
{
	Vec2 position = vertex.xy;
	fUv = vertex.zw;

	Vec3 transpoint = Vec3(position, 1.f) * vTransform;
	transpoint.x = (transpoint.x / vViewSize.x) * 2.f - 1.f;
	transpoint.y = ((vViewSize.y - transpoint.y) / vViewSize.y) * 2.f - 1.f;
	gl_Position = Vec4(transpoint, 1.f);
}
)SRC";

constexpr auto* textureVS =
R"SRC(
%i Vec2 position;
%o Vec2 fUv;
uniform Vec2 vViewSize;
uniform mat3 vTransform;
uniform Vec2 vTexturePosition;
uniform Vec2 vTextureSize; 

void main()
{
	Vec3 transpoint = Vec3(position, 1.f) * vTransform;
	fUv = (transpoint.xy - vTexturePosition) / vTextureSize;
	transpoint.x = (transpoint.x / vViewSize.x) * 2.f - 1.f;
	transpoint.y = ((vViewSize.y - transpoint.y) / vViewSize.y) * 2.f - 1.f;
	gl_Position = Vec4(transpoint, 1.f);
}
)SRC";

constexpr auto* colorFS = 
R"SRC(
uniform Vec4 fColor;

void main()
{
    %fragColor = fColor; 
}
)SRC";

constexpr auto* textureRGBAFS = 
R"SRC(
%i Vec2 fUv;
uniform sampler2D fTexture;

void main()
{
    %fragColor = %texture2D(fTexture, fUv); 
}
)SRC";

constexpr auto* textureRGBFS = 
R"SRC(
%i Vec2 fUv;
uniform sampler2D fTexture;

void main()
{
    %fragColor = Vec4(%texture2D(fTexture, fUv).rgb, 1.); 
}
)SRC";

constexpr auto* textureRColorFS = 
R"SRC(
%i Vec2 fUv;
uniform Vec4 fColor;
uniform sampler2D fTexture;

void main()
{
	%fragColor = fColor;
	%fragColor.a *= %texture2D(fTexture, fUv).r;
}
)SRC";

}

}
