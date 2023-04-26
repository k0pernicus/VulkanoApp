//
//  pipeline.cpp
//
//  Created by Antonin on 23/09/2022.
//

#include "pipeline.hpp"
#include "../utils/debug_tools.h"
#include "../utils/result.h"
#include "engine.hpp"
#include "memory.hpp"
#include "shaders.h"
#include <assert.h>
#include <filesystem>
#include <fstream>
#include <vector>

app::graphics::Pipeline::Pipeline()
{
}

app::graphics::Pipeline::~Pipeline()
{
    const auto graphics_device = app::Engine::getInstance()->m_graphics_device.getLogicalDevice();
    const auto resource_allocator = app::Engine::getInstance()->m_allocator;
    if (m_shader_modules.size() > 0)
    {
        Log("< Destroying the shader modules...");
        for (const auto shader_module : m_shader_modules)
            vkDestroyShaderModule(
                graphics_device,
                shader_module,
                nullptr);
    }
    if (VK_NULL_HANDLE != m_render_pass)
    {
        Log("< Destroying the render pass...");
        vkDestroyRenderPass(graphics_device, m_render_pass, nullptr);
        m_render_pass = VK_NULL_HANDLE;
    }
    if (VK_NULL_HANDLE != m_layout)
    {
        Log("< Destroying the pipeline layout...");
        vkDestroyPipelineLayout(graphics_device, m_layout, nullptr);
        m_layout = VK_NULL_HANDLE;
    }
    if (VK_NULL_HANDLE != m_vertex_buffer)
    {
        Log("< Destroying the vertex buffer...");
        vmaDestroyBuffer(resource_allocator, m_vertex_buffer, m_vertex_buffer_allocation);
        m_vertex_buffer = VK_NULL_HANDLE;
        m_vertex_buffer_allocation = VK_NULL_HANDLE;
    }
    if (VK_NULL_HANDLE != m_index_buffer)
    {
        Log("< Destroying the index buffer...");
        vmaDestroyBuffer(resource_allocator, m_index_buffer, m_index_buffer_allocation);
        m_index_buffer = VK_NULL_HANDLE;
        m_index_buffer_allocation = VK_NULL_HANDLE;
    }
    if (VK_NULL_HANDLE != m_pipeline)
    {
        Log("< Destroying the pipeline object...");
        vkDestroyPipeline(graphics_device, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }
    if (nullptr != m_sync_image_ready)
    {
        Log("< Destroying the image ready signal semaphore...");
        vkDestroySemaphore(graphics_device, *m_sync_image_ready, nullptr);
        m_sync_image_ready = nullptr;
    }
    if (nullptr != m_sync_present_done)
    {
        Log("< Destroying the present done signal semaphore...");
        vkDestroySemaphore(graphics_device, *m_sync_present_done, nullptr);
        m_sync_present_done = nullptr;
    }
    if (nullptr != m_sync_cpu_gpu)
    {
        Log("< Destroying the fence...");
        vkDestroyFence(graphics_device, *m_sync_cpu_gpu, nullptr);
        m_sync_cpu_gpu = nullptr;
    }
}

std::optional<uint64_t> app::graphics::Pipeline::fileSize(const char* filepath)
{
    try
    {
        const auto file_size = std::filesystem::file_size(filepath);
        return file_size;
    }
    catch (const std::exception& e)
    {
        LogE("< Error getting the file size at path '%s'", filepath);
        return std::nullopt;
    }
}

std::optional<uint64_t> app::graphics::Pipeline::readFile(const char* filepath,
                                                          char** buffer,
                                                          uint64_t buffer_length)
{
    std::ifstream file(filepath, std::ifstream::binary | std::ifstream::ate);
    assert(file);
    if (file)
    {
        std::optional<uint64_t> opt_length = fileSize(filepath);
        if (opt_length == std::nullopt)
        {
            return std::nullopt;
        }
        uint64_t length = opt_length.value();

        // do not read more than enough
        length = buffer_length < length ? (int)buffer_length : length;

        // read data as a block, close, and return
        // the length of the file
        file.seekg(0);
        file.read(*buffer, length);
        file.close();
        return length;
    }
    return std::nullopt;
}

utils::Result<std::vector<app::graphics::Shader::Module>> app::graphics::Pipeline::createGraphicsApplication(const char* vertex_shader_filepath,
                                                                                                             const char* fragment_shader_filepath)
{
    // Get the length
    const auto vs_file_size_opt = fileSize(vertex_shader_filepath);
    assert(vs_file_size_opt != std::nullopt);
    const auto fs_file_size_opt = fileSize(fragment_shader_filepath);
    assert(fs_file_size_opt != std::nullopt);
    // If one of them are empty, fail
    if (vs_file_size_opt == std::nullopt || fs_file_size_opt == std::nullopt)
    {
        LogE("< Cannot create the program");
        return utils::Result<std::vector<app::graphics::Shader::Module>>::Error((char*)"vertex or fragment shader is NULL");
    }
    // Get the content of the VS
    const auto vs_file_size = vs_file_size_opt.value();
    auto vs_buffer = new char[vs_file_size];
    readFile(vertex_shader_filepath, &vs_buffer, vs_file_size);
    Log("> For VS file '%s', read file ok (%d bytes)", vertex_shader_filepath, vs_file_size);
    // Get the content of the FS
    const auto fs_file_size = fs_file_size_opt.value();
    auto fs_buffer = new char[fs_file_size];
    readFile(fragment_shader_filepath, &fs_buffer, fs_file_size);
    Log("> For FS file '%s', read file ok (%d bytes)", fragment_shader_filepath, fs_file_size);
    std::vector<app::graphics::Shader::Module> shader_modules(
        {app::graphics::Shader::Module{
             .m_code = fs_buffer,
             .m_size = fs_file_size,
             .m_tag = (char*)fragment_shader_filepath,
             .m_type = app::graphics::Shader::Type::FRAGMENT_SHADER,
         },
         app::graphics::Shader::Module{
             .m_code = vs_buffer,
             .m_size = vs_file_size,
             .m_tag = (char*)vertex_shader_filepath,
             .m_type = app::graphics::Shader::Type::VERTEX_SHADER,
         }});
    return utils::Result<std::vector<app::graphics::Shader::Module>>::Ok(shader_modules);
}

void app::graphics::Pipeline::setShaderModules(const std::vector<VkShaderModule> shader_modules)
{
    m_shader_modules = shader_modules;
}

void app::graphics::Pipeline::setShaderStages(const std::vector<VkPipelineShaderStageCreateInfo> stages)
{
    m_shader_stages = stages;
}

static VkViewport createViewport(size_t height, size_t width)
{
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.height = (float)height;
    viewport.width = (float)width;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    return viewport;
}

static VkRect2D createScissor(const VkExtent2D& swapchain_extent)
{
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchain_extent;
    return scissor;
}

VkRenderPass& app::graphics::Pipeline::getRenderPass()
{
    return m_render_pass;
}

utils::VResult app::graphics::Pipeline::setupRenderPass()
{
    Log("> Setting up the render pass object of the graphics pipeline");

    const auto NB_ATTACHMENTS = 1;
    // Setup the color attachment format & samples
    VkAttachmentDescription attachments[NB_ATTACHMENTS] = {
        VkAttachmentDescription{
            .format = app::Engine::getInstance()->m_swapchain->getImageFormat().format,
            .samples = VK_SAMPLE_COUNT_1_BIT,        // No multi-sampling: 1 sample
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,   // Before rendering: clear the framebuffer to black before drawing
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE, // After rendering: store in memory to read it again later
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,     // Don't care what previous layout the image was in
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, // Images will be transitioned to the SwapChain for presentation
        },
    };

    // Subpasses and attachment references, as a render pass
    // can consist of multiple subpasses
    VkAttachmentReference color_attachment_reference{};
    color_attachment_reference.attachment = 0; // Index 0
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    const auto NB_SUBPASSES = 1;
    VkSubpassDescription subpasses[NB_SUBPASSES] = {
        VkSubpassDescription{
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &color_attachment_reference,
        },
    };

    VkSubpassDependency dependency{
        .srcSubpass = VK_SUBPASS_EXTERNAL, // Implicit subpass before or after the render pass
        .dstSubpass = 0,                   // our subpass
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    VkRenderPassCreateInfo render_pass_info{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = NB_ATTACHMENTS,
        .pAttachments = attachments,
        .subpassCount = NB_SUBPASSES,
        .pSubpasses = subpasses,
        .dependencyCount = 1,
        .pDependencies = &dependency,
    };

    const auto create_result_code = vkCreateRenderPass(
        app::Engine::getInstance()->m_graphics_device.getLogicalDevice(),
        &render_pass_info,
        nullptr,
        &m_render_pass);

    if (create_result_code != VK_SUCCESS)
    {
        return utils::VResult::Error((char*)"Failed to create the render pass");
    }
    return utils::VResult::Ok();
}

utils::VResult app::graphics::Pipeline::preconfigure()
{
    Log("> Preconfiguring the graphics pipeline");

    VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    const auto create_result_code = vkCreatePipelineLayout(
        app::Engine::getInstance()->m_graphics_device.getLogicalDevice(),
        &pipeline_layout_create_info,
        nullptr,
        &m_layout);

    if (create_result_code != VK_SUCCESS)
    {
        return utils::VResult::Error((char*)"Failed to create the pipeline layout!");
    }

    return createSyncObjects();
}

utils::VResult app::graphics::Pipeline::create()
{
    Log("> Creating the graphics pipeline");
    if (m_shader_stages.size() == 0)
    {
        return utils::VResult::Error((char*)"No shader stages to finalize the graphics pipeline creation - ok?");
    }

    if (m_layout == VK_NULL_HANDLE)
    {
        return utils::VResult::Error((char*)"Cannot create the graphics pipeline without pipeline layout information");
    }

    // TODO: make this array as a class parameter
    // If array belongs to the class parameter, move it to std::vector
    VkDynamicState dynamic_states[2] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo dynamic_state_create_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = sizeof(dynamic_states) / sizeof(VkDynamicState),
        .pDynamicStates = dynamic_states,
    };

    // TODO: change for dynamic state, in order to pass the viewport & scissor
    // through the command buffer
    const VkExtent2D& swapchain_extent = app::Engine::getInstance()->m_swapchain->getExtent();
    const VkViewport viewport = createViewport(swapchain_extent.height, swapchain_extent.width);
    const VkRect2D scissor = createScissor(swapchain_extent);
    VkPipelineViewportStateCreateInfo viewport_state_create_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
    };

    const auto vertex_binding_description = app::shaders::VertexUtils::getVertexBindingDescription();
    const auto vertex_attribute_descriptions = app::shaders::VertexUtils::getVertexAttributeDescriptions();

    // Vertex data settings:
    // * bindings: spacing between data, and whether the data is per-vertex or per-instance,
    // * attribute descriptions: type of the attributes passed to the vertex shader, offset, binding(s) to load, ...
    VkPipelineVertexInputStateCreateInfo vertex_input_create_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        // Vertex binding description
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertex_binding_description,
        // Vertex attribute description
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_attribute_descriptions.size()),
        .pVertexAttributeDescriptions = vertex_attribute_descriptions.data(),
    };

    // Describes the kind of geometry that will be used, and if primitive restart
    // should be enabled
    VkPipelineInputAssemblyStateCreateInfo assembly_state_create_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        // Setting VK_TRUE to primitiveRestartEnable returns a validation
        // error (VUID-VkPipelineInputAssemblyStateCreateInfo-topology-00428)
        .primitiveRestartEnable = VK_FALSE,
    };

    VkPipelineRasterizationStateCreateInfo rasterizer_state_create_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1,
    };

    // TODO: Check if needs another GPU feature
    VkPipelineMultisampleStateCreateInfo multisample_state_create_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
    };

    // TODO: Check if needed
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_FALSE,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
    };

    // Combine fragment shader's color with existing framebuffer's color,
    // configured **per** framebuffer.
    // Both modes are disabled, the fragment colors will be written to
    // the framebuffer unmodified.
    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                            VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT |
                                            VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment,
    };

    VkGraphicsPipelineCreateInfo pipeline_info{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = (uint32_t)m_shader_stages.size(),
        .pStages = m_shader_stages.data(),
        .pVertexInputState = &vertex_input_create_info,
        .pInputAssemblyState = &assembly_state_create_info,
        .pViewportState = &viewport_state_create_info,
        .pRasterizationState = &rasterizer_state_create_info,
        .pMultisampleState = &multisample_state_create_info,
        .pDepthStencilState = &depth_stencil_state_create_info,
        .pColorBlendState = &color_blend_state_create_info,
        .pDynamicState = &dynamic_state_create_info,
        .layout = m_layout,
        .renderPass = m_render_pass,
        .subpass = 0, // index of the subpass
    };

    const auto create_result_code = vkCreateGraphicsPipelines(
        app::Engine::getInstance()->m_graphics_device.getLogicalDevice(),
        VK_NULL_HANDLE,
        1,
        &pipeline_info,
        nullptr,
        &m_pipeline);

    if (create_result_code != VK_SUCCESS)
    {
        return utils::VResult::Error((char*)"Failed to create the main graphics pipeline");
    }
    return utils::VResult::Ok();
}

