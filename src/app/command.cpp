//
//  command.cpp
//
//  Created by Antonin on 26/10/2022.
//

#include "command.hpp"
#include "engine.hpp"

#ifdef IMGUI
#include "backends/imgui_impl_vulkan.h"
#include "imgui.h"
#endif

app::graphics::Command::Command(){};

app::graphics::Command::~Command()
{
    if (nullptr != m_pool)
    {
        if (nullptr != app::Engine::getInstance()->m_graphics_device.getLogicalDevice())
            vkDestroyCommandPool(app::Engine::getInstance()->m_graphics_device.getLogicalDevice(), m_pool, nullptr);
        m_pool = nullptr;
    }
    m_buffer = nullptr;
};

utils::VResult app::graphics::Command::createPool(const uint32_t family_index)
{
    VkCommandPoolCreateInfo pool_create_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = family_index,
    };
    const auto create_result = vkCreateCommandPool(
        app::Engine::getInstance()->m_graphics_device.getLogicalDevice(),
        &pool_create_info,
        nullptr,
        &m_pool);
    if (VK_SUCCESS == create_result)
        return utils::VResult::Ok();
    return utils::VResult::Error((char*)"> Error creating the command pool in the command buffer object");
}

utils::VResult app::graphics::Command::createBuffer()
{
    if (nullptr == m_pool)
        return utils::VResult::Error((char*)"> Error creating the buffer: no memory pool");
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = (m_pool);
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    const auto create_result = vkAllocateCommandBuffers(
        app::Engine::getInstance()->m_graphics_device.getLogicalDevice(),
        &alloc_info,
        &m_buffer);
    if (create_result == VK_SUCCESS)
        return utils::VResult::Ok();
    return utils::VResult::Error((char*)"> Error creating the buffer in the command buffer object");
}

utils::VResult app::graphics::Command::record()
{
    const auto swapchain_index = app::Engine::getInstance()->m_render->getFrameIndex();
    VkCommandBufferBeginInfo begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };

    // Reset the command buffer before any operation on the current buffer
    vkResetCommandBuffer(m_buffer, 0);

    if (const auto begin_result_code = vkBeginCommandBuffer(m_buffer, &begin_info); begin_result_code != VK_SUCCESS)
    {
        return utils::VResult::Error((char*)"< Error creating the command buffer");
    }

    const std::vector<VkFramebuffer> framebuffers = app::Engine::getInstance()->m_render->getFramebuffers();
    if (swapchain_index >= framebuffers.size())
    {
        return utils::VResult::Error((char*)"< The swapchain_index parameter is incorrect: not enough framebuffers");
    }

    VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    VkRenderPassBeginInfo render_pass_begin_info{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = app::Engine::getInstance()->m_render->getGraphicsPipeline()->getRenderPass(),
        .framebuffer = framebuffers[swapchain_index],
        .renderArea = {
            .offset = {0, 0},
            .extent = app::Engine::getInstance()->m_swapchain->getExtent(),
        },
        .clearValueCount = 1,
        .pClearValues = &clear_color,
    };

    vkCmdBeginRenderPass(m_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(
        m_buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        app::Engine::getInstance()->m_render->getGraphicsPipeline()->getPipeline());

    // Setup the viewport and scissor as dynamic
    // TODO: fix this in the fixed function
    VkViewport viewport{
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(app::Engine::getInstance()->m_swapchain->getExtent().width),
        .height = static_cast<float>(app::Engine::getInstance()->m_swapchain->getExtent().height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(m_buffer, 0, 1, &viewport);

    VkRect2D scissor{
        .offset = {0, 0},
        .extent = app::Engine::getInstance()->m_swapchain->getExtent(),
    };
    vkCmdSetScissor(m_buffer, 0, 1, &scissor);

    // Bind the vertex buffer
    std::vector<VkBuffer> vertex_buffers = {app::Engine::getInstance()->m_render->getGraphicsPipeline()->getVertexBuffer()};
    // // TODO: check to include this information getting the vertex buffer
    std::vector<VkDeviceSize> memory_offsets(vertex_buffers.size());
    const VkBuffer& index_buffer = app::Engine::getInstance()->m_render->getGraphicsPipeline()->getIndexBuffer();
    for (size_t i = 0; i < vertex_buffers.size(); ++i)
        memory_offsets[i] = i;
    vkCmdBindVertexBuffers(m_buffer, 0, (uint32_t)vertex_buffers.size(), vertex_buffers.data(), memory_offsets.data());
    vkCmdBindIndexBuffer(m_buffer, index_buffer, 0, VK_INDEX_TYPE_UINT32);

    // TODO: fix                                                                                                
    uint32_t indices_size = 0;

    vkCmdDrawIndexed(m_buffer, indices_size, 1, 0, 0, 0);

#ifdef IMGUI
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(draw_data, m_buffer);
#endif

    vkCmdEndRenderPass(m_buffer);
    if (const auto end_command_buffer_result_code = vkEndCommandBuffer(m_buffer); end_command_buffer_result_code != VK_SUCCESS)
    {
        return utils::VResult::Error((char*)"< Error recording the command buffer");
    }

    return utils::VResult::Ok();
}

VkCommandBuffer* app::graphics::Command::getBuffer()
{
    return &m_buffer;
};

VkCommandPool* app::graphics::Command::getPool()
{
    return &m_pool;
};
