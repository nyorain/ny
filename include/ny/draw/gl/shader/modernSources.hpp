#pragma once

namespace ny
{

constexpr const char* defaultShaderVS =
    R"SRC(

    #version 130

    in vec2 pos;
    uniform vec2 viewSize;
    uniform mat3 transform;

    void main()
    {
        vec3 transpoint = vec3(pos, 1.f) * transform;
        transpoint.x = (transpoint.x / viewSize.x) * 2 - 1; //normalize it to gl coords
        transpoint.y = ((viewSize.y - transpoint.y) / viewSize.y) * 2 - 1; //normalize and invert y
        gl_Position = vec4(transpoint, 1.f);
    }

    )SRC";

constexpr const char* defaultShaderFS =
    R"SRC(

    #version 130
    out vec4 outColor;
    uniform vec4 inColor;
    void main()
    {
        outColor = inColor;
    }

    )SRC";

}