utils::VResult app::graphics::Pipeline::createVertexBuffer() noexcept
{
    VkDevice graphics_device = app::Engine::getInstance()->m_graphics_device.getLogicalDevice();
    VmaAllocator resource_allocator = app::Engine::getInstance()->m_allocator;
    if (VK_NULL_HANDLE != m_vertex_buffer)
    {
        Log("< Destroying the vertex buffer...");
        vmaDestroyBuffer(resource_allocator, m_vertex_buffer, m_vertex_buffer_allocation);
        m_vertex_buffer = VK_NULL_HANDLE;
        m_vertex_buffer_allocation = VK_NULL_HANDLE;
    }
    VkQueue transfert_queue = app::Engine::getInstance()->m_graphics_device.getTransfertQueue();
    VkCommandPool* transfert_command_pool = app::Engine::getInstance()->m_render->getTransfertCommand()->getPool();
    const size_t buffer_size = 0;
    assert(transfert_command_pool != nullptr);

    // Use staging buffer (or temporary buffer) to transfer next from CPU to GPU
    // This buffer can be used as source in a memory transfer operation
    VkBuffer staging_buffer{};
    VmaAllocation staging_buffer_allocation{};
    // if (const auto result = app::graphics::Memory::initBuffer(
    //         resource_allocator,
    //         &staging_buffer_allocation,
    //         graphics_device,
    //         buffer_size,
    //         staging_buffer,
    //         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    //     result.IsError())
    //     return result;

    // // Now, fill the staging buffer with the actual data
    // void* data;
    // vmaMapMemory(resource_allocator, staging_buffer_allocation, &data);
    // // TODO: fix the memcpy
    // memcpy(data, nullptr, (size_t)buffer_size);
    // vmaUnmapMemory(resource_allocator, staging_buffer_allocation);

    // // Initialize the actual vertex buffer
    // // This buffer can be used as destination in a memory transfert operation
    // if (m_vertex_buffer != VK_NULL_HANDLE)
    // {
    //     LogW("the vertex buffer has already been intialized - resetting it...");
    //     vmaDestroyBuffer(resource_allocator, m_vertex_buffer, m_index_buffer_allocation);
    //     m_index_buffer_allocation = VK_NULL_HANDLE;
    // }
    // m_vertex_buffer = VkBuffer();
    // if (const auto result = app::graphics::Memory::initBuffer(
    //         resource_allocator,
    //         &m_vertex_buffer_allocation,
    //         graphics_device,
    //         buffer_size,
    //         m_vertex_buffer,
    //         VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    //     result.IsError())
    //     return result;

    // // Now, copy the data
    // if (const auto operation_result = app::graphics::Memory::copyBuffer(
    //         graphics_device,
    //         staging_buffer,
    //         m_vertex_buffer,
    //         transfert_command_pool,
    //         transfert_queue,
    //         buffer_size);
    //     operation_result.IsError())
    // {
    //     vmaDestroyBuffer(resource_allocator, staging_buffer, staging_buffer_allocation);
    //     staging_buffer_allocation = VK_NULL_HANDLE;
    //     return operation_result;
    // }

    // vmaDestroyBuffer(resource_allocator, staging_buffer, staging_buffer_allocation);
    // staging_buffer_allocation = VK_NULL_HANDLE;

    return utils::VResult::Ok();
}

