#pragma once

#include <ny/include.hpp>
#include <ny/util/nonCopyable.hpp>

#include <functional>
#include <vector>
#include <algorithm>
#include <utility>

namespace ny
{

class connection;
template < class > class callback;

//class callbackBase//////////////////////////////////////
class callbackBase
{
public:
    virtual void remove(const connection& con) = 0;
};



//connection//////////////////////////////////////////
class connection : public nonCopyable
{
protected:
    template<class T> friend class callback;

    callbackBase& callback_;
    bool connected_;

    void wasRemoved(){ connected_ = 0; }

public:
    connection(callbackBase& call) : callback_(call) { connected_ = 1; }
    virtual ~connection(){}

    connection(const connection&& mover) noexcept : callback_(mover.callback_), connected_(mover.connected_) {} //for callback
    connection& operator=(const connection&& mover) noexcept { callback_ = mover.callback_; connected_ = mover.connected_; return *this; } //for callback

    void destroy(){ callback_.remove(*this); } //will delete this object implicitly
    bool isConnected() const { return connected_; };
};



//callback////////////////////////////////////////////
template <class  Ret, class ... Args> class callback<Ret(Args...)> : public callbackBase
{
protected:
    std::vector< std::pair<connection, std::function<Ret(Args ...)>> > callbacks_;

public:
    ~callback()
    {
        for(unsigned int i(0); i < callbacks_.size(); i++)
        {
            callbacks_[i].first.wasRemoved();
        }
    }

    //adds an callback by += operator
    callback<Ret(Args...)>& operator+=(const std::function<Ret(Args...)>& func)
    {
        add(func);
        return *this;
    };

    //clears all callbacks and sets one new callback
    callback<Ret(Args...)>& operator=(const std::function<Ret(Args...)>& func)
    {
        clear();
        add(func);
        return *this;
    };

    //adds new callback and return connection for removing of the callback
    connection& add(const std::function<Ret(Args ...)>& func)
    {
        connection conn(*this);

        callbacks_.push_back(std::make_pair(std::move(conn), func));

        return callbacks_.back().first;
    };

    //removes a callback identified by its connection. Functions (std::function) can't be compared => we need connections
    void remove(const connection& con)
    {
        for(unsigned int i(0); i < callbacks_.size(); i++)
        {
            if(&(callbacks_[i].first) == &con)
            {
                callbacks_[i].first.wasRemoved();
                callbacks_.erase(callbacks_.begin() + i);
                return;
            }
        }
    };

    //calls the callback
    std::vector<Ret> call(Args ... a)
    {
        std::vector<Ret> ret;
        for(unsigned int i(0); i < callbacks_.size(); i++)
        {
            ret.push_back(callbacks_[i].second(a ...));
        }
        return ret;
    };

    //clears all registered callbacks and connections
    void clear()
    {
        for(unsigned int i(0); i < callbacks_.size(); i++)
        {
            callbacks_[i].first.wasRemoved();
        }

        callbacks_.clear();
    }

    std::vector<Ret> operator() (Args... a)
    {
        return call(a ...);
    }
};


//callback specialization for void because callback cant return a <void>-vector/////////////////////////////////
template< class ... Args> class callback<void(Args...)> : public callbackBase
{
protected:
    std::vector< std::pair<connection, std::function<void(Args ...)>> > callbacks_;

public:
    ~callback()
    {
        for(unsigned int i(0); i < callbacks_.size(); i++)
        {
            callbacks_[i].first.wasRemoved();
        }
    }


    callback<void(Args...)>& operator+=(const std::function<void(Args...)>& func)
    {
        add(func);
        return *this;
    };

    callback<void(Args...)>& operator=(const std::function<void(Args...)>& func)
    {
        clear();
        add(func);
        return *this;
    };

    connection& add(const std::function<void(Args...)>& func)
    {
        connection conn(*this);

        callbacks_.push_back(std::make_pair(std::move(conn), func));

        return callbacks_.back().first;
    };

    void remove(const connection& con)
    {
        for(unsigned int i(0); i < callbacks_.size(); i++)
        {
            if(&(callbacks_[i].first) == &con)
            {
                callbacks_[i].first.wasRemoved();
                callbacks_.erase(callbacks_.begin() + i);
                return;
            }
        }
    };

    void call(Args ... a)
    {
        for(unsigned int i(0); i < callbacks_.size(); i++)
        {
            callbacks_[i].second(a ...);
        }
    };

    void clear()
    {
        for(unsigned int i(0); i < callbacks_.size(); i++)
        {
            callbacks_[i].first.wasRemoved();
        }

        callbacks_.clear();
    }

    void operator() (Args... a)
    {
        call(a ...);
    }
};

}
