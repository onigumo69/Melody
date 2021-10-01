#include "Melody.h"

#include <GL/gl.h>
typedef BOOL(WINAPI wglSwapInterval_t) (int interval);
wglSwapInterval_t* wglSwapInterval;

namespace Melody
{
	Pixel::Pixel()
	{
		r = 0;
		g = 0;
		b = 0;
		a = 255;
	}

	Pixel::Pixel(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
	{
		r = red;
		g = green;
		b = blue;
		a = alpha;
	}

	Sprite::Sprite()
	{
		_width = 0;
		_height = 0;
		_color_data = nullptr;
	}

	Sprite::Sprite(std::string image_file)
	{
		load_from_file(image_file);
	}

	Sprite::Sprite(int32_t w, int32_t h)
	{
		if (_color_data)
			delete[] _color_data;

		_width = w;
		_height = h;
		_color_data = new Pixel[_width * _height];

		for (int32_t i = 0; i > _width * _height; i++)
			_color_data[i] = Pixel();
	}

	Sprite::~Sprite()
	{
		if (_color_data)
			delete[] _color_data;
	}

	ReturnCode Sprite::load_from_file(std::string image_file)
	{
		std::wstring ws_image_file;

		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		ws_image_file = converter.from_bytes(image_file);

		Gdiplus::Bitmap* bmp = Gdiplus::Bitmap::FromFile(ws_image_file.c_str());

		if (bmp == nullptr)
			return ReturnCode::NO_FILE;

		_width = bmp->GetWidth();
		_height = bmp->GetHeight();
		_color_data = new Pixel[_width * _height];

		for (int x = 0; x < _width; x++)
		{
			for (int y = 0; y < _height; y++)
			{
				Gdiplus::Color c;
				bmp->GetPixel(x, y, &c);
				set_pixel(x, y, Pixel(c.GetRed(), c.GetGreen(), c.GetBlue(), c.GetAlpha()));
			}
		}
		delete bmp;
		return ReturnCode::OK;
	}

	Pixel Sprite::get_pixel(int32_t x, int32_t y) const
	{
		if (x >= 0 && x < _width && y >= 0 && y < _height)
			return _color_data[y * _width + x];
		else
			return Pixel();
	}

	void Sprite::set_pixel(int32_t x, int32_t y, Pixel p)
	{
		if (x >= 0 && x < _width && y >= 0 && y < _height)
			_color_data[y * _width + x] = p;
	}

	Pixel Sprite::sample(float x, float y) const
	{
		int32_t sx = (int32_t)(x * (float)_width);
		int32_t sy = (int32_t)(y * (float)_height);
		return get_pixel(sx, sy);
	}

	Pixel* Sprite::get_data() const
	{
		return _color_data;
	}

	Engine::Engine()
	{
		_app_name = "Melody";
	}

	ReturnCode Engine::construct(uint32_t screen_w, uint32_t screen_h, uint32_t pixel_w, uint32_t pixel_h)
	{
		_screen_width = screen_w;
		_screen_height = screen_h;
		_pixel_width = pixel_w;
		_pixel_height = pixel_h;

		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		_window_name = converter.from_bytes(_app_name);

		_default_drawing_target = new Sprite(_screen_width, _screen_height);
		set_drawing_target(nullptr);
		return ReturnCode::OK;
	}

	ReturnCode Engine::Start()
	{
		if (!create_window())
			return ReturnCode::FAIL;

		ULONG_PTR token;
		Gdiplus::GdiplusStartupInput startup_input;
		Gdiplus::GdiplusStartup(&token, &startup_input, NULL);

		_atom_active = true;
		std::thread t = std::thread(&Engine::threading, this);

		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0) > 0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		t.join();
		return ReturnCode::OK;
	}

	void Engine::set_drawing_target(Sprite* target)
	{
		if (target)
			_drawing_target = target;
		else
			_drawing_target = _default_drawing_target;
	}

