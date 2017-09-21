#include "assignment2.hpp"
#include "interpolation.hpp"
#include "parametric_shapes.hpp"

#include "config.hpp"
#include "external/glad/glad.h"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/InputHandler.h"
#include "core/Log.h"
#include "core/LogView.h"
#include "core/Misc.h"
#include "core/node.hpp"
#include "core/utils.h"
#include "core/Window.h"
#include <imgui.h>
#include "external/imgui_impl_glfw_gl3.h"

#include "external/glad/glad.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdlib>
#include <stdexcept>

enum class polygon_mode_t : unsigned int {
	fill = 0u,
	line,
	point
};

static polygon_mode_t get_next_mode(polygon_mode_t mode)
{
	return static_cast<polygon_mode_t>((static_cast<unsigned int>(mode) + 1u) % 3u);
}

edaf80::Assignment2::Assignment2()
{
	Log::View::Init();

	window = Window::Create("EDAF80: Assignment 2", config::resolution_x,
	                        config::resolution_y, config::msaa_rate, false);
	if (window == nullptr) {
		Log::View::Destroy();
		throw std::runtime_error("Failed to get a window: aborting!");
	}
	inputHandler = new InputHandler();
	window->SetInputHandler(inputHandler);
}

edaf80::Assignment2::~Assignment2()
{
	delete inputHandler;
	inputHandler = nullptr;

	Window::Destroy(window);
	window = nullptr;

	Log::View::Destroy();
}

