#include "assignment4.hpp"

#include "config.hpp"
#include "external/glad/glad.h"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/helpers.hpp"
#include "core/InputHandler.h"
#include "core/Log.h"
#include "core/LogView.h"
#include "core/Misc.h"
#include "core/utils.h"
#include "core/Window.h"
#include <imgui.h>
#include "external/imgui_impl_glfw_gl3.h"

#include "external/glad/glad.h"
#include "parametric_shapes.hpp"
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <core/node.hpp>
#include <glm/src/glm/glm/gtc/type_ptr.hpp>

enum class polygon_mode_t : unsigned int {
    fill = 0u,
    line,
    point
};

static polygon_mode_t get_next_mode(polygon_mode_t mode)
{
    return static_cast<polygon_mode_t>((static_cast<unsigned int>(mode) + 1u) % 3u);
}

edaf80::Assignment4::Assignment4()
{
	Log::View::Init();

	window = Window::Create("EDAF80: Assignment 4", config::resolution_x,
	                        config::resolution_y, config::msaa_rate, false);
	if (window == nullptr) {
		Log::View::Destroy();
		throw std::runtime_error("Failed to get a window: aborting!");
	}
	inputHandler = new InputHandler();
	window->SetInputHandler(inputHandler);
}

edaf80::Assignment4::~Assignment4()
{
	delete inputHandler;
	inputHandler = nullptr;

	Window::Destroy(window);
	window = nullptr;

	Log::View::Destroy();
}

void
edaf80::Assignment4::run()
{
	// Set up the camera
	FPSCameraf mCamera(bonobo::pi / 4.0f,
	                   static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
	                   0.01f, 1000.0f);
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 0.0f, 6.0f));
	mCamera.mMouseSensitivity = 0.003f;
	mCamera.mMovementSpeed = 0.025;
	window->SetCamera(&mCamera);

	// Create the shader programs
	auto fallback_shader = bonobo::createProgram("fallback.vert", "fallback.frag");
	if (fallback_shader == 0u) {
		LogError("Failed to load fallback shader");
		return;
	}

    GLuint water_shader = 0u;
    auto const reload_shaders = [&water_shader](){
        if (water_shader != 0u)
            glDeleteProgram(water_shader);
        water_shader = bonobo::createProgram("water.vert", "water.frag");
        if (water_shader == 0u)
            LogError("Failed to load water map shader");

    };
    reload_shaders();


    auto light_position = glm::vec3(-2.0f, 4.0f, 2.0f);
    auto const set_uniforms = [&light_position](GLuint program) {
        glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
    };

    auto camera_position = mCamera.mWorld.GetTranslation();
    float u_time = 0.0f;
    auto const water_set_uniforms = [&camera_position, &u_time](GLuint program) {
        glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
        glUniform1f(glGetUniformLocation(program, "u_time"), u_time);
    };


    auto polygon_mode = polygon_mode_t::fill;


    auto quad_shape = parametric_shapes::createHQuad(20u, 20u);
    auto quad = Node();
    quad.set_geometry(quad_shape);
    quad.set_program(water_shader, water_set_uniforms);



    auto const cir = parametric_shapes::createCircleRing(4u, 60u, 1.0f, 2.0f);
    auto circle_rings = Node();
    circle_rings.set_geometry(cir);
    circle_rings.set_program(fallback_shader, water_set_uniforms);

	//
	// Todo: Load your geometry
	//

	glEnable(GL_DEPTH_TEST);

	// Enable face culling to improve performance:
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);
	//glCullFace(GL_BACK);


	f64 ddeltatime;
	size_t fpsSamples = 0;
	double nowTime, lastTime = GetTimeMilliseconds();
	double fpsNextTick = lastTime + 1000.0;

	while (!glfwWindowShouldClose(window->GetGLFW_Window())) {
		nowTime = GetTimeMilliseconds();
		ddeltatime = nowTime - lastTime;
		if (nowTime > fpsNextTick) {
			fpsNextTick += 1000.0;
			fpsSamples = 0;
		}
		fpsSamples++;
        u_time += ddeltatime/1000;

		auto& io = ImGui::GetIO();
		inputHandler->SetUICapture(io.WantCaptureMouse, io.WantCaptureMouse);
        glfwPollEvents();
		inputHandler->Advance();
		mCamera.Update(ddeltatime, *inputHandler);

        camera_position = mCamera.mWorld.GetTranslation();

        ImGui_ImplGlfwGL3_NewFrame();

		//
		// Todo: If you need to handle inputs, you can do it here
		//
        if (inputHandler->GetKeycodeState(GLFW_KEY_5) & JUST_PRESSED) {
            quad.set_program(water_shader, water_set_uniforms);
            circle_rings.set_program(water_shader, water_set_uniforms);
        }
        if (inputHandler->GetKeycodeState(GLFW_KEY_Z) & JUST_PRESSED) {
            polygon_mode = get_next_mode(polygon_mode);
        }
		if (inputHandler->GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED) {
			reload_shaders();
		}

        switch (polygon_mode) {
            case polygon_mode_t::fill:
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                break;
            case polygon_mode_t::line:
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                break;
            case polygon_mode_t::point:
                glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
                break;
        }

		auto const window_size = window->GetDimensions();
		glViewport(0, 0, window_size.x, window_size.y);
		glClearDepthf(1.0f);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		//
		// Todo: Render all your geometry here.
		//


//        circle_rings.render(mCamera.GetWorldToClipMatrix(), circle_rings.get_transform());
//
//        quad.render(mCamera.GetWorldToClipMatrix(), quad.get_transform());
        quad.render(mCamera.GetWorldToClipMatrix(), quad.get_transform());

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		Log::View::Render();

		//
		// Todo: If you want a custom ImGUI window, you can set it up
		//       here
		//

		ImGui::Render();

		window->Swap();
		lastTime = nowTime;
	}


    glDeleteProgram(fallback_shader);
    fallback_shader = 0u;
    glDeleteProgram(water_shader);
    water_shader = 0u;
	//
	// Todo: Do not forget to delete your shader programs, by calling
	//       `glDeleteProgram($your_shader_program)` for each of them.
	//
}

int main()
{
	Bonobo::Init();
	try {
		edaf80::Assignment4 assignment4;
		assignment4.run();
	} catch (std::runtime_error const& e) {
		LogError(e.what());
	}
	Bonobo::Destroy();
}