utils::VResult app::graphics::Pipeline::createIndexBuffer() noexcept
{
    VkDevice graphics_device = app::Engine::getInstance()->m_graphics_device.getLogicalDevice();
    VmaAllocator resource_allocator = app::Engine::getInstance()->m_allocator;
    if (VK_NULL_HANDLE != m_index_buffer)
    {
        Log("< Destroying the index buffer...");
        vmaDestroyBuffer(resource_allocator, m_index_buffer, m_index_buffer_allocation);
        m_index_buffer = VK_NULL_HANDLE;
        m_index_buffer_allocation = VK_NULL_HANDLE;
    }
    VkQueue transfert_queue = app::Engine::getInstance()->m_graphics_device.getTransfertQueue();
    VkCommandPool* transfert_command_pool = app::Engine::getInstance()->m_render->getTransfertCommand()->getPool();
    const size_t buffer_size = 0;
    assert(transfert_command_pool != nullptr);

    // Use staging buffer (or temporary buffer) to transfer next from CPU to GPU
    // This buffer can be used as source in a memory transfer operation
    VkBuffer staging_buffer{};
    VmaAllocation staging_buffer_allocation{};
    // if (const auto result = app::graphics::Memory::initBuffer(
    //         resource_allocator,
    //         &staging_buffer_allocation,
    //         graphics_device,
    //         buffer_size,
    //         staging_buffer,
    //         VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    //     result.IsError())
    //     return result;

    // // Now, fill the staging buffer with the actual data
    // void* data;
    // vmaMapMemory(resource_allocator, staging_buffer_allocation, &data);
    // // TODO: fix the memcpy
    // memcpy(data, nullptr, (size_t)buffer_size);
    // vmaUnmapMemory(resource_allocator, staging_buffer_allocation);

    // // Initialize the actual vertex buffer
    // // This buffer can be used as destination in a memory transfert operation
    // if (m_index_buffer != VK_NULL_HANDLE)
    // {
    //     LogW("the index buffer has already been intialized - resetting it...");
    //     vmaDestroyBuffer(resource_allocator, m_index_buffer, m_index_buffer_allocation);
    //     m_index_buffer_allocation = VK_NULL_HANDLE;
    // }
    // m_index_buffer = VkBuffer();
    // m_index_buffer_allocation = {};
    // if (const auto result = app::graphics::Memory::initBuffer(
    //         resource_allocator,
    //         &m_index_buffer_allocation,
    //         graphics_device,
    //         buffer_size,
    //         m_index_buffer,
    //         VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    //     result.IsError())
    //     return result;

    // // Now, copy the data
    // if (const auto operation_result = app::graphics::Memory::copyBuffer(
    //         graphics_device,
    //         staging_buffer,
    //         m_index_buffer,
    //         transfert_command_pool,
    //         transfert_queue,
    //         buffer_size);
    //     operation_result.IsError())
    // {
    //     vmaDestroyBuffer(resource_allocator, staging_buffer, staging_buffer_allocation);
    //     staging_buffer_allocation = VK_NULL_HANDLE;
    //     return operation_result;
    // }

    // vmaDestroyBuffer(resource_allocator, staging_buffer, staging_buffer_allocation);
    // staging_buffer_allocation = VK_NULL_HANDLE;

    return utils::VResult::Ok();
}

