// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include <mouselook.h>

#include <algorithm>
#include <cmath>

namespace
{
	constexpr double kWindowMs = 16.0;     // velocity measuring window
	constexpr double kHoldMs = 24.0;       // keep the last velocity this long between events
	constexpr double kCoastGapMs = 12.0;   // with momentum, coast once events pause this long
	constexpr double kFullTiltSpeed = 1400.0 * 80.0; // counts/s * reference sensitivity
}

void MouseLook::Reset()
{
	samples.clear();
	has_previous = false;
	gliding = false;
	last_read = -1.0;
}

void MouseLook::Push(double dx, double dy, double now_ms)
{
	if(!std::isfinite(dx) || !std::isfinite(dy) || (dx == 0.0 && dy == 0.0))
		return;
	samples.push_back({dx, dy, now_ms});
}

MouseLookStick MouseLook::Read(double now_ms, const MouseLookConfig &config)
{
	const double dt = last_read < 0.0 ? 16.0 : std::clamp(now_ms - last_read, 0.0, 100.0);
	last_read = now_ms;
	const double tau = std::clamp(config.momentum_ms, 0.0, 1000.0);
	const double sensitivity = std::max(1.0, config.sensitivity);
	const double gain = std::max(0.1, config.gain);
	const double curve = std::clamp(config.curve, 0.2, 2.0);
	const double deadzone = std::clamp(config.deadzone, 0.0, 0.3);
	// Glides stop within 1% of the usable range above the dead zone (inverted through the curve).
	const double stop_speed = std::pow(0.01, 1.0 / curve) * kFullTiltSpeed / (sensitivity * gain);

	samples.erase(std::remove_if(samples.begin(), samples.end(),
		[now_ms](const Sample &s) { return now_ms - s.t > kHoldMs; }), samples.end());

	Velocity v;
	const bool stale = tau > 0.0 && gliding && !samples.empty() && now_ms - samples.back().t > kCoastGapMs;
	if(!samples.empty() && !stale)
	{
		bool live = false;
		for(const auto &s : samples)
		{
			if(now_ms - s.t < kWindowMs)
			{
				v.x += s.dx;
				v.y += s.dy;
				live = true;
			}
		}
		v.x /= kWindowMs / 1000.0;
		v.y /= kWindowMs / 1000.0;
		if(!live && has_previous)
			v = previous;
		previous = v;
		has_previous = true;
		if(tau > 0.0 && gliding && live)
		{
			// light smoothing while moving keeps glides from inheriting jitter
			const double a = 1.0 - std::exp(-dt / std::max(8.0, tau * 0.12));
			v.x = glide.x + (v.x - glide.x) * a;
			v.y = glide.y + (v.y - glide.y) * a;
		}
		glide = v;
		gliding = tau > 0.0;
	}
	else
	{
		if(samples.empty())
			has_previous = false;
		if(tau <= 0.0 || !gliding)
		{
			gliding = false;
			return {};
		}
		const double k = std::exp(-dt / tau);
		glide.x *= k;
		glide.y *= k;
		if(std::hypot(glide.x, glide.y) < stop_speed)
		{
			gliding = false;
			return {};
		}
		v = glide;
	}

	const double speed = std::hypot(v.x, v.y);
	if(speed <= 0.0)
		return {};
	const double linear = std::min(1.0, speed * sensitivity * gain / kFullTiltSpeed);
	const double magnitude = deadzone + (1.0 - deadzone) * std::pow(linear, curve);
	MouseLookStick out;
	out.x = v.x / speed * magnitude;
	out.y = v.y / speed * magnitude * (config.invert_y ? -1.0 : 1.0);
	return out;
}
