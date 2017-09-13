#include "assignment1.hpp"

#include "config.hpp"
#include "external/glad/glad.h"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/helpers.hpp"
#include "core/InputHandler.h"
#include "core/Log.h"
#include "core/LogView.h"
#include "core/Misc.h"
#include "core/node.hpp"
#include "core/opengl.hpp"
#include "core/utils.h"
#include "core/various.hpp"
#include "core/Window.h"
#include <imgui.h>
#include "external/imgui_impl_glfw_gl3.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <array>
#include <chrono>
#include <cstdlib>
#include <unordered_map>
#include <stack>
#include <stdexcept>
#include <vector>


edaf80::Assignment1::Assignment1()
{
	Log::View::Init();

	window = Window::Create("EDAF80: Assignment 1", config::resolution_x,
	                        config::resolution_y, config::msaa_rate, false);
	if (window == nullptr) {
		Log::View::Destroy();
		throw std::runtime_error("Failed to get a window: aborting!");
	}
	inputHandler = new InputHandler();
	window->SetInputHandler(inputHandler);
}

edaf80::Assignment1::~Assignment1()
{
	delete inputHandler;
	inputHandler = nullptr;

	Window::Destroy(window);
	window = nullptr;

	Log::View::Destroy();
}

void
edaf80::Assignment1::run()
{
	// Load the sphere geometry
	auto const objects = bonobo::loadObjects("sphere.obj");
	if (objects.empty())
		return;
	auto const& sphere = objects.front();

	// Set up the camera
	FPSCameraf mCamera(bonobo::pi / 4.0f,
	                   static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
	                   0.01f, 1000.0f);
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 0.0f, 6.0f));
	mCamera.mMouseSensitivity = 0.003f;
	mCamera.mMovementSpeed = 0.25f * 12.0f;
	window->SetCamera(&mCamera);

	// Create the shader program
	auto shader = bonobo::createProgram("default.vert", "default.frag");
	if (shader == 0u) {
		LogError("Failed to load shader");
		return;
	}

	// Load the sun's texture
	auto sun_texture = bonobo::loadTexture2D("sunmap.png");

	auto sun = Node();
	sun.set_geometry(sphere);
	sun.set_program(shader, [](GLuint /*program*/){});
	//
	// Todo: Attach a texture to the sun
	// DONE
    sun.add_texture("diffuse_texture", sun_texture, GL_TEXTURE_2D);


	//
	// Todo: Create an Earth node
	//
    // Load the sun's texture
    auto earth_texture = bonobo::loadTexture2D("earth_diffuse.png");
    auto pivot = Node();

    auto earth = Node();
    earth.set_geometry(sphere);
    earth.set_program(shader, [](GLuint /*program*/){});

    earth.add_texture("diffuse_texture", earth_texture, GL_TEXTURE_2D);

    //earth moon
    auto emarble = Node();
    auto epivot = Node();
    emarble.set_geometry(sphere);
    emarble.set_program(shader, [](GLuint /*program*/){});
    auto emarble_texture = bonobo::loadTexture2D("marble.png");
    emarble.add_texture("diffuse_texture", emarble_texture, GL_TEXTURE_2D);


    //saturn
    auto saturn_texture = bonobo::loadTexture2D("jupiter.png");
    auto pivot2 = Node();

    auto saturn = Node();
    saturn.set_geometry(sphere);
    saturn.set_program(shader, [](GLuint /*program*/){});
    saturn.add_texture("diffuse_texture", saturn_texture, GL_TEXTURE_2D);

    auto sring_texture = bonobo::loadTexture2D("saturn_rings.png");
    auto sring = Node();
    sring.set_geometry(sphere);
    sring.set_program(shader, [](GLuint /*program*/){});
    sring.add_texture("diffuse_texture", saturn_texture, GL_TEXTURE_2D);



    // Add node
    auto world = Node();
    world.add_child(&sun);
    world.add_child(&pivot);
    world.add_child(&pivot2);

    pivot.add_child(&earth);
    pivot2.add_child(&saturn);

    earth.add_child(&epivot);
    saturn.add_child(&sring);

    epivot.add_child(&emarble);

    glEnable(GL_DEPTH_TEST);

	f64 ddeltatime;
	size_t fpsSamples = 0;
	double nowTime, lastTime = GetTimeSeconds();
	double fpsNextTick = lastTime + 1.0;

	while (!glfwWindowShouldClose(window->GetGLFW_Window())) {
		nowTime = GetTimeSeconds();
		ddeltatime = nowTime - lastTime;
		if (nowTime > fpsNextTick) {
			fpsNextTick += 1.0;
			fpsSamples = 0;
		}
		fpsSamples++;

		auto& io = ImGui::GetIO();
		inputHandler->SetUICapture(io.WantCaptureMouse, io.WantCaptureMouse);

		glfwPollEvents();
		inputHandler->Advance();
		mCamera.Update(ddeltatime, *inputHandler);

		ImGui_ImplGlfwGL3_NewFrame();


		//
		// How-To: Translate the sun
        //
        //sun.set_translation(glm::vec3(std::sin(nowTime), 0.0f, 0.0f));
        sun.rotate_y(std::sin(nowTime/(nowTime*10)));

        earth.set_translation(glm::vec3(2.5f, 0.0f, 1.0f));
        emarble.set_translation(glm::vec3(2.5f, 0.0f, 1.0f));
        saturn.set_translation(glm::vec3(2.5f, 0.0f, 1.0f));
        //earth.set_translation(glm::vec3(1.0f, 1.0f, 1.0f));
        earth.set_scaling(glm::vec3(.6f));
        emarble.set_scaling(glm::vec3(.3f));
        //earth.scale(glm::vec3(4.3f));
        //earth.set_rotation_y(.7);
        pivot.rotate_y(nowTime/(nowTime*20));
        earth.rotate_z(nowTime/(nowTime*10));

        epivot.rotate_z(nowTime/(nowTime*24));
        epivot.rotate_y(nowTime/(nowTime*10));
        //earth.set_translation(glm::vec3(std::sin(nowTime), std::sin(nowTime), 4*std::sin(nowTime)));

		auto const window_size = window->GetDimensions();
		glViewport(0, 0, window_size.x, window_size.y);
		glClearDepthf(1.0f);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		// Traverse the scene graph and render all the nodes
		auto node_stack = std::stack<Node const*>();
		auto matrix_stack = std::stack<glm::mat4>();
		node_stack.push(&world);
		matrix_stack.push(glm::mat4());
		do {
			auto const* const current_node = node_stack.top();
			node_stack.pop();

			auto const parent_matrix = matrix_stack.top();
			matrix_stack.pop();

			auto const current_node_matrix = current_node->get_transform();

			//
			// Todo: Compute the current node's world matrix
			//
			auto const current_node_world_matrix = parent_matrix * current_node_matrix;
			current_node->render(mCamera.GetWorldToClipMatrix(), current_node_world_matrix);

			for (int i = static_cast<int>(current_node->get_children_nb()) - 1; i >= 0; --i) {
				node_stack.push(current_node->get_child(static_cast<size_t>(i)));
				matrix_stack.push(current_node_world_matrix);
			}
		} while (!node_stack.empty());

		Log::View::Render();
		ImGui::Render();

		window->Swap();
		lastTime = nowTime;
	}

	glDeleteProgram(shader);
	shader = 0u;
}

int main()
{
	Bonobo::Init();
	try {
		edaf80::Assignment1 assignment1;
		assignment1.run();
	}
	catch (std::runtime_error const& e) {
		LogError(e.what());
	}
	Bonobo::Destroy();
}
