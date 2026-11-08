// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#ifndef CHIAKI_MOUSECAPTURE_H
#define CHIAKI_MOUSECAPTURE_H

#include <QPointF>

#include <functional>

class QWindow;

/**
 * Locks the pointer to a window and reports relative motion, for mouse look.
 *
 * macOS: the cursor is frozen with CGAssociateMouseAndMouseCursorPosition and deltas
 * come from native mouse events, so pointer acceleration (important for trackpads)
 * is preserved. Elsewhere the cursor is warped back to the window centre after each
 * move; this needs cursor warping, which Wayland does not allow.
 */
class MouseCapture
{
	public:
		using DeltaCallback = std::function<void(double dx, double dy)>;

		explicit MouseCapture(QWindow *window);
		~MouseCapture();
		MouseCapture(const MouseCapture &) = delete;
		MouseCapture &operator=(const MouseCapture &) = delete;

		/// Returns false if pointer capture is not supported on this platform.
		bool Start(DeltaCallback callback);
		void Stop();
		bool IsActive() const { return active; }

		/// For the warp fallback: feed window-local mouse positions while active.
		void HandleMouseMove(const QPointF &local_pos);

	private:
		QWindow *window;
		DeltaCallback callback;
		bool active = false;
		void *native = nullptr; // macOS event monitor
		QPointF center;
};

#endif // CHIAKI_MOUSECAPTURE_H
