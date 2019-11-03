#include "../../SDK/Vector.h"
#include "../../Utils/Color.h"
#include <vector>
#include "../../SDK/singleton.h"
class IGameEvent;

class bullettracers : public singleton< bullettracers > {
private:
	class trace_info {
	public:
		trace_info(Vector starts, Vector positions, float times) {
			this->start = starts;
			this->position = positions;
			this->time = times;
		}

		Vector position;
		Vector start;
		float time;
	};

	std::vector<trace_info> logs;

	void draw_beam(Vector src, Vector end, Color color);
public:
	void events(IGameEvent* event);
};