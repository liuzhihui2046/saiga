﻿/*
* Vulkan Example - imGui (https://github.com/ocornut/imgui)
*
* Copyright (C) 2017 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <array>
#include <algorithm>

//#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>
//#include <gli/gli.hpp>

#include <saiga/imgui/imgui.h>

#include "saiga/vulkan/SDLWindow.h"
#include "saiga/vulkan/base/VulkanImgui.h"


#include "saiga/vulkan/AssetRenderer.h"
#include "saiga/assets/objAssetLoader.h"

#include "saiga/sdl/sdl_camera.h"

#define ENABLE_VALIDATION true

// ----------------------------------------------------------------------------
// VulkanExample
// ----------------------------------------------------------------------------


// Options and values to display/toggle from the UI
struct UISettings {
    bool displayModels = true;
    bool displayLogos = true;
    bool displayBackground = true;
    bool animateLight = false;
    float lightSpeed = 0.25f;
    std::array<float, 50> frameTimes{};
    float frameTimeMin = 9999.0f, frameTimeMax = 0.0f;
    float lightTimer = 0.0f;
} ;

extern UISettings uiSettings;

class VulkanExample : public Saiga::Vulkan::SDLWindow
{
public:
    Saiga::SDLCamera<Saiga::PerspectiveCamera> camera;
    std::shared_ptr<Saiga::Vulkan::ImGuiVulkanRenderer> imGui;

    Saiga::Vulkan::Asset teapot;


    Saiga::Vulkan::AssetRenderer assetRenderer;

    VulkanExample();

    ~VulkanExample();
	
    void buildCommandBuffers();


    void updateUniformBuffers();

    void draw();

    void loadAssets();

    void prepareImGui();

    void prepare();

    virtual void render();

	virtual void viewChanged()
	{
		updateUniformBuffers();
	}

	virtual void mouseMoved(double x, double y, bool &handled)
	{
		ImGuiIO& io = ImGui::GetIO();	
		handled = io.WantCaptureMouse;
	}

};

