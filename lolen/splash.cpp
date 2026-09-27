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

namespace LoLEN {

// Name of the image in the resources of the executable (see dists/scummvm.rc)
static const char *const kSplashFileName = "lolen_splash.bmp";
// Display time in milliseconds, unless a key or mouse button is pressed
static const uint32 kSplashDuration = 2000;
// Duration of the cross-fade from the ScummVM splash screen, in milliseconds
static const uint32 kCrossFadeDuration = 500;
// Duration of the fade to black before the game starts, in milliseconds
static const uint32 kFadeOutDuration = 400;

namespace {

/** Returns true if the player wants to skip the splash screen. */
bool pollSkipEvents() {
	bool skip = false;
	Common::Event event;
	while (g_system->getEventManager()->pollEvent(event)) {
		if (event.type == Common::EVENT_KEYDOWN || event.type == Common::EVENT_LBUTTONDOWN ||
		    event.type == Common::EVENT_RBUTTONDOWN || event.type == Common::EVENT_QUIT ||
		    event.type == Common::EVENT_RETURN_TO_LAUNCHER)
			skip = true;
	}
	return skip;
}

/**
 * Writes into dst the mix of the surfaces from and to: weight 0 gives from,
 * weight 256 gives to. All surfaces have the same size and format.
 */
void blendSurfaces(Graphics::Surface &dst, const Graphics::Surface &from, const Graphics::Surface &to, uint weight) {
	const uint inverse = 256 - weight;

	if (dst.format.bytesPerPixel == 4) {
		// Every channel has 8 bits, the bytes can be mixed directly.
		for (int y = 0; y < dst.h; ++y) {
			const byte *a = (const byte *)from.getBasePtr(0, y);
			const byte *b = (const byte *)to.getBasePtr(0, y);
			byte *d = (byte *)dst.getBasePtr(0, y);
			for (int i = 0; i < dst.w * 4; ++i)
				d[i] = (byte)((a[i] * inverse + b[i] * weight) >> 8);
		}
		return;
	}

	for (int y = 0; y < dst.h; ++y) {
		for (int x = 0; x < dst.w; ++x) {
			uint8 ar, ag, ab, br, bg, bb;
			from.format.colorToRGB(from.getPixel(x, y), ar, ag, ab);
			to.format.colorToRGB(to.getPixel(x, y), br, bg, bb);
			dst.setPixel(x, y, dst.format.RGBToColor(
				(ar * inverse + br * weight) >> 8,
				(ag * inverse + bg * weight) >> 8,
				(ab * inverse + bb * weight) >> 8));
		}
	}
}

/**
 * Fades the overlay from one image to another. The fade is not interrupted by
 * the player, so that the screen never changes abruptly.
 */
void fade(const Graphics::Surface &from, const Graphics::Surface &to, uint32 duration) {
	Graphics::Surface frame;
	frame.create(from.w, from.h, from.format);

	const uint32 startTime = g_system->getMillis();
	uint32 elapsed = 0;
	while (elapsed < duration) {
		blendSurfaces(frame, from, to, elapsed * 256 / duration);
		g_system->copyRectToOverlay(frame.getPixels(), frame.pitch, 0, 0, frame.w, frame.h);
		g_system->updateScreen();
		pollSkipEvents();
		g_system->delayMillis(10);
		elapsed = g_system->getMillis() - startTime;
	}

	g_system->copyRectToOverlay(to.getPixels(), to.pitch, 0, 0, to.w, to.h);
	g_system->updateScreen();
	frame.free();
}

} // End of anonymous namespace

void showSplashScreen() {
	Common::File file;
	if (!file.open(Common::Path(kSplashFileName))) {
		warning("LoLEN splash screen '%s' not found", kSplashFileName);
		return;
	}

	Image::BitmapDecoder bitmap;
	if (!bitmap.loadStream(file)) {
		warning("Could not load the LoLEN splash screen '%s'", kSplashFileName);
		return;
	}

	OSystem &system = *g_system;
	system.showOverlay();

	const Graphics::PixelFormat format = system.getOverlayFormat();
	const int16 overlayWidth = system.getOverlayWidth();
	const int16 overlayHeight = system.getOverlayHeight();
	const uint32 black = format.ARGBToColor(0xff, 0, 0, 0);

	// What is on the overlay now: the ScummVM splash screen, shown just before
	Graphics::Surface previous;
	previous.create(overlayWidth, overlayHeight, format);
	system.grabOverlay(previous);

	// Black screen, the end of the fade to the game
	Graphics::Surface blackScreen;
	blackScreen.create(overlayWidth, overlayHeight, format);
	blackScreen.fillRect(Common::Rect(blackScreen.w, blackScreen.h), black);

	// The LoLEN image scaled to fit the window, keeping its aspect ratio, on a black background
	Graphics::Surface splash;
	splash.create(overlayWidth, overlayHeight, format);
	splash.fillRect(Common::Rect(splash.w, splash.h), black);

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
	splash.copyRectToSurface(*image, (overlayWidth - image->w) / 2, (overlayHeight - image->h) / 2, Common::Rect(image->w, image->h));
	image->free();
	delete image;

	// Cross-fade from the ScummVM splash screen
	fade(previous, splash, kCrossFadeDuration);

	// Wait, unless the player skips the splash screen
	const uint32 endTime = system.getMillis() + kSplashDuration;
	bool skip = false;
	while (!skip && system.getMillis() < endTime) {
		skip = pollSkipEvents();
		system.delayMillis(10);
	}

	// Fade to black before the game starts
	fade(splash, blackScreen, kFadeOutDuration);

	previous.free();
	blackScreen.free();
	splash.free();

	system.hideOverlay();
}

} // End of namespace LoLEN
