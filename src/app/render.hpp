//
//  render.hpp
//
//  Created by Antonin on 28/09/2022.
//

#pragma once
#ifndef render_h
#define render_h

#include "../utils/result.h"
#include "command.hpp"
#include "pipeline.hpp"
#include "vulkan/vulkan.h"
#include <vector>
#ifdef WIN32
#include <memory>
#endif

namespace app
{
    namespace graphics
    {
        /// @brief Contains a basic Renderer
        class Render
        {
        public:
            /// @brief Returns the static instance (singleton) of the object
            /// @return A static Render pointer
            static Render* getInstance();
            /// @brief Destructor
            ~Render();
            /// @brief Presents rendered images to a window.
            /// @return A Result type to know if the surface abstraction
            /// has been created.
            utils::VResult createSurface();
            /// @brief Returns the KHR surface as a pointer
            VkSurfaceKHR* getSurface();
            /// @brief Returns the framebuffers
            std::vector<VkFramebuffer> getFramebuffers();
            /// @brief Creates the image views for the Render, from the
            /// images from the SwapChain object.
            /// @return A VResult type to know if the function succeeded
            /// or not.
            utils::VResult createImageViews();
            /// @brief Creates the framebuffers for the objects to render
            /// @return A VResult type to know if the function succeeded
            /// or not.
            utils::VResult createFramebuffers();
            /// @brief Creates the graphics pipeline
            /// @return A VResult type to know if the function succeeded
            /// or not.
            utils::VResult createGraphicsPipeline();
            /// @brief Returns a reference to the current frame index, or swap chain index
            /// @return A reference to the current frame index
            uint32_t& getFrameIndex();
            /// @brief Updates the current frame, or swap chain, index
            /// Should not be called more than once per frame present
            void updateFrameIndex(uint64_t current_frame);
            /// @brief Returns the associated Graphics Command object if it exists
            std::shared_ptr<app::graphics::Command> getGraphicsCommand() const;
            /// @brief Returns the associated Transfert Command object if it exists
            std::shared_ptr<app::graphics::Command> getTransfertCommand() const;
            /// @brief Returns the associated Graphics pipeline object if it exists
            std::shared_ptr<app::graphics::Pipeline> getGraphicsPipeline() const;

        private:
            /// @brief Constructor
            Render();
            /// @brief Render should not be cloneable
            Render(Render& other) = delete;
            /// @brief Render should not be assignable
            void operator=(const Render& other) = delete;
            /// @brief The default surface to present rendered images
            VkSurfaceKHR m_surface = VK_NULL_HANDLE;
            /// @brief The internal instance (singleton) of
            /// the Render object
            static Render* m_instance;
            /// @brief Literal views to different images - describe how
            /// to access images and which part of the images to access
            std::vector<VkImageView> m_image_views;
            /// @brief Reference all of the VkImageView objects
            std::vector<VkFramebuffer> m_framebuffers;
            /// @brief The graphics pipeline, associated to a Renderer
            std::shared_ptr<app::graphics::Pipeline> m_graphics_pipeline = nullptr;
            /// @brief Graphics command pool
            std::shared_ptr<app::graphics::Command> m_graphics_command = nullptr;
            /// @brief Transfert command pool
            std::shared_ptr<app::graphics::Command> m_transfert_command = nullptr;
            /// @brief Creates the shader module:
            /// 1. Read the SPIR-V shaders,
            /// 2. Create the shader modules,
            /// 3. Create the shader stages.
            /// @return A Result type to know if the function succeeded
            /// or not.
            utils::VResult createShaderModule();
            /// @brief The current frame index, or swap chain index
            uint32_t m_frame_index = 0;
        };
    } // namespace graphics
} // namespace app

#endif // render_h
