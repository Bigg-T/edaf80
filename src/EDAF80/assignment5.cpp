#include "assignment5.hpp"

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
#include "core/Bonobo.h"

#include "external/glad/glad.h"
#include "parametric_shapes.hpp"
#include "interpolation.hpp"
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <glm/src/glm/glm/gtc/type_ptr.hpp>
#include <core/node.hpp>
#include <stack>


enum class polygon_mode_t : unsigned int {
    fill = 0u,
    line,
    point
};

static polygon_mode_t get_next_mode(polygon_mode_t mode)
{
    return static_cast<polygon_mode_t>((static_cast<unsigned int>(mode) + 1u) % 3u);
}

edaf80::Assignment5::Assignment5()
{
	Log::View::Init();

	window = Window::Create("EDAF80: Assignment 5", config::resolution_x,
	                        config::resolution_y, config::msaa_rate, false);
	if (window == nullptr) {
		Log::View::Destroy();
		throw std::runtime_error("Failed to get a window: aborting!");
	}
	inputHandler = new InputHandler();
	window->SetInputHandler(inputHandler);
}

edaf80::Assignment5::~Assignment5()
{
	delete inputHandler;
	inputHandler = nullptr;

	Window::Destroy(window);
	window = nullptr;

	Log::View::Destroy();
}

