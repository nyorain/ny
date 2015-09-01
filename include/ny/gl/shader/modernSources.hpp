#pragma once

namespace ny
{

constexpr const char* defaultShaderVS =
    R"SOURCE(

    #version 130
    in vec2 pos;
    //uniform mat3 transform;
    void main()
    {
        gl_Position = vec4(vec3(pos, 1.), 1.);
    }

    )SOURCE";

constexpr const char* defaultShaderFS =
    R"SOURCE(

    #version 130
    out vec4 outColor;
    uniform vec4 inColor;
    void main()
    {
        outColor = inColor;
    }

    )SOURCE";

}
