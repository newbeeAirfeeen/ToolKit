/*****************************************************************************
 * Internal.hpp: Wraps an internal vlc type.
 *****************************************************************************
 * Copyright © 2014 VideoLAN
 *
 * Authors: Hugo Beauzée-Luyssen <hugo@beauzee.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef VLCPP_H
#define VLCPP_H

#include <cassert>
#include <memory>
#include <stdexcept>
#include <stdlib.h>
#include <vlc/libvlc.h>

namespace VLC {

    ///
    /// @brief The Internal class is a helper to wrap a raw libvlc type in a common
    ///         C++ type.
    ///
    template<typename T, typename Releaser = void (*)(T *)>
    class Internal {
    public:
        using InternalType = T;
        using InternalPtr = T *;
        using Pointer = std::shared_ptr<T>;

        ///
        /// \brief get returns the underlying libvlc type, or nullptr if this
        ///        is an empty instance
        ///
        InternalPtr get() const { return m_obj.get(); }

        ///
        /// \brief isValid returns true if this instance isn't wrapping a nullptr
        /// \return
        ///
        bool isValid() const { return (bool) m_obj; }

        ///
        /// \brief operator T * helper to convert to the underlying libvlc type
        ///
        operator T *() const { return m_obj.get(); }

    protected:
        Internal() = default;


        Internal(InternalPtr obj, Releaser releaser) {
            if (obj == nullptr)
                throw std::runtime_error("Wrapping a nullptr instance");
            m_obj.reset(obj, releaser);
        }

        Internal(Releaser releaser)
            : m_obj{nullptr, releaser} {
        }

    protected:
        Pointer m_obj;
    };

}// namespace VLC

#endif// VLCPP_H
