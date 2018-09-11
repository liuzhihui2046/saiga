﻿/*
* Vulkan Example - imGui (https://github.com/ocornut/imgui)
*
* Copyright (C) 2017 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include "saiga/vulkan/VulkanForwardRenderer.h"
#include "saiga/vulkan/renderModules/AssetRenderer.h"
#include "saiga/vulkan/renderModules/LineAssetRenderer.h"
#include "saiga/vulkan/renderModules/PointCloudRenderer.h"
#include "saiga/vulkan/renderModules/TexturedAssetRenderer.h"
#include "saiga/vulkan/renderModules/TextureDisplay.h"
#include "saiga/sdl/sdl_camera.h"
#include "saiga/sdl/sdl_eventhandler.h"
#include "saiga/window/Interfaces.h"


class VulkanExample :  public Saiga::Updating, public Saiga::Vulkan::VulkanForwardRenderingInterface, public Saiga::SDL_KeyListener
{
public:
    VulkanExample(
            Saiga::Vulkan::VulkanWindow& window, Saiga::Vulkan::VulkanForwardRenderer& renderer
            );
    ~VulkanExample();

    virtual void init(Saiga::Vulkan::VulkanBase& base);


    virtual void update(float dt) override;
    virtual void transfer(vk::CommandBuffer cmd);
    virtual void render  (vk::CommandBuffer cmd) override;
    virtual void renderGUI() override;
private:
    Saiga::SDLCamera<Saiga::PerspectiveCamera> camera;


    bool change = true;
    Saiga::Object3D teapotTrans;

    std::shared_ptr<Saiga::Vulkan::Texture2D> texture;

    Saiga::Vulkan::VulkanTexturedAsset box;
    Saiga::Vulkan::VulkanVertexColoredAsset teapot,plane;
    Saiga::Vulkan::VulkanLineVertexColoredAsset grid, frustum;
    Saiga::Vulkan::VulkanPointCloudAsset pointCloud;
    Saiga::Vulkan::AssetRenderer assetRenderer;
    Saiga::Vulkan::LineAssetRenderer lineAssetRenderer;
    Saiga::Vulkan::PointCloudRenderer pointCloudRenderer;
    Saiga::Vulkan::TexturedAssetRenderer texturedAssetRenderer;


    vk::DescriptorSet textureDes;
    Saiga::Vulkan::TextureDisplay textureDisplay;

    Saiga::Vulkan::VulkanForwardRenderer &renderer;

    bool displayModels = true;


    void keyPressed(SDL_Keysym key) override;
    void keyReleased(SDL_Keysym key) override;
};

