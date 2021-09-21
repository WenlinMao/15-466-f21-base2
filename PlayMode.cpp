#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint hexapod_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("city3.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > hexapod_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("city3.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = hexapod_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

PlayMode::PlayMode() : scene(*hexapod_scene) {
	//get pointers to leg for convenience:
	for (auto &transform : scene.transforms) {
		if (transform.name == "Car Body") car_body = &transform;
		else if (transform.name == "wall1") walls.push_back(Wall(FaceDir::negY, 3.5f, &transform));
		else if (transform.name == "wall2.000") walls.push_back(Wall(FaceDir::posY, 20.f, &transform));
		else if (transform.name == "wall2.001") walls.push_back(Wall(FaceDir::negX, 20.f, &transform));
		else if (transform.name == "wall2.002") walls.push_back(Wall(FaceDir::posX, 0.8f, &transform));
		else if (transform.name == "wall2.003") walls.push_back(Wall(FaceDir::posY, 1.f, &transform));
		else if (transform.name == "wall2.004") walls.push_back(Wall(FaceDir::posX, 0.5f, &transform));
		else if (transform.name == "wall2.005") walls.push_back(Wall(FaceDir::posY, 1.5f, &transform));
		else if (transform.name == "wall2.006") walls.push_back(Wall(FaceDir::negY, 0.5f, &transform));
		else if (transform.name == "wall2.007") walls.push_back(Wall(FaceDir::negY, 0.2f, &transform));
		else if (transform.name == "wall2.008") walls.push_back(Wall(FaceDir::posX, 0.3f, &transform));
		else if (transform.name == "wall2.009") walls.push_back(Wall(FaceDir::negY, 3.f, &transform));
		else if (transform.name == "wall2.010") walls.push_back(Wall(FaceDir::posX, 20.f, &transform));
		else if (transform.name == "wall2.012") walls.push_back(Wall(FaceDir::posX, 0.8f, &transform));
		else if (transform.name == "wall2.013") walls.push_back(Wall(FaceDir::negX, 1.f, &transform));
		else if (transform.name == "wall2.011") {
			parkingWall = Wall(FaceDir::negY, 2.f, &transform);
			walls.push_back(parkingWall);
		}
		else if (transform.name == "Parking") parking = &transform;
	}
	if (car_body == nullptr) throw std::runtime_error("Car body not found.");

	car_body_base_rotation = car_body->rotation;

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	} else if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			camera->transform->rotation = glm::normalize(
				camera->transform->rotation
				* glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f))
				* glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
			);
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {
	if (isGameOver) return;

	// hit wall algorithm
	auto hitWalls = [this](glm::vec3 & pos){
		// min_x, max_x
		// min_y, max_y
		// min_z, max_z
		AABB posAABB (pos.x - carSize.x, pos.x + carSize.x, 
			pos.y - carSize.y, pos.y + carSize.y);
		
		bool ret = false;
		for (Wall& w : walls){
			ret |= w.bbox.isIntersect(posAABB);
		}
		return ret;
	};

	auto isParked = [this](glm::vec3 & pos) {
		AABB posAABB (pos.x - carSize.x, pos.x + carSize.x, 
			pos.y - carSize.y, pos.y + carSize.y);
		// AABB parkingBBox (parking->position.x - parkingSize.x, 
		// 	parking->position.x + parkingSize.x, 
		// 	parking->position.y - parkingSize.y,
		// 	parking->position.y + parkingSize.y);
		// std::cout << parkingWall.bbox.isIntersect(posAABB) << std::endl;
		// return parkingBBox.isContain(posAABB);

		return parkingWall.bbox.isIntersect(posAABB);

	};

	// move car body
	{
		//combine inputs into a move:
		constexpr float PlayerSpeed = 7.0f;
		constexpr float RotateAngle = 0.01f;
		float move = 0.f;
		float angle = 0.f;
		// move forward and change direction
		if (left.pressed 
			&& !right.pressed 
			&& up.pressed) angle = -RotateAngle;
		if (!left.pressed 
			&& right.pressed 
			&& up.pressed) angle = RotateAngle;

		// move backward and change direction in opposite
		if (left.pressed 
			&& !right.pressed 
			&& down.pressed) angle = RotateAngle;
		if (!left.pressed 
			&& right.pressed 
			&& down.pressed) angle = -RotateAngle;
		if (down.pressed && !up.pressed) move = -PlayerSpeed * elapsed;
		if (!down.pressed && up.pressed) move = PlayerSpeed * elapsed;

		glm::mat4x3 frame = car_body->make_local_to_parent();
		// glm::vec3 right = frame[0];
		glm::vec3 up = -frame[1];
		// glm::vec3 forward = -frame[2];

		glm::vec3 newPos = car_body->position + move * up;

		if (isParked(newPos)){
			isGameOver = true;
		}

		if (!hitWalls(newPos)) {
			car_body->position = newPos;
			car_body->rotation = glm::normalize(
				car_body->rotation
				* glm::angleAxis(-angle, glm::vec3(0.0f, 0.0f, 1.0f))
			);
		}
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	GL_ERRORS(); //print any errors produced by this setup code

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		if (isGameOver) {
			lines.draw_text("Success!",
				glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0x00, 0x00, 0x00));
			float ofs = 2.0f / drawable_size.y;
			lines.draw_text("Success!",
				glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		} else {
			lines.draw_text("Mouse motion rotates camera; WASD moves car; escape ungrabs mouse",
				glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));
			float ofs = 2.0f / drawable_size.y;
			lines.draw_text("Mouse motion rotates camera; WASD moves car; escape ungrabs mouse",
				glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		}

		
	}
}
