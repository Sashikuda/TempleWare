#pragma once
#include <string>

// -----------------------------------------------------------------------------
// Client Chat Messages
//
// Prints client-side messages into the in-game chat / notice HUD using the
// game's own CCSGO_HudVoiceStatus push_notice function. Messages are purely
// local (never sent to the server) and support HTML-style font color tags:
//
//   chat::push("<font color=\"#8FD8FF\">[TempleWare]</font> hello");
//
// push() is thread-safe: messages are queued and flushed on the game thread
// during FrameStageNotify (FRAME_RENDER_END), since the HUD element lookup and
// push_notice call must happen on the game thread while in-game.
// -----------------------------------------------------------------------------
namespace chat
{
	// Resolves push_notice and find_hud_element via pattern scan in client.dll.
	// Call once during hook initialization.
	void init();

	// Queues a client-side chat message (thread-safe, callable from anywhere).
	// Supports <font color="#RRGGBB"> tags.
	void push(const std::string& text);

	// Convenience helper: prints "[REDLINE] <text>" with the branded prefix.
	void push_prefixed(const std::string& text);

	// Runs on the game thread (FrameStageNotify / FRAME_RENDER_END), including
	// out of game. Detects every out-of-game -> in-game transition and prints
	// the REDLINE welcome message, then flushes queued messages while in-game.
	void on_frame();
}