void
edaf80::Assignment5::run()
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
    // Create the shader program
    auto def_shader = bonobo::createProgram("default.vert", "default.frag");
    if (def_shader == 0u) {
        LogError("Failed to load shader");
        return;
    }

	GLuint diffuse_shader = 0u, normal_shader = 0u, texcoord_shader = 0u, phong_shader = 0u, cube_shader = 0u, bump_shader = 0u;
	auto const reload_shaders = [&diffuse_shader,&normal_shader,&texcoord_shader, &phong_shader, &cube_shader, &bump_shader](){
		if (diffuse_shader != 0u)
			glDeleteProgram(diffuse_shader);
		diffuse_shader = bonobo::createProgram("diffuse.vert", "diffuse.frag");
		if (diffuse_shader == 0u)
			LogError("Failed to load diffuse shader");

		if (normal_shader != 0u)
			glDeleteProgram(normal_shader);
		normal_shader = bonobo::createProgram("normal.vert", "normal.frag");
		if (normal_shader == 0u)
			LogError("Failed to load normal shader");

		if (texcoord_shader != 0u)
			glDeleteProgram(texcoord_shader);
		texcoord_shader = bonobo::createProgram("texcoord.vert", "texcoord.frag");
		if (texcoord_shader == 0u)
			LogError("Failed to load texcoord shader");

		if (phong_shader != 0u)
			glDeleteProgram(phong_shader);
		phong_shader = bonobo::createProgram("phong.vert", "phong.frag");
		if (phong_shader == 0u)
			LogError("Failed to load phong shader");

		if (cube_shader != 0u)
			glDeleteProgram(cube_shader);
		cube_shader = bonobo::createProgram("cubemap.vert", "cubemap.frag");
		if (cube_shader == 0u)
			LogError("Failed to load cube map shader");

		if (bump_shader != 0u)
			glDeleteProgram(bump_shader);
		bump_shader = bonobo::createProgram("bumpmap.vert", "bumpmap.frag");
		if (bump_shader == 0u)
			LogError("Failed to load cube map shader");

	};
	reload_shaders();

	auto light_position = glm::vec3(-2.0f, 4.0f, 2.0f);
	auto const set_uniforms = [&light_position](GLuint program){
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
	};

	// ***Phong Shader
	auto camera_position = mCamera.mWorld.GetTranslation();
	auto ambient = glm::vec3(0.2f, 0.2f, 0.2f);
	auto diffuse = glm::vec3(0.7f, 0.2f, 0.4f);
	auto specular = glm::vec3(1.0f, 1.0f, 1.0f);
	auto shininess = 1.0f;
	auto const phong_set_uniforms = [&light_position,&camera_position,&ambient,&diffuse,&specular,&shininess](GLuint program){
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
		glUniform3fv(glGetUniformLocation(program, "ambient"), 1, glm::value_ptr(ambient));
		glUniform3fv(glGetUniformLocation(program, "diffuse"), 1, glm::value_ptr(diffuse));
		glUniform3fv(glGetUniformLocation(program, "specular"), 1, glm::value_ptr(specular));
		glUniform1f(glGetUniformLocation(program, "shininess"), shininess);
	};

	//***Cube Map Shader
	auto my_cube_map_id = bonobo::loadTextureCubeMap("sunset_sky/posx.png","sunset_sky/negx.png",
													 "sunset_sky/posy.png","sunset_sky/negy.png",
													 "sunset_sky/posz.png","sunset_sky/negz.png", true);
    auto my_cube_map_id2 = bonobo::loadTextureCubeMap("space.png","space.png",
                                                     "space.png","space.png",
                                                     "space.png","space.png", true);
    auto my_cube_map_id3 = bonobo::loadTextureCubeMap("northlight.png","northlight.png",
                                                      "northlight.png","northlight.png",
                                                      "northlight.png","northlight.png", true);



	auto const cube_set_uniforms = [&camera_position](GLuint program) {
		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
	};

	//***Bump Map Shader
	auto my_bump_map_id = bonobo::loadTexture2D("earth_bump.png", true);
    auto bullet_bump_map = bonobo::loadTexture2D("red.png", true);
    auto exploded_bump_map = bonobo::loadTexture2D("exploded.png", true);
    auto exploded2_bump_map = bonobo::loadTexture2D("exploded2.png", true);

	auto my_bump_map_id2 = bonobo::loadTexture2D("earth_diffuse.png", true);

	auto const bump_set_uniforms = [&light_position,&camera_position,&ambient,&diffuse,&specular,&shininess](GLuint program) {
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
		glUniform3fv(glGetUniformLocation(program, "ambient"), 1, glm::value_ptr(ambient));
		glUniform3fv(glGetUniformLocation(program, "diffuse"), 1, glm::value_ptr(diffuse));
		glUniform3fv(glGetUniformLocation(program, "specular"), 1, glm::value_ptr(specular));
		glUniform1f(glGetUniformLocation(program, "shininess"), shininess);
	};

    // Set the default tensions value; it can always be changed at runtime
    // through the "Scene Controls" window.
    float catmull_rom_tension = 0.0f;

    // Set whether the default interpolation algorithm should be the linear one;
    // it can always be changed at runtime through the "Scene Controls" window.
    bool use_linear = true;


    //
	// Todo: Load your geometry
	//

    auto s43_text = bonobo::loadTexture2D("stone43_bump.png", true);
    auto s43_dif = bonobo::loadTexture2D("stone43_diffuse.png", true);

    auto s47_text = bonobo::loadTexture2D("stone47_bump.png",true);
    auto s47_dif = bonobo::loadTexture2D("stone47_diffuse.png", true);

    auto fs_text = bonobo::loadTexture2D("fieldstone_bump.png", true);
    auto fs_dif = bonobo::loadTexture2D("fieldstone_bump.png", true);

    GLuint text[8] = {my_bump_map_id, my_bump_map_id2, s43_text, s43_dif, s47_text, s47_dif, fs_text, fs_dif};

    GLuint texture_type;
    GLuint diffuse_type;

    //Load asteroids
	auto ast_par = Node();
    int ast_num = 20;
    Node asteroids[ast_num];
    for (int i = 1; i <= ast_num; i++) {
        auto radius = 1.0f;
        auto ast = Node(radius);
        auto const s = parametric_shapes::createSphere(6u, 7u, radius);
        ast.set_geometry(s);
        ast.set_program(bump_shader, set_uniforms);
        auto scale = static_cast<float >(rand() % 1 + 0.7) + 0.5f;
        ast.set_scaling(glm::vec3(scale));
        //todo randomize texture for variety of texture

        switch (rand() % 3) {
            case 0:
                texture_type = text[0];
                diffuse_type = text[1];
                break;
            case 1:
                texture_type = text[2];
                diffuse_type = text[3];
                break;
            case 2:
                texture_type = text[4];
                diffuse_type = text[5];
                break;
            default:
                texture_type = text[6];
                diffuse_type = text[7];
                break;

        }
        ast.add_texture("my_normal_map", texture_type, GL_TEXTURE_2D);
        ast.add_texture("my_diffuse", diffuse_type, GL_TEXTURE_2D);


        asteroids[i-1] = ast;
        ast_par.add_child(&ast);
    }

    //setting the planets coords
    for (int i = 1; i <= ast_num; i++ ) {
        float x = static_cast<float >(std::floor(i));
        printf("ran here\n");
        float yCoord = static_cast<float >(rand() % 30 + 10);
        float xCoord = static_cast<float >(rand() % 20 - 10);
        asteroids[i - 1].set_translation(glm::vec3(xCoord, yCoord, 0.0f));
    }


    // Creating bullets
    auto bullet_par = Node();
    int bullet_num = 10;
    Node bullets[bullet_num];
    int isBulletBusy[bullet_num];  // 0 is not, 1 is busy (meaning render bullet cor to that pos)
    for (int i = 1; i <= bullet_num; i++) {
        auto radius = 1.0f;
        auto bullet = Node(radius);
        auto const s = parametric_shapes::createSphere(6u, 7u, radius);
        bullet.set_geometry(s);
        bullet.set_program(def_shader, [](GLuint /*program*/){});

        bullet.add_texture("diffuse_texture", bullet_bump_map, GL_TEXTURE_2D);
        bullet.set_scaling(glm::vec3(0.2));

        bullets[i-1] = bullet;
        isBulletBusy[i-1] = 0;
        bullet_par.add_child(&bullet);

    }

    //explosion

    auto radius = 1.0f;
    auto explosion = Node(radius);
    auto const s = parametric_shapes::createSphere(30u, 20u, radius);
    explosion.set_geometry(s);
    explosion.set_program(def_shader, [](GLuint /*program*/){});