	Sprite* Engine::get_drawing_target()
	{
		return _drawing_target;
	}

	int32_t Engine::get_drawing_target_width() const
	{
		if (_drawing_target)
			return _drawing_target->_width;
		else
			return 0;
	}

	int32_t Engine::get_drawing_target_height() const
	{
		if (_drawing_target)
			return _drawing_target->_height;
		else
			return 0;
	}

	bool Engine::is_focused() const
	{
		return _has_input_focus;
	}

	ButtonState Engine::get_key(KeyCode key) const
	{
		return _keyboard_state[key];
	}

	ButtonState Engine::get_mouse(char button) const
	{
		return _mouse_state[button];
	}

	int32_t Engine::get_mouse_x() const
	{
		return _mouse_pos_x;
	}

	int32_t Engine::get_mouse_y() const
	{
		return _mouse_pos_y;
	}

	int32_t Engine::get_screen_width() const
	{
		return _screen_width;
	}

	int32_t Engine::get_screen_height() const
	{
		return _screen_height;
	}

	void Engine::draw_pixel(int32_t x, int32_t y, Pixel p)
	{
		if (!_drawing_target)
			return;

		if (_pixel_mode == Pixel::Mode::NORMAL)
		{
			_drawing_target->set_pixel(x, y, p);
			return;
		}

		if (_pixel_mode == Pixel::Mode::MASK)
		{
			if (p.a != 255)
				_drawing_target->set_pixel(x, y, p);
			return;
		}

		if (_pixel_mode == Pixel::Mode::ALPHA)
		{
			Pixel d = _drawing_target->get_pixel(x, y);
			float a = (float)p.a / 255.0f;
			float c = 1.0f - a;
			float r = a * (float)p.r + c * (float)d.r;
			float g = a * (float)p.g + c * (float)d.g;
			float b = a * (float)p.b + c * (float)d.b;
			_drawing_target->set_pixel(x, y, Pixel((uint8_t)r, (uint8_t)g, (uint8_t)b));
			return;
		}
	}