void
edaf80::Assignment2::run()
{
	// Load the sphere geometry
	auto const shape = parametric_shapes::createCircleRing(4u, 60u, 1.0f, 2.0f);
	if (shape.vao == 0u)
		return;

	// Set up the camera
	FPSCameraf mCamera(bonobo::pi / 4.0f,
	                   static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
	                   0.01f, 1000.0f);
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 0.0f, 6.0f));
	mCamera.mMouseSensitivity = 0.003f;
	mCamera.mMovementSpeed = 0.25f * 12.0f;
	window->SetCamera(&mCamera);

	// Create the shader programs
	auto fallback_shader = bonobo::createProgram("fallback.vert", "fallback.frag");
	if (fallback_shader == 0u) {
		LogError("Failed to load fallback shader");
		return;
	}
	auto diffuse_shader = bonobo::createProgram("diffuse.vert", "diffuse.frag");
	if (diffuse_shader == 0u) {
		LogError("Failed to load diffuse shader");
		return;
	}
	auto normal_shader = bonobo::createProgram("normal.vert", "normal.frag");
	if (normal_shader == 0u) {
		LogError("Failed to load normal shader");
		return;
	}
	auto texcoord_shader = bonobo::createProgram("texcoord.vert", "texcoord.frag");
	if (texcoord_shader == 0u) {
		LogError("Failed to load texcoord shader");
		return;
	}

	auto const light_position = glm::vec3(-2.0f, 4.0f, 2.0f);
	auto const set_uniforms = [&light_position](GLuint program){
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
	};

	// Set the default tensions value; it can always be changed at runtime
	// through the "Scene Controls" window.
	float catmull_rom_tension = 0.0f;

	// Set whether the default interpolation algorithm should be the linear one;
	// it can always be changed at runtime through the "Scene Controls" window.
	bool use_linear = true;

	auto circle_rings = Node();
	circle_rings.set_geometry(shape);
	circle_rings.set_program(fallback_shader, set_uniforms);

    auto quad = Node();
    auto const q = parametric_shapes::createQuad(4u, 6u);
    quad.set_geometry(q);
    quad.set_program(fallback_shader, set_uniforms);
    circle_rings.add_child(&quad);
    auto sphere = Node();
    auto const s = parametric_shapes::createSphere(30u, 40u, 1.0f);
    sphere.set_geometry(s);
    sphere.set_program(fallback_shader, set_uniforms);

    auto tor = Node();
    auto const t = parametric_shapes::createTorus(5u, 10u, 2.0f, 3.0f);
    tor.set_geometry(t);
    tor.set_program(fallback_shader, set_uniforms);


	//! \todo Create a tesselated sphere and a tesselated torus


	auto polygon_mode = polygon_mode_t::fill;

	glEnable(GL_DEPTH_TEST);

	// Enable face culling to improve performance
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);
	//glCullFace(GL_BACK);


	f64 ddeltatime;
	size_t fpsSamples = 0;
	double nowTime, lastTime = GetTimeSeconds();
	double fpsNextTick = lastTime + 1.0;
    float l_inter = 0.0f;
	while (!glfwWindowShouldClose(window->GetGLFW_Window())) {
        nowTime = GetTimeSeconds();
        ddeltatime = nowTime - lastTime;
        if (nowTime > fpsNextTick) {
            fpsNextTick += 1.0;
            fpsSamples = 0;
        }
        fpsSamples++;

        auto &io = ImGui::GetIO();
        inputHandler->SetUICapture(io.WantCaptureMouse, io.WantCaptureMouse);

        glfwPollEvents();
        inputHandler->Advance();
        mCamera.Update(ddeltatime, *inputHandler);

        ImGui_ImplGlfwGL3_NewFrame();


        if (inputHandler->GetKeycodeState(GLFW_KEY_1) & JUST_PRESSED) {
            sphere.set_program(fallback_shader, set_uniforms);
        }
        if (inputHandler->GetKeycodeState(GLFW_KEY_2) & JUST_PRESSED) {
            sphere.set_program(diffuse_shader, set_uniforms);
        }
        if (inputHandler->GetKeycodeState(GLFW_KEY_3) & JUST_PRESSED) {
            sphere.set_program(normal_shader, set_uniforms);
        }
        if (inputHandler->GetKeycodeState(GLFW_KEY_4) & JUST_PRESSED) {
            sphere.set_program(texcoord_shader, set_uniforms);
        }
        if (inputHandler->GetKeycodeState(GLFW_KEY_Z) & JUST_PRESSED) {
            polygon_mode = get_next_mode(polygon_mode);
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

        circle_rings.rotate_y(0.01f);
        quad.set_scaling(glm::vec3(0.3f));

        //! \todo Interpolate the movement of a shape between various
        //!        control points


        auto const window_size = window->GetDimensions();
        glViewport(0, 0, window_size.x, window_size.y);
        glClearDepthf(1.0f);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

        circle_rings.render(mCamera.GetWorldToClipMatrix(), circle_rings.get_transform());


//        quad.render(mCamera.GetWorldToClipMatrix(), circle_rings.get_transform());

        glm::vec3 p1 = glm::vec3(1, 2, 3);
        glm::vec3 p2 = glm::vec3(4, 2, 3);

        glm::vec3 cp[10];

        cp[0] = glm::vec3(1,2,-1);
        cp[1] = glm::vec3(4,8,2);
        cp[2] = glm::vec3(6,3,0);
        cp[3] = glm::vec3(6,2,7);
        cp[4] = glm::vec3(8,-2,8);
        cp[5] = glm::vec3(4,-2,0);
        cp[6] = glm::vec3(0,-6,2);
        cp[7] = glm::vec3(7,8,6);
        cp[8] = glm::vec3(-2,-4,-2);
        cp[9] = glm::vec3(0, 0, 0);

//        cp[0] = glm::vec3(0, 1, 1);
//        cp[1] = glm::vec3(1, 0, 0);
//        cp[2] = glm::vec3(0, -1, 0);




        int counter = 0;
        if (l_inter > 10) {
            l_inter -= 10;
        } else {

            l_inter += ddeltatime;

        }
        int in = static_cast<int>(std::floor(l_inter));
        if (use_linear) {
            glm::vec3 interp_me = interpolation::evalLERP(cp[in % 10], cp[(in + 1) % 10], l_inter - std::floor(l_inter));
//            sphere.set_translation(interp_me);
        } else {
            glm::vec3 interp_me = interpolation::evalCatmullRom(cp[in % 10], cp[(in + 1) % 10], cp[(in + 2) % 10], cp[(in + 3) % 10], catmull_rom_tension, l_inter - std::floor(l_inter));
//            sphere.set_translation(interp_me);
        }
//        glm::vec3 interp_me = interpolation::evalLERP(cp[l_inter % 10], cp[(l_inter + 1) % 10], .5);
//        sphere.set_translation(interp_me);
//        my_x += 0.1;

//        l_inter = l_inter + 1;
//        interp_me = interpolation::evalLERP(cp[l_inter % 2], cp[(l_inter + 1) % 2], .5);
//        sphere.set_translation(interp_me);
        sphere.render(mCamera.GetWorldToClipMatrix(), sphere.get_transform());

//        tor.render(mCamera.GetWorldToClipMatrix(), circle_rings.get_transform());


        bool const opened = ImGui::Begin("Scene Controls", nullptr, ImVec2(300, 100), -1.0f, 0);
		if (opened) {
			ImGui::SliderFloat("Catmull-Rom tension", &catmull_rom_tension, 0.0f, 1.0f);
			ImGui::Checkbox("Use linear interpolation", &use_linear);
		}
		ImGui::End();

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		Log::View::Render();
		ImGui::Render();

		window->Swap();
		lastTime = nowTime;
	}

	glDeleteProgram(texcoord_shader);
	normal_shader = 0u;
	glDeleteProgram(normal_shader);
	normal_shader = 0u;
	glDeleteProgram(diffuse_shader);
	diffuse_shader = 0u;
	glDeleteProgram(fallback_shader);
	diffuse_shader = 0u;
}

int main()
{
	Bonobo::Init();
	try {
		edaf80::Assignment2 assignment2;
		assignment2.run();
	} catch (std::runtime_error const& e) {
		LogError(e.what());
	}
	Bonobo::Destroy();
}