//    explosion.add_texture("diffuse_texture", exploded_bump_map, GL_TEXTURE_2D);
    explosion.add_texture("diffuse_texture", exploded2_bump_map, GL_TEXTURE_2D);
    explosion.set_scaling(glm::vec3(0.8));
    explosion.set_translation(glm::vec3(0,20,0));


	// The cube map, background -> sdf
	auto cube_bg = Node();
	auto const s2 = parametric_shapes::createSphere(60u, 70u, 5.0f);
	cube_bg.set_geometry(s2);
	cube_bg.set_program(cube_shader, cube_set_uniforms);
	cube_bg.set_scaling(glm::vec3(10));

	cube_bg.add_texture("my_cube_map", my_cube_map_id2, GL_TEXTURE_CUBE_MAP);

	// the spaceship
	// Load the ship geometry
	auto const obj_ship = bonobo::loadObjects("spaceship.obj");
	if (obj_ship.empty())
		return;
	auto const& geo_ship = obj_ship.front();
    //create a ship
	Node ship = Node();
	ship.set_geometry(geo_ship);
	ship.set_program(def_shader, [](GLuint /*program*/){});
	ship.set_scaling(glm::vec3(.8));
    ship.set_translation(glm::vec3(0,-4,0));

    // Load the sun's texture
    auto ship_texture = bonobo::loadTexture2D("metal-crate.png");
    ship.add_texture("diffuse_texture", ship_texture, GL_TEXTURE_2D); //add ship texture



    // ADD Everything to world Scene
    // Add node
    auto world = Node();
