#pragma once

// used: callvfunc
#include "..\..\utils\memory\vfunc\vfunc.h"
#include "..\..\utils\math\vector\vector.h"
#include <type_traits>

class c_local_data
{
public:
	char pad_0000[12]; //0x0000
	Vector_t m_eye_pos; //0x000C
}; //Size: 0x0018

class c_networked_client_info
{
public:
	char pad_0000[4]; //0x0000
	int32_t m_render_tick; //0x0004
	float m_render_tick_fraction; //0x0008
	int32_t m_player_tick_count; //0x000C
	float m_player_tick_fraction; //0x0010
	char pad_0014[4]; //0x0014
	c_local_data* m_local_data; //0x0018
	char pad_0030[24]; //0x0030
}; //Size: 0x0048

class IEngineClient
{
public:
	int maxClients()
	{
		return M::vfunc<int, 35U>(this);
	}

	bool in_game()
	{
		return M::vfunc<bool, 39U>(this);
	}

	bool connected()
	{
		return M::vfunc<bool, 40U>(this);
	}

	int get_local_player() {
		int nIndex = -1;
		M::vfunc<void, 54U>(this, std::ref(nIndex), 0);
		return nIndex + 1;
	}

	c_networked_client_info* get_networked_client_info() {
		c_networked_client_info client_info;

		M::CallVFunc<void, 179U>(this, &client_info, 1);
		return &client_info;
	}

	void get_screen_size(int& width, int& height)
	{
		return M::CallVFunc<void, 60U>(this, width, height);
	}
public:
	inline bool valid() {
		return connected() && in_game();
	}

};