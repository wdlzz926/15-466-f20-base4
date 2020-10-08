#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"
#include "DrawText.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----
	int relation = 0;
	int skill = 0;
	int study = 0;
	std::vector<std::vector<std::string>> story;
	std::vector<std::vector<std::string>> choice;
	int step = 0;
	bool change = true;
	
	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;
	bool choice_made = false;
	bool end = false;

	//music coming from the tip of the leg (as a demonstration):
	std::shared_ptr< Sound::PlayingSample > leg_tip_loop;
	
	//camera:
	Scene::Camera *camera = nullptr;

};
