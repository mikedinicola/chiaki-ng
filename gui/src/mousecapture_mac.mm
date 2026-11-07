// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

// macOS pointer capture: freeze the cursor and read deltas from native mouse events.

#include <mousecapture.h>

#include <QWindow>

#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>

MouseCapture::MouseCapture(QWindow *window) : window(window) {}

MouseCapture::~MouseCapture()
{
	Stop();
}

bool MouseCapture::Start(DeltaCallback cb)
{
	if(active)
		return true;
	callback = std::move(cb);
	// Park the frozen cursor over the stream, away from the menu bar and Dock.
	center = QPointF(window->width() / 2.0, window->height() / 2.0);
	const QPoint global = window->mapToGlobal(center.toPoint());
	CGWarpMouseCursorPosition(CGPointMake(global.x(), global.y()));
	CGAssociateMouseAndMouseCursorPosition(false);

	MouseCapture *self = this;
	const NSEventMask mask = NSEventMaskMouseMoved | NSEventMaskLeftMouseDragged
		| NSEventMaskRightMouseDragged | NSEventMaskOtherMouseDragged;
	id monitor = [NSEvent addLocalMonitorForEventsMatchingMask:mask handler:^NSEvent *(NSEvent *event) {
		// deltaX/deltaY include the system's pointer acceleration; positive y is down.
		if(self->active && self->callback && (event.deltaX != 0.0 || event.deltaY != 0.0))
			self->callback(event.deltaX, event.deltaY);
		return event;
	}];
	native = (void *)CFBridgingRetain(monitor); // valid with or without ARC
	active = true;
	return true;
}

void MouseCapture::Stop()
{
	if(native)
	{
		id monitor = CFBridgingRelease(native);
		[NSEvent removeMonitor:monitor];
		native = nullptr;
	}
	if(active)
		CGAssociateMouseAndMouseCursorPosition(true);
	active = false;
	callback = nullptr;
}

void MouseCapture::HandleMouseMove(const QPointF &)
{
	// Native deltas are used on macOS; window positions are ignored while captured.
}