	void Engine::draw_line(int32_t x1, int32_t y1, int32_t x2, int32_t y2, Pixel p)
	{
		int x, y, dx, dy, dx1, dy1, px, py, xe, ye, i;
		dx = x2 - x1; dy = y2 - y1;
		dx1 = abs(dx); dy1 = abs(dy);
		px = 2 * dy1 - dx1;	py = 2 * dx1 - dy1;
		if (dy1 <= dx1)
		{
			if (dx >= 0)
			{
				x = x1; y = y1; xe = x2;
			}
			else
			{
				x = x2; y = y2; xe = x1;
			}

			draw_pixel(x, y, p);

			for (i = 0; x < xe; i++)
			{
				x = x + 1;
				if (px < 0)
					px = px + 2 * dy1;
				else
				{
					if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0)) y = y + 1; else y = y - 1;
					px = px + 2 * (dy1 - dx1);
				}
				draw_pixel(x, y, p);
			}
		}
		else
		{
			if (dy >= 0)
			{
				x = x1; y = y1; ye = y2;
			}
			else
			{
				x = x2; y = y2; ye = y1;
			}

			draw_pixel(x, y, p);

			for (i = 0; y < ye; i++)
			{
				y = y + 1;
				if (py <= 0)
					py = py + 2 * dx1;
				else
				{
					if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0)) x = x + 1; else x = x - 1;
					py = py + 2 * (dx1 - dy1);
				}
				draw_pixel(x, y, p);
			}
		}
	}

	void Engine::draw_circle(int32_t x, int32_t y, int32_t radius, Pixel p)
	{
		int x0 = 0;
		int y0 = radius;
		int d = 3 - 2 * radius;
		if (!radius) return;

		while (y0 >= x0) // only formulate 1/8 of circle
		{
			draw_pixel(x - x0, y - y0, p);//upper left left
			draw_pixel(x - y0, y - x0, p);//upper upper left
			draw_pixel(x + y0, y - x0, p);//upper upper right
			draw_pixel(x + x0, y - y0, p);//upper right right
			draw_pixel(x - x0, y + y0, p);//lower left left
			draw_pixel(x - y0, y + x0, p);//lower lower left
			draw_pixel(x + y0, y + x0, p);//lower lower right
			draw_pixel(x + x0, y + y0, p);//lower right right
			if (d < 0) d += 4 * x0++ + 6;
			else d += 4 * (x0++ - y0--) + 10;
		}
	}

	void Engine::fill_circle(int32_t x, int32_t y, int32_t radius, Pixel p)
	{
		// Taken from wikipedia
		int x0 = 0;
		int y0 = radius;
		int d = 3 - 2 * radius;
		if (!radius) return;

		auto drawline = [&](int sx, int ex, int ny)
		{
			for (int i = sx; i <= ex; i++)
				draw_pixel(i, ny, p);
		};

		while (y0 >= x0)
		{
			// Modified to draw scan-lines instead of edges
			drawline(x - x0, x + x0, y - y0);
			drawline(x - y0, x + y0, y - x0);
			drawline(x - x0, x + x0, y + y0);
			drawline(x - y0, x + y0, y + x0);
			if (d < 0) d += 4 * x0++ + 6;
			else d += 4 * (x0++ - y0--) + 10;
		}
	}

	void Engine::draw_rect(int32_t x, int32_t y, int32_t w, int32_t h, Pixel p)
	{
		draw_line(x, y, x + w, y, p);
		draw_line(x + w, y, x + w, y + h, p);
		draw_line(x + w, y + h, x, y + h, p);
		draw_line(x, y + h, x, y, p);
	}

	void Engine::fill_rect(int32_t x, int32_t y, int32_t w, int32_t h, Pixel p)
	{
		int32_t x2 = x + w;
		int32_t y2 = y + h;

		if (x < 0) x = 0;
		if (x >= (int32_t)get_screen_width()) x = (int32_t)get_screen_width();
		if (y < 0) y = 0;
		if (y >= (int32_t)get_screen_height()) y = (int32_t)get_screen_height();

		if (x2 < 0) x2 = 0;
		if (x2 >= (int32_t)get_screen_width()) x2 = (int32_t)get_screen_width();
		if (y2 < 0) y2 = 0;
		if (y2 >= (int32_t)get_screen_height()) y2 = (int32_t)get_screen_height();

		for (int i = x; i < x2; i++)
			for (int j = y; j < y2; j++)
				draw_pixel(i, j, p);
	}

	void Engine::draw_triangle(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x3, int32_t y3, Pixel p)
	{
		draw_line(x1, y1, x2, y2, p);
		draw_line(x2, y2, x3, y3, p);
		draw_line(x3, y3, x1, y1, p);
	}

	void Engine::fill_triangle(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x3, int32_t y3, Pixel p)
	{
		auto SWAP = [](int& x, int& y) { int t = x; x = y; y = t; };
		auto drawline = [&](int sx, int ex, int ny) { for (int i = sx; i <= ex; i++) draw_pixel(i, ny, p); };

		int t1x, t2x, y, minx, maxx, t1xp, t2xp;
		bool changed1 = false;
		bool changed2 = false;
		int signx1, signx2, dx1, dy1, dx2, dy2;
		int e1, e2;
		// Sort vertices
		if (y1 > y2) { SWAP(y1, y2); SWAP(x1, x2); }
		if (y1 > y3) { SWAP(y1, y3); SWAP(x1, x3); }
		if (y2 > y3) { SWAP(y2, y3); SWAP(x2, x3); }

		t1x = t2x = x1; y = y1;   // Starting points
		dx1 = (int)(x2 - x1); if (dx1 < 0) { dx1 = -dx1; signx1 = -1; }
		else signx1 = 1;
		dy1 = (int)(y2 - y1);

		dx2 = (int)(x3 - x1); if (dx2 < 0) { dx2 = -dx2; signx2 = -1; }
		else signx2 = 1;
		dy2 = (int)(y3 - y1);

		if (dy1 > dx1) {   // swap values
			SWAP(dx1, dy1);
			changed1 = true;
		}
		if (dy2 > dx2) {   // swap values
			SWAP(dy2, dx2);
			changed2 = true;
		}

		e2 = (int)(dx2 >> 1);
		// Flat top, just process the second half
		if (y1 == y2) goto next;
		e1 = (int)(dx1 >> 1);

		for (int i = 0; i < dx1;) {
			t1xp = 0; t2xp = 0;
			if (t1x < t2x) { minx = t1x; maxx = t2x; }
			else { minx = t2x; maxx = t1x; }
			// process first line until y value is about to change
			while (i < dx1) {
				i++;
				e1 += dy1;
				while (e1 >= dx1) {
					e1 -= dx1;
					if (changed1) t1xp = signx1;//t1x += signx1;
					else          goto next1;
				}
				if (changed1) break;
				else t1x += signx1;
			}
			// Move line
		next1:
			// process second line until y value is about to change
			while (1) {
				e2 += dy2;
				while (e2 >= dx2) {
					e2 -= dx2;
					if (changed2) t2xp = signx2;//t2x += signx2;
					else          goto next2;
				}
				if (changed2)     break;
				else              t2x += signx2;
			}
		next2:
			if (minx > t1x) minx = t1x; if (minx > t2x) minx = t2x;
			if (maxx < t1x) maxx = t1x; if (maxx < t2x) maxx = t2x;
			drawline(minx, maxx, y);    // Draw line from min to max points found on the y
										// Now increase y
			if (!changed1) t1x += signx1;
			t1x += t1xp;
			if (!changed2) t2x += signx2;
			t2x += t2xp;
			y += 1;
			if (y == y2) break;

		}
	next:
		// Second half
		dx1 = (int)(x3 - x2); if (dx1 < 0) { dx1 = -dx1; signx1 = -1; }
		else signx1 = 1;
		dy1 = (int)(y3 - y2);
		t1x = x2;

		if (dy1 > dx1) {   // swap values
			SWAP(dy1, dx1);
			changed1 = true;
		}
		else changed1 = false;

		e1 = (int)(dx1 >> 1);

		for (int i = 0; i <= dx1; i++) {
			t1xp = 0; t2xp = 0;
			if (t1x < t2x) { minx = t1x; maxx = t2x; }
			else { minx = t2x; maxx = t1x; }
			// process first line until y value is about to change
			while (i < dx1) {
				e1 += dy1;
				while (e1 >= dx1) {
					e1 -= dx1;
					if (changed1) { t1xp = signx1; break; }//t1x += signx1;
					else          goto next3;
				}
				if (changed1) break;
				else   	   	  t1x += signx1;
				if (i < dx1) i++;
			}
		next3:
			// process second line until y value is about to change
			while (t2x != x3) {
				e2 += dy2;
				while (e2 >= dx2) {
					e2 -= dx2;
					if (changed2) t2xp = signx2;
					else          goto next4;
				}
				if (changed2)     break;
				else              t2x += signx2;
			}
		next4:

			if (minx > t1x) minx = t1x; if (minx > t2x) minx = t2x;
			if (maxx < t1x) maxx = t1x; if (maxx < t2x) maxx = t2x;
			drawline(minx, maxx, y);
			if (!changed1) t1x += signx1;
			t1x += t1xp;
			if (!changed2) t2x += signx2;
			t2x += t2xp;
			y += 1;
			if (y > y3) return;
		}
	}

	void Engine::draw_sprite(int32_t x, int32_t y, Sprite* sprite)
	{
		if (sprite == nullptr)
			return;

		for (int i = 0; i < sprite->_width; i++)
		{
			for (int j = 0; j < sprite->_height; j++)
			{
				draw_pixel(x + i, y + j, sprite->get_pixel(i, j));
			}
		}
	}

	void Engine::draw_sprite_partial(int32_t x, int32_t y, Sprite* sprite, int32_t ox, int32_t oy, int32_t w, int32_t h)
	{
		if (sprite = nullptr)
			return;

		for (int i = 0; i < w; i++)
		{
			for (int j = 0; j < h; j++)
			{
				draw_pixel(x + i, y + j, sprite->get_pixel(i + ox, j + oy));
			}
		}
	}

	void Engine::set_pixel_mode(Pixel::Mode mode)
	{
		_pixel_mode = mode;
	}

	bool Engine::on_awake()
	{
		return false;
	}
	bool Engine::on_update(float delta_time)
	{
		return false;
	}
	bool Engine::on_destroy()
	{
		return true;
	}

	void Engine::update_mouse(uint32_t x, uint32_t y)
	{
		// come in screen space
		// leave in pixel space
		_mouse_pos_x = x / _pixel_width;
		_mouse_pos_y = y / _pixel_height;
	}

	void Engine::threading()
	{
		// init opengl, context owned by the game thread
		create_opengl();

		// create screen texture
		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, &_gl_buffer);
		glBindTexture(GL_TEXTURE_2D, _gl_buffer);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

		// load resources
		if (!on_awake())
			_atom_active = false;

		auto time_point1 = std::chrono::system_clock::now();
		auto time_point2 = std::chrono::system_clock::now();

		while (_atom_active)
		{
			// run afap
			while (_atom_active)
			{
				time_point2 = std::chrono::system_clock::now();
				std::chrono::duration<float> elapsed_time = time_point2 - time_point1;
				time_point1 = time_point2;

				float delta_time = elapsed_time.count();

				// keyboard input
				for (int i = 0; i < 256; i++)
				{
					_keyboard_state[i].Pressed = false;
					_keyboard_state[i].Released = false;

					if (_key_new_state[i] != _key_old_state[i])
					{
						if (_key_new_state[i])
						{
							_keyboard_state[i].Pressed = !_keyboard_state[i].Held;
							_keyboard_state[i].Held = true;
						}
						else
						{
							_keyboard_state[i].Released = true;
							_keyboard_state[i].Held = false;
						}
					}

					_key_old_state[i] = _key_new_state[i];
				}

				// mouse input
				for (int i = 0; i < 5; i++)
				{
					_mouse_state[i].Pressed = false;
					_mouse_state[i].Released = false;

					if (_mouse_new_state[i] != _mouse_old_state[i])
					{
						if (_mouse_new_state[i])
						{
							_mouse_state[i].Pressed = !_mouse_state[i].Held;
							_mouse_state[i].Held = true;
						}
						else
						{
							_mouse_state[i].Released = true;
							_mouse_state[i].Held = false;
						}
					}

					_mouse_old_state[i] = _mouse_new_state[i];
				}

				// frame
				if (!on_update(delta_time))
					_atom_active = false;


				// TODO: this is a bit slow (especially in debug, but 100x faster in release mode...)
				// copy pixel array into texture
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, get_screen_width(), get_screen_height(), 0,
					GL_RGBA, GL_UNSIGNED_BYTE, _default_drawing_target->get_data());

				// display texture on screen
				glBegin(GL_QUADS);
				glTexCoord2f(0.0, 1.0); glVertex3f(-1.0f, -1.0f, 0.0f);
				glTexCoord2f(0.0, 0.0); glVertex3f(-1.0f, 1.0f, 0.0f);
				glTexCoord2f(1.0, 0.0); glVertex3f(1.0f, 1.0f, 0.0f);
				glTexCoord2f(1.0, 1.0); glVertex3f(1.0f, -1.0f, 0.0f);
				glEnd();

				// present
				SwapBuffers(_gl_device_context);

				// update title text
				wchar_t title_text[256];
				swprintf(title_text, 256, L"Melody - %s - FPS: %3.2f", _window_name.c_str(), 1.0f / delta_time);
				SetWindowText(_hwnd, title_text);
			}

			if (on_destroy())
			{

			}
			else
			{
				// if abort destroying, continue running
				_atom_active = true;
			}
		}

		wglDeleteContext(_gl_render_context);
		PostMessage(_hwnd, WM_DESTROY, 0, 0);
	}


	HWND Engine::create_window()
	{
		WNDCLASS wc;
		wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		wc.hInstance = GetModuleHandle(nullptr);
		wc.lpfnWndProc = window_event;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.lpszMenuName = nullptr;
		wc.hbrBackground = nullptr;

		wc.lpszClassName = L"MELODY_ENGINE";

		RegisterClass(&wc);

		// windows bs
		DWORD ex_style = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		DWORD style = WS_CAPTION | WS_SYSMENU | WS_VISIBLE;
		RECT window_rect = { 0, 0, (LONG)get_screen_width() * (LONG)_pixel_width,
			(LONG)get_screen_height() * (LONG)_pixel_height };

		// keep client size as requested
		AdjustWindowRectEx(&window_rect, style, FALSE, ex_style);

		int width = window_rect.right - window_rect.left;
		int height = window_rect.bottom - window_rect.top;

		_hwnd = CreateWindowEx(ex_style, wc.lpszClassName, L"", style,
			CW_USEDEFAULT, CW_USEDEFAULT, width, height, NULL, NULL, GetModuleHandle(nullptr), this);

		// create keyboard mapping
		_map_keys[0x41] = KeyCode::A; _map_keys[0x42] = KeyCode::B; _map_keys[0x43] = KeyCode::C; _map_keys[0x44] = KeyCode::D; _map_keys[0x45] = KeyCode::E;
		_map_keys[0x46] = KeyCode::F; _map_keys[0x47] = KeyCode::G; _map_keys[0x48] = KeyCode::H; _map_keys[0x49] = KeyCode::I; _map_keys[0x4A] = KeyCode::J;
		_map_keys[0x4B] = KeyCode::K; _map_keys[0x4C] = KeyCode::L; _map_keys[0x4D] = KeyCode::M; _map_keys[0x4E] = KeyCode::N; _map_keys[0x4F] = KeyCode::O;
		_map_keys[0x50] = KeyCode::P; _map_keys[0x51] = KeyCode::Q; _map_keys[0x52] = KeyCode::R; _map_keys[0x53] = KeyCode::S; _map_keys[0x54] = KeyCode::T;
		_map_keys[0x55] = KeyCode::U; _map_keys[0x56] = KeyCode::V; _map_keys[0x57] = KeyCode::W; _map_keys[0x58] = KeyCode::X; _map_keys[0x59] = KeyCode::Y;
		_map_keys[0x5A] = KeyCode::Z;

		_map_keys[VK_F1] = KeyCode::F1; _map_keys[VK_F2] = KeyCode::F2; _map_keys[VK_F3] = KeyCode::F3; _map_keys[VK_F4] = KeyCode::F4;
		_map_keys[VK_F5] = KeyCode::F5; _map_keys[VK_F6] = KeyCode::F6; _map_keys[VK_F7] = KeyCode::F7; _map_keys[VK_F8] = KeyCode::F8;
		_map_keys[VK_F9] = KeyCode::F9; _map_keys[VK_F10] = KeyCode::F10; _map_keys[VK_F11] = KeyCode::F11; _map_keys[VK_F12] = KeyCode::F12;

		_map_keys[VK_DOWN] = KeyCode::DOWN; _map_keys[VK_LEFT] = KeyCode::LEFT; _map_keys[VK_RIGHT] = KeyCode::RIGHT; _map_keys[VK_UP] = KeyCode::UP;

		_map_keys[VK_BACK] = KeyCode::BACK; _map_keys[VK_ESCAPE] = KeyCode::ESCAPE; _map_keys[VK_RETURN] = KeyCode::ENTER; _map_keys[VK_PAUSE] = KeyCode::PAUSE;
		_map_keys[VK_SCROLL] = KeyCode::SCROLL; _map_keys[VK_TAB] = KeyCode::TAB; _map_keys[VK_DELETE] = KeyCode::DEL; _map_keys[VK_HOME] = KeyCode::HOME;
		_map_keys[VK_END] = KeyCode::END; _map_keys[VK_PRIOR] = KeyCode::PGUP; _map_keys[VK_NEXT] = KeyCode::PGDN; _map_keys[VK_INSERT] = KeyCode::INS;
		_map_keys[VK_LSHIFT] = KeyCode::LSHIFT; _map_keys[VK_RSHIFT] = KeyCode::RSHIFT; _map_keys[VK_LCONTROL] = KeyCode::LCTRL; _map_keys[VK_RCONTROL] = KeyCode::RCTRL;
		_map_keys[VK_SPACE] = KeyCode::SPACE;

		_map_keys[0x30] = KeyCode::K0; _map_keys[0x31] = KeyCode::K1; _map_keys[0x32] = KeyCode::K2; _map_keys[0x33] = KeyCode::K3; _map_keys[0x34] = KeyCode::K4;
		_map_keys[0x35] = KeyCode::K5; _map_keys[0x36] = KeyCode::K6; _map_keys[0x37] = KeyCode::K7; _map_keys[0x38] = KeyCode::K8; _map_keys[0x39] = KeyCode::K9;

		return _hwnd;
	}

	bool Engine::create_opengl()
	{
		// create device context
		_gl_device_context = GetDC(_hwnd);
		PIXELFORMATDESCRIPTOR pfd =
		{
			sizeof(PIXELFORMATDESCRIPTOR), 1,
			PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
			PFD_TYPE_RGBA, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			PFD_MAIN_PLANE, 0, 0, 0, 0
		};

		int pf = 0;
		if (!(pf = ChoosePixelFormat(_gl_device_context, &pfd))) return false;
		SetPixelFormat(_gl_device_context, pf, &pfd);

		if (!(_gl_render_context = wglCreateContext(_gl_device_context))) return false;
		wglMakeCurrent(_gl_device_context, _gl_render_context);

		// remove Frame cap
		wglSwapInterval = (wglSwapInterval_t*)wglGetProcAddress("wglSwapIntervalEXT");
		wglSwapInterval(0);

		return true;
	}

	LRESULT CALLBACK Engine::window_event(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		static Engine* e;
		switch (uMsg)
		{
		case WM_CREATE:		e = (Engine*)((LPCREATESTRUCT)lParam)->lpCreateParams;	return 0;
		case WM_MOUSEMOVE:	e->update_mouse(LOWORD(lParam), HIWORD(lParam));		return 0;
		case WM_SETFOCUS:	e->_has_input_focus = true;								return 0;
		case WM_KILLFOCUS:	e->_has_input_focus = false;							return 0;
		case WM_KEYDOWN:	e->_key_new_state[_map_keys[wParam]] = true;			return 0;
		case WM_KEYUP:		e->_key_new_state[_map_keys[wParam]] = false;			return 0;
		case WM_LBUTTONDOWN:e->_mouse_new_state[0] = true;							return 0;
		case WM_LBUTTONUP:	e->_mouse_new_state[0] = false;							return 0;
		case WM_RBUTTONDOWN:e->_mouse_new_state[1] = true;							return 0;
		case WM_RBUTTONUP:	e->_mouse_new_state[1] = false;							return 0;
		case WM_MBUTTONDOWN:e->_mouse_new_state[2] = true;							return 0;
		case WM_MBUTTONUP:	e->_mouse_new_state[2] = false;							return 0;
		case WM_CLOSE:		_atom_active = false;									return 0;
		case WM_DESTROY:	PostQuitMessage(0);										return 0;
		}
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	// singleton
	std::atomic<bool> Engine::_atom_active{ false };
	std::map<uint16_t, uint8_t> Engine::_map_keys;
}