/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "lolen/splash.h"

#include "common/events.h"
#include "common/file.h"
#include "common/system.h"
#include "common/textconsole.h"

#include "graphics/surface.h"

#include "image/bmp.h"

namespace LolEN {

// Name of the image in the resources of the executable (see dists/scummvm.rc)
static const char *const kSplashFileName = "lolen_splash.bmp";
// Display time in milliseconds, unless a key or mouse button is pressed
static const uint32 kSplashDuration = 2000;

void showSplashScreen() {
	Common::File file;
	if (!file.open(Common::Path(kSplashFileName))) {
		warning("LolEN splash screen '%s' not found", kSplashFileName);
		return;
	}

	Image::BitmapDecoder bitmap;
	if (!bitmap.loadStream(file)) {
		warning("Could not load the LolEN splash screen '%s'", kSplashFileName);
		return;
	}

	OSystem &system = *g_system;
	system.showOverlay();

	const Graphics::PixelFormat format = system.getOverlayFormat();
	const int16 overlayWidth = system.getOverlayWidth();
	const int16 overlayHeight = system.getOverlayHeight();

	// Black background
	Graphics::Surface screen;
	screen.create(overlayWidth, overlayHeight, format);
	screen.fillRect(Common::Rect(screen.w, screen.h), format.ARGBToColor(0xff, 0, 0, 0));
	system.copyRectToOverlay(screen.getPixels(), screen.pitch, 0, 0, screen.w, screen.h);
	screen.free();

	// Scale the image to fit the window, keeping its aspect ratio
	Graphics::Surface *image = bitmap.getSurface()->convertTo(format, bitmap.getPalette().data(), bitmap.getPalette().size());
	const float scale = MIN<float>((float)overlayWidth / image->w, (float)overlayHeight / image->h);
	const int16 width = MAX<int16>(1, (int16)(image->w * scale));
	const int16 height = MAX<int16>(1, (int16)(image->h * scale));
	if (width != image->w || height != image->h) {
		Graphics::Surface *scaled = image->scale(width, height, true);
		image->free();
		delete image;
		image = scaled;
	}

	const int x = (overlayWidth - image->w) / 2;
	const int y = (overlayHeight - image->h) / 2;
	system.copyRectToOverlay(image->getPixels(), image->pitch, x, y, image->w, image->h);
	image->free();
	delete image;

	system.updateScreen();

	// Wait, unless the player skips the splash screen
	const uint32 endTime = system.getMillis() + kSplashDuration;
	bool skip = false;
	while (!skip && system.getMillis() < endTime) {
		Common::Event event;
		while (system.getEventManager()->pollEvent(event)) {
			if (event.type == Common::EVENT_KEYDOWN || event.type == Common::EVENT_LBUTTONDOWN ||
			    event.type == Common::EVENT_RBUTTONDOWN || event.type == Common::EVENT_QUIT ||
			    event.type == Common::EVENT_RETURN_TO_LAUNCHER)
				skip = true;
		}
		system.delayMillis(10);
	}

	system.hideOverlay();
}

} // End of namespace LolEN
