#include "Mode.hpp"

#include "Scene.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

enum class FaceDir {
	None,
	posX,
	negX,
	posY,
	negY
};

struct AABB {
	float minX;
	float maxX;
	float minY;
	float maxY;
	AABB() : minX(0.f), maxX(0.f), minY(0.f), maxY(0.f){}
	AABB(float minX, float maxX, float minY, float maxY) 
		: minX(minX), maxX(maxX), minY(minY), maxY(maxY){}

	bool isIntersect(const AABB& in) {
		return (in.minX <= this->maxX && in.maxX >= this->minX) &&
         	(in.minY <= this->maxY && in.maxY >= this->minY);
	}

	bool isContain(const AABB& in) {
		return (in.minX >= this->minX && in.maxX <= this->maxX) &&
         	(in.minY >= this->minY && in.maxY <= this->maxY);
	}
};

struct Wall {
	Wall(): faceDir(FaceDir::None), length(0.f), transform(nullptr) {}
	
	Wall(FaceDir dir, float length, Scene::Transform* ts)
		: faceDir(dir), length(length), transform(ts) {
		glm::vec3 pos = ts->position;

		if (dir == FaceDir::posX || dir == FaceDir::negX){
			bbox = AABB(
				pos.x - this->width, pos.x + this->width, 
				pos.y - this->length, pos.y + this->length);
		} else if (dir == FaceDir::posY || dir == FaceDir::negY) {
			bbox = AABB(
				pos.x - this->length, pos.x + this->length, 
				pos.y - this->width, pos.y + this->width);
		}
	}

	FaceDir faceDir = FaceDir::None;
	float width = 0.01f;
	float length = 0.f;
	Scene::Transform* transform = nullptr;
	AABB bbox = AABB();
};

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	Scene::Transform * car_body = nullptr;
	glm::vec3 carSize = glm::vec3(0.9f, 0.9f, 0.9f);

	Scene::Transform * parking = nullptr;
	glm::vec2 parkingSize = glm::vec2(2.5f, 2.5f);
	Wall parkingWall;

	glm::quat car_body_base_rotation;
	std::vector<Wall> walls;

	bool isGameOver = false;
	
	//camera:
	Scene::Camera *camera = nullptr;

};
