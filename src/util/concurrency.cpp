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

#include "concurrency.hpp"

#include <iostream>

TaskManager TaskManager::manager;

TaskManager::TaskManager() :
    mWork(std::make_unique<asio::io_service::work>(mIOService))
{
    for (unsigned int i = 0 ; i < std::thread::hardware_concurrency() ; ++i)
        mThreadPool.emplace_back([this]() {mIOService.run();});
}

TaskManager::~TaskManager()
{
    mWork.reset();
    mIOService.stop();
    for (auto& thread : mThreadPool)
        thread.join();
}

void TaskManager::launch(const std::function<void()>& f)
{
    mIOService.post(f);
}

