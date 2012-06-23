#ifndef Magnum_Trade_CameraData_h
#define Magnum_Trade_CameraData_h
/*
    Copyright © 2010, 2011, 2012 Vladimír Vondruš <mosra@centrum.cz>

    This file is part of Magnum.

    Magnum is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License version 3
    only, as published by the Free Software Foundation.

    Magnum is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Lesser General Public License version 3 for more details.
*/

/** @file
 * @brief Class Magnum::Trade::CameraData
 */

#include <string>

namespace Magnum { namespace Trade {

/**
@brief %Camera data
*/
class MAGNUM_EXPORT CameraData {
    CameraData(const CameraData& other) = delete;
    CameraData(CameraData&& other) = delete;
    CameraData& operator=(const CameraData& other) = delete;
    CameraData& operator=(CameraData&& other) = delete;

    public:
        /**
         * @brief Constructor
         * @param name      %Camera name
         */
        inline CameraData(const std::string& name): _name(name) {}

        /** @brief %Camera name */
        inline std::string name() const { return _name; }

    private:
        std::string _name;
};

}}

#endif