VkPipeline app::graphics::Pipeline::getPipeline()
{
    return m_pipeline;
}

utils::VResult app::graphics::Pipeline::createSyncObjects()
{
    Log("> Creating the sync objects");
    auto graphics_device = app::Engine::getInstance()->m_graphics_device.getLogicalDevice();
    if (nullptr == m_sync_image_ready)
    {
        m_sync_image_ready = new VkSemaphore();
        VkSemaphoreCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        if (VK_SUCCESS != vkCreateSemaphore(graphics_device, &create_info, nullptr, m_sync_image_ready))
            return utils::VResult::Error((char*)"< Failed to create the semaphore to signal image ready");
    }
    if (nullptr == m_sync_present_done)
    {
        m_sync_present_done = new VkSemaphore();
        VkSemaphoreCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        if (VK_SUCCESS != vkCreateSemaphore(graphics_device, &create_info, nullptr, m_sync_present_done))
            return utils::VResult::Error((char*)"< Failed to create the semaphore to signal present is done");
    }
    if (nullptr == m_sync_cpu_gpu)
    {
        m_sync_cpu_gpu = new VkFence();
        VkFenceCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT; // This allows to not wait for the first wait
        if (VK_SUCCESS != vkCreateFence(graphics_device, &create_info, nullptr, m_sync_cpu_gpu))
            return utils::VResult::Error((char*)"< Failed to create the fence");
    }
    return utils::VResult::Ok();
}

