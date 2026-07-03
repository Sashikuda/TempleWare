#include "chat.h"

#include <Windows.h>
#include <cstdint>
#include <mutex>
#include <vector>

#include "../../interfaces/interfaces.h"
#include "../../utils/memory/patternscan/patternscan.h"

// -----------------------------------------------------------------------------
// client.dll functions (resolved by pattern scan, revalidate after updates)
// -----------------------------------------------------------------------------
namespace
{
	// "48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 41 56 41 57 48 8D AC 24 ? ? ? ? B8 60 10 00 00" @ client.dll
	using push_notice_fn = __int64(__fastcall*)(__int64 hud_voice_status, const char* text, unsigned int ent_index, unsigned char* flags);
	push_notice_fn g_push_notice = nullptr;

	// "4C 8B DC 53 48 83 EC ? 48 8B 05" @ client.dll
	using find_hud_element_fn = __int64(__fastcall*)(const char* name);
	find_hud_element_fn g_find_hud_element = nullptr;

	// Pending messages queued from any thread, flushed on the game thread.
	std::mutex               g_queueMutex;
	std::vector<std::string> g_queue;

	// Prints a single message. MUST be called on the game thread while in-game.
	bool print_now(const char* text)
	{
		if (!g_push_notice || !g_find_hud_element)
			return false;

		const __int64 hud = g_find_hud_element("CCSGO_HudVoiceStatus");
		const __int64 hud_voice_status = hud ? hud - 0x20 : 0;
		if (!hud_voice_status)
			return false;

		unsigned char flags[2]{ 1, 0 };
		g_push_notice(hud_voice_status, text, 0xFFFFFFFFu, flags);
		return true;
	}
}

// -----------------------------------------------------------------------------
// Initialization
// -----------------------------------------------------------------------------
void chat::init()
{
	g_push_notice = reinterpret_cast<push_notice_fn>(
		M::patternScan("client", "48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 41 56 41 57 48 8D AC 24 ? ? ? ? B8 60 10 00 00"));

	g_find_hud_element = reinterpret_cast<find_hud_element_fn>(
		M::patternScan("client", "4C 8B DC 53 48 83 EC ? 48 8B 05"));

	printf("chat::push_notice: 0x%p\n", reinterpret_cast<void*>(g_push_notice));
	printf("chat::find_hud_element: 0x%p\n", reinterpret_cast<void*>(g_find_hud_element));
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------
void chat::push(const std::string& text)
{
	std::lock_guard<std::mutex> lock(g_queueMutex);
	g_queue.push_back(text);
}

void chat::push_prefixed(const std::string& text)
{
	push(std::string("<font color=\"#8FD8FF\">[TempleWare]</font> ") + text);
}

// -----------------------------------------------------------------------------
// Game thread flush
// -----------------------------------------------------------------------------
void chat::on_frame()
{
	if (!g_push_notice || !g_find_hud_element)
		return;

	// the HUD only exists while in-game; keep messages queued until then
	if (!I::EngineClient || !I::EngineClient->valid())
		return;

	std::vector<std::string> pending;
	{
		std::lock_guard<std::mutex> lock(g_queueMutex);
		if (g_queue.empty())
			return;
		pending.swap(g_queue);
	}

	for (std::size_t i = 0; i < pending.size(); i++)
	{
		// if the HUD element is not available yet, re-queue the remaining
		// messages (including this one) and retry next frame
		if (!print_now(pending[i].c_str()))
		{
			std::lock_guard<std::mutex> lock(g_queueMutex);
			g_queue.insert(g_queue.begin(), pending.begin() + i, pending.end());
			return;
		}
	}
}
