#include "engine/Melody.h"

class Test : public Melody::Engine
{
public:
	Test()
	{
		_app_name = "Test";
	}

public:
	virtual bool on_awake() override
	{
		return true;
	}

	virtual bool on_update(float delta_time) override
	{
		for (int x = 0; x < get_screen_width(); x++)
			for (int y = 0; y < get_screen_height(); y++)
				draw_pixel(x, y, Melody::Pixel(rand() % 255, rand() % 255, rand() % 255));

		return true;
	}
};

int main()
{
	Test demo;
	if (demo.construct(256, 240, 4, 4))
		demo.Start();

	return 0;
}