void app::graphics::Pipeline::acquireImage()
{
    // Wait that all fences are sync...
    vkWaitForFences(
        app::Engine::getInstance()->m_graphics_device.getLogicalDevice(),
        1,
        m_sync_cpu_gpu,
        VK_TRUE,
        UINT64_MAX);
    // ... and reset them
    vkResetFences(
        app::Engine::getInstance()->m_graphics_device.getLogicalDevice(),
        1,
        m_sync_cpu_gpu);

    // Acquire the new frame
    vkAcquireNextImageKHR(
        app::Engine::getInstance()->m_graphics_device.getLogicalDevice(),
        app::Engine::getInstance()->m_swapchain->getSwapchainDevice(),
        UINT64_MAX,
        *m_sync_image_ready,
        VK_NULL_HANDLE,
        &app::Engine::getInstance()->m_render->getFrameIndex());
}

void app::graphics::Pipeline::present()
{
    VkSemaphore signal[] = {*m_sync_present_done};
    VkSwapchainKHR swapchains[] = {
        app::Engine::getInstance()->m_swapchain->getSwapchainDevice()};
    VkPresentInfoKHR present_info{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signal,
        .swapchainCount = 1,
        .pSwapchains = swapchains,
        .pImageIndices = &app::Engine::getInstance()->m_render->getFrameIndex(),
    };
    vkQueuePresentKHR(
        app::Engine::getInstance()->m_graphics_device.getPresentsQueue(),
        &present_info);
}

