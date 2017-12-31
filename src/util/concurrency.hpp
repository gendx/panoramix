/*
    Panoramix - 3D view of your surroundings.
    Copyright (C) 2017  Guillaume Endignoux

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see http://www.gnu.org/licenses/gpl-3.0.txt
*/

#ifndef CONCURRENCY_HPP
#define CONCURRENCY_HPP

#include <asio/io_service.hpp>
#include <thread>
#include <mutex>
#include <condition_variable>

class TaskManager
{
public:
    static TaskManager manager;

    void launch(const std::function<void()>& f);

private:
    TaskManager();
    ~TaskManager();

    asio::io_service mIOService;
    std::unique_ptr<asio::io_service::work> mWork;
    std::vector<std::thread> mThreadPool;
};

template <typename T>
class LockGuarded
{
    std::unique_lock<std::mutex> acquire() const
        {return std::unique_lock<std::mutex>(mMutex);}

public:
    template <typename... Args>
    LockGuarded(Args&&... a) :
        t(std::forward<decltype(a)>(a)...) {}

    void notify_one()
        {mCondVar.notify_one();}
    void wait(const std::function<bool(const T&)>& f)
    {
        auto lock = acquire();
        if (!f(t))
            mCondVar.wait(lock, [this, f] {return f(t);});
    }

    template <typename F>
    void apply(F& f)
        {auto lock = acquire(); f(t);}

    T get() const
        {auto lock = acquire(); return t;}
    void set(const T& x)
        {auto lock = acquire(); t = x;}
    void set(T&& x)
        {auto lock = acquire(); t = std::move(x);}
    void swap(T& _t)
        {auto lock = acquire(); std::swap(t, _t);}

private:
    T t;
    mutable std::mutex mMutex;
    std::condition_variable mCondVar;
};

template <typename T>
class LockGuardedShared
{
public:
    template <typename... Args>
    LockGuardedShared(Args&&... a) :
        mGuarded(std::make_shared<T>(std::forward<decltype(a)>(a)...)) {}

    std::shared_ptr<T> get() const
        {return mGuarded.get();}
    void set(std::shared_ptr<T>&& x)
        {mGuarded.set(std::move(x));}
    void swap(std::shared_ptr<T>& x)
        {mGuarded.swap(x);}

private:
    LockGuarded<std::shared_ptr<T>> mGuarded;
};

#endif