//    world.add_child(&ast_par);


	glEnable(GL_DEPTH_TEST);

	// Enable face culling to improve performance:
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);
	//glCullFace(GL_BACK);

	f64 ddeltatime2;
	size_t fpsSamples2 = 0;
	double nowTime2, lastTime2 = GetTimeSeconds();
	double fpsNextTick2 = lastTime2 + 1.0;



	f64 ddeltatime;
	size_t fpsSamples = 0;
	double nowTime, lastTime = GetTimeMilliseconds();
	double fpsNextTick = lastTime + 1000.0;

    int my_score = 0;
    int my_lives = 3;

    float l_inter = 0.0f;
	while (!glfwWindowShouldClose(window->GetGLFW_Window())) {
		nowTime = GetTimeMilliseconds();
		ddeltatime = nowTime - lastTime;
		if (nowTime > fpsNextTick) {
			fpsNextTick += 1000.0;
			fpsSamples = 0;
		}
		fpsSamples++;


		nowTime2 = GetTimeSeconds();
		ddeltatime2 = nowTime2 - lastTime2;
		if (nowTime2 > fpsNextTick2) {
			fpsNextTick2 += 1.0;
			fpsSamples2 = 0;
		}
		fpsSamples2++;

		auto& io = ImGui::GetIO();
		inputHandler->SetUICapture(io.WantCaptureMouse, io.WantCaptureMouse);

		glfwPollEvents();
		inputHandler->Advance();
		cube_bg.set_translation(mCamera.mWorld.GetTranslation()); // Use the

		mCamera.Update(ddeltatime, *inputHandler);

		ImGui_ImplGlfwGL3_NewFrame();


        auto polygon_mode = polygon_mode_t::fill;

		//
		// Todo: If you need to handle inputs, you can do it here
		//
        if (inputHandler->GetKeycodeState(GLFW_KEY_Z) & JUST_PRESSED) {
            polygon_mode = get_next_mode(polygon_mode);
        }
		if (inputHandler->GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED) {
			reload_shaders();
		}
        if (inputHandler->GetKeycodeState(GLFW_KEY_SPACE) & JUST_PRESSED) {
            for (int i = 0; i < bullet_num; i++) {
                if (isBulletBusy[i] == 0) {
                    isBulletBusy[i] = 1;
                    bullets[i].render(mCamera.GetWorldToClipMatrix(), bullets[i].get_transform());
                    printf("bullet number %d \n", i);
                    break;
                }
            }
        }
        if (inputHandler->GetKeycodeState(GLFW_KEY_V) & JUST_PRESSED) {
            //  reset all bullet to allow for use
            for (int i = 0; i < bullet_num; i++) {
                isBulletBusy[i] = 0;
                bullets[i].set_translation(ship.get_translation());
            }
        }
        if (inputHandler->GetKeycodeState(GLFW_KEY_C) & JUST_PRESSED) {
            ship.translate(glm::vec3(1,0,0));
        }
        if (inputHandler->GetKeycodeState(GLFW_KEY_X) & JUST_PRESSED) {
            ship.translate(glm::vec3(-1,0,0));
        }
        if (inputHandler->GetKeycodeState(GLFW_KEY_B) & JUST_PRESSED) {
            //setting the planets coords
            for (int i = 1; i <= ast_num; i++ ) {
                float x = static_cast<float >(std::floor(i));
                printf("ran here\n");
                float yCoord = static_cast<float >(rand() % 30 + 10);
                float xCoord = static_cast<float >(rand() % 20 - 10);
                asteroids[i - 1].set_translation(glm::vec3(xCoord, yCoord, 0.0f));
            }
            ship.set_translation(glm::vec3(0, -4, 0));
        }

        // fire bullet,
        for (int i = 0; i < bullet_num; i ++) {
            if (isBulletBusy[i] == 1) {
                bullets[i].translate(glm::vec3(0,1,0));
            }
        }

        //reset bullet when pass
        for (int i = 0; i < bullet_num; i++) {
            if (bullets[i].get_translation().y > 20) {
                isBulletBusy[i] = 0;
                bullets[i].set_translation(ship.get_translation());
            }
        }

        //bullets follow ship
        for (int i = 0; i < bullet_num; i++) {
            if (isBulletBusy[i] == 0) {
                bullets[i].set_translation(ship.get_translation());
            } else {
                if (bullets[i].get_translation().y > 20) {
                    isBulletBusy[i] = 0;
                    bullets[i].set_translation(ship.get_translation());
                }
            }
        }

        float fall_speed = 0.05f;
        //fall of solar systems
        for (int i = 0; i < ast_num; i++ ) {
            asteroids[i].rotate_x(nowTime/(nowTime*20));
            asteroids[i].rotate_y(nowTime/(nowTime*10));
            asteroids[i].translate(glm::vec3(0,-fall_speed,0));
        }

		//reset planets that fall below y < -3
        for (int i = 0; i < ast_num; i++) {
            float yCoord = static_cast<float >(rand() % 100 + 50);
            auto ast = asteroids[i];
            if (ast.get_translation().y < -3) {
                asteroids[i].set_translation(glm::vec3(asteroids[i].get_translation().x, yCoord, 0));
                my_score -= 1;
            }
        }

        //detection bullet colliding with falling objects
        for (int i = 0; i < bullet_num; i++) {
            if (isBulletBusy[i] == 1) {
                auto bx = bullets[i].get_translation().x;
                auto by = bullets[i].get_translation().y;
                auto br = bullets[i]._r;
                for (int j = 0; j < ast_num; j++) {
                    auto ax = asteroids[j].get_translation().x;
                    auto ay = asteroids[j].get_translation().y;
                    auto ar = asteroids[j]._r;
                    auto collidedRadius = ar + br;
                    if ( std::abs(ax - bx) < collidedRadius && std::abs(ay - by) < collidedRadius ) {
                        float yCoord = static_cast<float >(rand() % 100 + 50);

                        explosion.set_translation(asteroids[j].get_translation());
                        //reset the asteroids
                        asteroids[j].set_translation(glm::vec3(asteroids[j].get_translation().x, yCoord, 0));

                        // reset the bullet
                        bullets[i].set_translation(ship.get_translation());
                        isBulletBusy[i] = 0;

                        my_score += 1;
                    }
                }
            }
        }
//        explosion.set_translation(glm::vec3(0,20,0));




        // detection for the jet and asteroids ------
        auto sx = ship.get_translation().x;
        auto sy = ship.get_translation().y;
        auto sr = 1.5;
        for (int j = 0; j < ast_num; j++) {

            auto ax = asteroids[j].get_translation().x;
            auto ay = asteroids[j].get_translation().y;
            auto ar = asteroids[j]._r;
            auto collidedRadius = ar + sr;
            if (std::abs(ax - sx) < collidedRadius && std::abs(ay - sy) < collidedRadius ) {
                my_lives -= 1;
                ship.set_translation(glm::vec3(-100, 30, 0));
            }

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


        // Bullet logic
        /*
         * Bullet does hit target -> move it back to plane coord
         * Bullet goes beyond a threshold -> move it back
         *
         */


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



        if (l_inter > 10) {
            l_inter -= 10;
        } else {

            l_inter += ddeltatime*100;

        }


//        int in = static_cast<int>(std::floor(l_inter));
//        if (use_linear) {
//            glm::vec3 interp_me = interpolation::evalLERP(cp[in % 10], cp[(in + 1) % 10], l_inter - std::floor(l_inter));
//            earth.set_translation(interp_me);
//        } else {
//            glm::vec3 interp_me = interpolation::evalCatmullRom(cp[in % 10], cp[(in + 1) % 10], cp[(in + 2) % 10], cp[(in + 3) % 10], catmull_rom_tension, l_inter - std::floor(l_inter));
//            earth.set_translation(interp_me);
//        }

		//
		// Todo: Render all your geometry here.
		//




		cube_bg.render(mCamera.GetWorldToClipMatrix(), cube_bg.get_transform());
		ship.render(mCamera.GetWorldToClipMatrix(), ship.get_transform());
        for (int i = 0; i < ast_num; i++) {
            asteroids[i].render(mCamera.GetWorldToClipMatrix(), asteroids[i].get_transform());
        }

        for (int i = 0; i < bullet_num; i++) {
            bullets[i].render(mCamera.GetWorldToClipMatrix(), bullets[i].get_transform());
        }

        explosion.render(mCamera.GetWorldToClipMatrix(), explosion.get_transform());
        //bullets[0].render(mCamera.GetWorldToClipMatrix(), bullets[0].get_transform());
        //
		// Todo: If you want a custom ImGUI window, you can set it up
		//       here
		//


//        bool const opened = ImGui::Begin("Scene Controls", nullptr, ImVec2(300, 100), -1.0f, 0);
//        if (opened) {
//            ImGui::SliderFloat("Catmull-Rom tension", &catmull_rom_tension, 0.0f, 1.0f);
//            ImGui::Checkbox("Use linear interpolation", &use_linear);
//        }
//        ImGui::End();

        bool const opened = ImGui::Begin("Game Controls", nullptr, ImVec2(300, 100), -1.0f, 0);
        if (opened) {
//            ImGui::Checkbox("Use linear interpolation", &use_linear);

//            char * text = (char) std::to_string(my_score);


            //speed of asteroid
            ImGui::SliderFloat("Falling Speed", &fall_speed, 0.05f, 1.5f);
            // score board
            std::string s = "My Score " + std::to_string(my_score);
            char const *score = s.c_str();
            ImGui::Checkbox(score, &use_linear);

            // lives left
            std::string l = "My Lives: " + std::to_string(my_lives);
            char const *life = l.c_str();
            ImGui::Checkbox(life, &use_linear);
        }
        ImGui::End();

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        Log::View::Render();
        ImGui::Render();

		window->Swap();
		lastTime = nowTime;
	}

	//
	// Todo: Do not forget to delete your shader programs, by calling
	//       `glDeleteProgram($your_shader_program)` for each of them.
	//
	glDeleteProgram(texcoord_shader);
	texcoord_shader = 0u;
	glDeleteProgram(normal_shader);
	normal_shader = 0u;
	glDeleteProgram(diffuse_shader);
	diffuse_shader = 0u;
	glDeleteProgram(fallback_shader);
	diffuse_shader = 0u;
	glDeleteProgram(phong_shader);
	phong_shader = 0u;
	glDeleteProgram(cube_shader);
	cube_shader = 0u;
	glDeleteProgram(bump_shader);
	bump_shader = 0u;
    glDeleteProgram(def_shader);
    def_shader = 0u;

}

int main()
{
	Bonobo::Init();
	try {
		edaf80::Assignment5 assignment5;
		assignment5.run();
	} catch (std::runtime_error const& e) {
		LogError(e.what());
	}
	Bonobo::Destroy();
}