utils::Result<int> app::graphics::Pipeline::draw()
{
    // Reset the command buffer
    auto command_buffer = app::Engine::getInstance()->m_render->getGraphicsCommand();
    if (command_buffer == nullptr)
        return utils::Result<int>::Error((char*)"Cannot get the command buffer in the draw call");
    vkResetCommandBuffer(
        *(command_buffer.get()->getBuffer()),
        0);
    // Record the current command
    app::Engine::getInstance()->m_render->getGraphicsCommand()->record();
    // Submit
    VkSemaphore wait_semaphores[] = {*m_sync_image_ready};
    VkSemaphore signal_semaphores[] = {*m_sync_present_done};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submit_info{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1, // TODO: to change for something more idiomatic / maintainable
        .pWaitSemaphores = wait_semaphores,
        .pWaitDstStageMask = wait_stages,
        .commandBufferCount = 1,
        .pCommandBuffers = command_buffer->getBuffer(),
        .signalSemaphoreCount = 1, // TODO: to change for something more idiomatic / maintainable
        .pSignalSemaphores = signal_semaphores,
    };

    if (const auto submit_result_code = vkQueueSubmit(
            app::Engine::getInstance()->m_graphics_device.getGraphicsQueue(),
            1,
            &submit_info,
            *m_sync_cpu_gpu);
        submit_result_code != VK_SUCCESS)
    {
        return utils::Result<int>::Error((char*)"Error submitting the queue in Draw call");
    }

    return utils::Result<int>::Ok(0);
}

const VkBuffer& app::graphics::Pipeline::getVertexBuffer() noexcept
{
    return m_vertex_buffer;
}

const VkBuffer& app::graphics::Pipeline::getIndexBuffer() noexcept
{
    return m_index_buffer;
}
