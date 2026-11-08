// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

// Warp-to-centre pointer capture for platforms without a native implementation.

#include <mousecapture.h>

#include <QCursor>
#include <QGuiApplication>
#include <QWindow>

#include <cmath>

MouseCapture::MouseCapture(QWindow *window) : window(window) {}

MouseCapture::~MouseCapture()
{
	Stop();
}

bool MouseCapture::Start(DeltaCallback cb)
{
	if(active)
		return true;
	if(QGuiApplication::platformName().startsWith(QLatin1String("wayland")))
		return false; // Wayland clients cannot warp the cursor
	callback = std::move(cb);
	center = QPointF(window->width() / 2.0, window->height() / 2.0);
	QCursor::setPos(window->mapToGlobal(center.toPoint()));
	active = true;
	return true;
}

void MouseCapture::Stop()
{
	active = false;
	callback = nullptr;
}

void MouseCapture::HandleMouseMove(const QPointF &local_pos)
{
	if(!active)
		return;
	const QPointF delta = local_pos - center;
	if(std::abs(delta.x()) < 0.5 && std::abs(delta.y()) < 0.5)
		return; // the event caused by our own warp
	if(callback)
		callback(delta.x(), delta.y());
	QCursor::setPos(window->mapToGlobal(center.toPoint()));
}
