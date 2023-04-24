//
//  engine.hpp
//
//  Created by Antonin on 24/09/2022.
//

#pragma once
#ifndef engine_hpp
#define engine_hpp

#include "../utils/result.h"
#include "device.hpp"
#include "pipeline.hpp"
#include "render.hpp"
#include "swapchain.hpp"
#include <cstdlib>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace app
{
    class Engine
    {
    public:
        /// @brief Internal state of the Engine object,
        /// to know if the unique object needs
        /// to be initialized or not
        enum struct State
        {
            /// @brief The starting state
            UNINITIALIZED,
            /// @brief The graphics pipeline and state
            /// machine have been initialized - it is
            /// now ready to use
            INITIALIZED,
            /// @brief An error happened during the initialization,
            /// the application should not run
            ERROR,
        };

    private:
        Engine();
        /// @brief Engine should not be cloneable
        Engine(Engine& other) = delete;
        /// @brief Engine should not be assignable
        void operator=(const Engine& other) = delete;
        /// @brief The single internal instance of the Engine object
        static Engine* m_instance;
        /// @brief Creates the Vulkan instance of the Engine
        utils::VResult createGraphicsInstance();
        /// @brief Choose and picks a physical device
        utils::VResult pickPhysicalDevice();
        /// @brief Creates the custom allocator
        utils::VResult createAllocator();
        /// @brief Creates the render device
        utils::VResult createRenderDevice();
        /// @brief Creates the swapchain
        utils::VResult createSwapChain();
        /// @brief Creates the descriptor pool
        utils::VResult createDescriptorPool();
        /// @brief Stores the internal state of the unique
        /// Engine object
        app::Engine::State m_state;
        /// @brief Descriptor pool
        VkDescriptorPool m_descriptor_pool = VK_NULL_HANDLE;

    public:
        /// @brief Get the singleton Engine object
        /// @return A static pointer to the Engine unique object
        static Engine* getInstance();
        /// @brief Initializes the Engine object
        /// only if the Engine object is in
        /// UNINITIALIZED state
        void initialize();
        /// @brief Returns the internal state of the unique
        /// Engine object
        app::Engine::State getState();
        ~Engine();

        /// @brief Custom allocator (VMA)
        VmaAllocator m_allocator = VK_NULL_HANDLE;
        /// @brief The engine instance
        VkInstance m_graphics_instance = VK_NULL_HANDLE;
        /// @brief The physical device
        app::graphics::Device m_graphics_device = app::graphics::Device();
        /// @brief The renderer of the engine
        std::unique_ptr<app::graphics::Render> m_render;
        /// @brief The swapchain of the engine
        std::unique_ptr<app::graphics::SwapChain> m_swapchain;
        /// @brief Returns a VkDescriptorPool object, associated to the current object
        VkDescriptorPool getDescriptorPool() const noexcept;
    };

} // namespace app

#endif // engine_hpp
