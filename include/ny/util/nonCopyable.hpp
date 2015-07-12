#pragma once

namespace ny
{


class nonCopyable
{
private:
	nonCopyable(const nonCopyable&) = delete;
	nonCopyable& operator =(const nonCopyable&) = delete;
protected:
	nonCopyable() = default;
	nonCopyable(nonCopyable&) = default;
};

}

