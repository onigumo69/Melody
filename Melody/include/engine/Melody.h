#pragma once

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "gdiplus.lib")

// windows
#include <Windows.h>
#include <gdiplus.h>

// opengl extension
#include <gl/GL.h>

// std
#include <cmath>
#include <cstdint>
#include <string>
#include <iostream>
#include <chrono>
#include <vector>
#include <list>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <fstream>
#include <map>
#include <codecvt>


namespace Melody
{
	enum ReturnCode
	{
		FAIL = 0,
		OK = 1,
		NO_FILE = -1
	};

	struct ButtonState
	{
		bool Pressed = false;	// Set once during the frame the event occurs
		bool Released = false;	// Set once during the frame the event occurs
		bool Held = false;		// Set tru for all frames between pressed and released events
	};

	enum KeyCode
	{
		A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
		K0, K1, K2, K3, K4, K5, K6, K7, K8, K9,
		F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
		UP, DOWN, LEFT, RIGHT,
		SPACE, TAB, LSHIFT, RSHIFT, LCTRL, RCTRL, LALT, RALT, INS, DEL, HOME, END, PGUP, PGDN,
		BACK, ESCAPE, ENTER, PAUSE, SCROLL,
	};

	struct Pixel
	{
		union
		{
			uint32_t n = 0xFF000000;
			struct
			{
				uint8_t r;
				uint8_t g;
				uint8_t b;
				uint8_t a;
			};
		};

		Pixel();
		Pixel(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255);
		enum Mode
		{
			NORMAL, // no transparency
			MASK,	// transparent if alpha is < 255
			ALPHA	// full transparency
		};
	};

	static const Pixel
		WHITE(255, 255, 255),
		GREY(192, 192, 192), DARK_GREY(128, 128, 128), VERY_DARK_GREY(64, 64, 64),
		RED(255, 0, 0), DARK_RED(128, 0, 0), VERY_DARK_RED(64, 0, 0),
		YELLOW(255, 255, 0), DARK_YELLOW(128, 128, 0), VERY_DARK_YELLOW(64, 64, 0),
		GREEN(0, 255, 0), DARK_GREEN(0, 128, 0), VERY_DARK_GREEN(0, 64, 0),
		CYAN(0, 255, 255), DARK_CYAN(0, 128, 128), VERY_DARK_CYAN(0, 64, 64),
		BLUE(0, 0, 255), DARK_BLUE(0, 0, 128), VERY_DARK_BLUE(0, 0, 64),
		MAGENTA(255, 0, 255), DARK_MAGENTA(128, 0, 128), VERY_DARK_MAGENTA(64, 0, 64),
		BLACK(0, 0, 0);

	class Sprite
	{
	public:
		Sprite();
		Sprite(std::string image_file);
		Sprite(int32_t w, int32_t h);
		~Sprite();

	public:
		ReturnCode load_from_file(std::string image_file);

	public:
		int32_t _width = 0;
		int32_t _height = 0;

	public:
		Pixel get_pixel(int32_t x, int32_t y) const;
		void set_pixel(int32_t x, int32_t y, Pixel p);
		Pixel sample(float x, float y) const;
		Pixel* get_data() const;

	private:
		Pixel* _color_data = nullptr;
	};


	class Engine
	{
	public:
		Engine();

	public:
		ReturnCode construct(uint32_t screen_w, uint32_t screen_h, uint32_t pixel_w, uint32_t pixel_h);
		ReturnCode Start();

	public: // game override interface
		virtual bool on_awake();
		virtual bool on_update(float delta_time);
		virtual bool on_destroy();

	public: // input
		bool is_focused() const;
		ButtonState get_key(KeyCode key) const;
		ButtonState get_mouse(char button) const;
		int32_t get_mouse_x() const;
		int32_t get_mouse_y() const;

	public: // util
		int32_t get_screen_width() const;
		int32_t get_screen_height() const;
		int32_t get_drawing_target_width() const;
		int32_t get_drawing_target_height() const;
		Sprite* get_drawing_target();

	public: // draw routine
		void set_drawing_target(Sprite* target); // pass null to specify the primary screen
		void set_pixel_mode(Pixel::Mode mode);

		virtual void draw_pixel(int32_t x, int32_t y, Pixel p);
		void draw_line(int32_t x1, int32_t y1, int32_t x2, int32_t y2, Pixel p);
		void draw_circle(int32_t x, int32_t y, int32_t radius, Pixel p);
		void fill_circle(int32_t x, int32_t y, int32_t radius, Pixel p);
		void draw_rect(int32_t x, int32_t y, int32_t w, int32_t h, Pixel p);
		void fill_rect(int32_t x, int32_t y, int32_t w, int32_t h, Pixel p);
		void draw_triangle(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x3, int32_t y3, Pixel p);
		void fill_triangle(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x3, int32_t y3, Pixel p);
		void draw_sprite(int32_t x, int32_t y, Sprite* sprite);
		void draw_sprite_partial(int32_t x, int32_t y, Sprite* sprite, int32_t ox, int32_t oy, int32_t w, int32_t h);

	public:
		std::string _app_name;

	private:
		Sprite* _default_drawing_target = nullptr;
		Sprite* _drawing_target = nullptr;
		Pixel::Mode _pixel_mode = Pixel::Mode::NORMAL;
		uint32_t _screen_width = 256;
		uint32_t _screen_height = 240;
		uint32_t _pixel_width = 4;
		uint32_t _pixel_height = 4;
		uint32_t _mouse_pos_x = 0;
		uint32_t _mouse_pos_y = 0;
		bool _has_input_focus = false;

		static std::map<uint16_t, uint8_t> _map_keys;

		bool _key_new_state[256]{ 0 };
		bool _key_old_state[256]{ 0 };
		ButtonState _keyboard_state[256];

		bool _mouse_new_state[5]{ 0 };
		bool _mouse_old_state[5]{ 0 };
		ButtonState _mouse_state[5];

		HDC _gl_device_context = nullptr;
		HGLRC _gl_render_context = nullptr;

		GLuint _gl_buffer;

		void threading();

		// flag for shutting down
		static std::atomic<bool> _atom_active;

		// initialization
		void update_mouse(uint32_t x, uint32_t y);
		bool create_opengl();

		// windows bs
		HWND _hwnd = nullptr;
		HWND create_window();
		std::wstring _window_name;
		static LRESULT CALLBACK window_event(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	};
}