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

#ifndef CACHE_HPP
#define CACHE_HPP

#include <string>
#include <memory>
#include <vector>
#include <mutex>

class Cache
{
public:
    Cache(const std::string& folder);

    std::unique_ptr<std::ifstream> readLabels() const;
    std::unique_ptr<std::ifstream> read(const std::string& key);
    std::unique_ptr<std::ofstream> write(const std::string& key);
    bool has(const std::string& key) const;

private:
    mutable std::mutex mMutex;

    void flushIndex() const;

    std::string mFolder;
    std::vector<std::string> mFiles;
};

#endif // CACHE_HPP
