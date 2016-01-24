#pragma once

//macro for current context validation
#if defined(__GNUC__) || defined(__clang__)
 #define FUNC_NAME __PRETTY_FUNCTION__
#else
 #define FUNC_NAME __func__
#endif //FUNCNAME

//macro for assuring a valid context (warn and return if there is none)
#define VALIDATE_CTX(x) if(!GlContext::current())\
	{ nytl::sendWarning(FUNC_NAME, ": no current opengl context."); return x; }

#define RES_VALIDATE_CTX(x) if(!this->validContext())\
	{ nytl::sendWarning(FUNC_NAME, ": no or invalid current opengl context."); return x; }
