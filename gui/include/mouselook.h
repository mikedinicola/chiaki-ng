// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#ifndef CHIAKI_MOUSELOOK_H
#define CHIAKI_MOUSELOOK_H

#include <vector>

/**
 * Converts relative mouse/trackpad motion into an analog stick position.
 *
 * Motion is measured as velocity over a short fixed window, so the result does not
 * depend on how the OS batches events. The stick starts at the game's dead zone
 * (compensation) so small movements are not swallowed. Trackpad settings add a
 * gain, a response curve (< 1 lifts small movements) and momentum: after motion
 * stops the stick keeps its velocity and decays exponentially, like inertial scrolling.
 *
 * Based on the mouse-to-stick model in Chiaki-NG Web (pc-controls.js).
 */
struct MouseLookConfig
{
	double sensitivity = 80.0; ///< 10..400; 80 reaches full tilt at 1400 counts/s
	double deadzone = 0.12;    ///< 0..0.3 of stick range, matched to the game's dead zone
	double gain = 1.0;         ///< trackpad boost
	double curve = 1.0;        ///< response exponent, 0.2..2
	double momentum_ms = 0.0;  ///< glide time constant after motion stops, 0 = off
	bool invert_y = false;
};

struct MouseLookStick
{
	double x = 0.0; ///< -1..1
	double y = 0.0; ///< -1..1, positive = down
	bool IsIdle() const { return x == 0.0 && y == 0.0; }
};

class MouseLook
{
	public:
		void Reset();
		/// Relative motion in pointer units at time now_ms.
		void Push(double dx, double dy, double now_ms);
		/// Stick position at now_ms. Call regularly (every few ms) while active.
		MouseLookStick Read(double now_ms, const MouseLookConfig &config);

	private:
		struct Sample { double dx, dy, t; };
		struct Velocity { double x = 0.0, y = 0.0; };
		std::vector<Sample> samples;
		Velocity previous;
		bool has_previous = false;
		Velocity glide;
		bool gliding = false;
		double last_read = -1.0;
};

#endif // CHIAKI_MOUSELOOK_H
