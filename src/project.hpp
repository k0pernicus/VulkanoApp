//
//  project.hpp
//  FrameTech
//
//  Created by Antonin on 23/09/2022.
//

#pragma once
#ifndef app_project_h
#define app_project_h

#include "utils/version.h"
#include <cstdlib>
#include <optional>

/// @brief Limits of 120 FPS
constexpr uint8_t FPS_LIMIT_120 = 120;
/// @brief Limits of 60 FPS
constexpr uint8_t FPS_LIMIT_60 = 60;
/// @brief Limits of 30 FPS
constexpr uint8_t FPS_LIMIT_30 = 30;
/// @brief No FPS limit
constexpr std::optional<uint8_t> NO_FPS_LIMIT = std::nullopt;

namespace Project
{

    /// @brief The name of the application
    constexpr const char* APPLICATION_NAME = "Application";
#ifdef UNSET_FPS_LIMIT
    /// @brief Pause / limit the Application to XXX FPS
    constexpr std::optional<uint8_t> APPLICATION_FPS_LIMIT = NO_FPS_LIMIT;
#else
    /// @brief Pause / limit the Application to XXX FPS
    constexpr std::optional<uint8_t> APPLICATION_FPS_LIMIT = FPS_LIMIT_60;
#endif
    /// @brief Major version number of the application
    constexpr uint8_t const APPLICATION_VERSION_MAJOR_NUMBER = 0;
    /// @brief Minor version number of the application
    constexpr uint8_t const APPLICATION_VERSION_MINOR_NUMBER = 1;
    /// @brief Bug fix version number of the application
    constexpr uint8_t const APPLICATION_VERSION_BUGFIX_NUMBER = 0;

    /// @brief The Version object to represent / formalize the version of the application
    const utils::Version APPLICATION_VERSION = utils::Version(APPLICATION_VERSION_MAJOR_NUMBER,
                                                              APPLICATION_VERSION_MINOR_NUMBER,
                                                              APPLICATION_VERSION_BUGFIX_NUMBER);

    /// @brief Minimum major version number of the Vulkan API
    constexpr uint8_t const VULKAN_MIN_VERSION_MAJOR = 1;
    /// @brief Minimum minor version number of the Vulkan API
    constexpr uint8_t const VULKAN_MIN_VERSION_MINOR = 3;
    /// @brief Minimum bug fix version number of the Vulkan API
    constexpr uint8_t const VULKAN_MIN_VERSION_BUGFIX = 211;

} // namespace Project

#endif // app_project_h
