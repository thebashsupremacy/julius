#include "Graphics.h"
#include "Graphics_private.h"
#include "Graphics_Footprint.h"
#include "Loader.h"

#include "Data/Graphics.h"
#include "Data/Screen.h"
#include "Data/Types.h"
#include "Data/Constants.h"

#include <stdio.h> // remove later
#include <stdlib.h> // remove later
#include <string.h>

enum ColorType {
	ColorType_Set,
	ColorType_And,
	ColorType_None
};

static struct ClipRectangle {
	int xStart;
	int xEnd;
	int yStart;
	int yEnd;
} clipRectangle = {0, 800, 0, 600};

static GraphicsClipInfo clipInfo;

static void drawImageUncompressed(Data_Graphics_Index *index, const Color *data, int xOffset, int yOffset, Color color, ColorType type);
static void drawImageCompressed(Data_Graphics_Index *index, const unsigned char *data, int xOffset, int yOffset, int height, Color color, ColorType type);
static void setClipX(int xOffset, int width);
static void setClipY(int yOffset, int height);


ScreenColor ColorLookup[65536];

void Graphics_initialize()
{
	for (int c = 0; c < 65536; c++) {
		ColorLookup[c] =
			((c & 0xf800) << 8) | ((c & 0xe000) << 3) |
			((c & 0x7e0) << 5)  | ((c & 0x600) >> 1) |
			((c & 0x1f) << 3)   | ((c & 0x1c) >> 2);
	}
}

void Graphics_clearScreen()
{
	// TODO scanline?
	memset(Data_Screen.drawBuffer, 0, sizeof(ScreenColor) * Data_Screen.width * Data_Screen.height);
}

static void drawDot(int x, int y, Color color)
{
	if (x >= clipRectangle.xStart && x < clipRectangle.xEnd) {
		if (y >= clipRectangle.yStart && y < clipRectangle.yEnd) {
			ScreenPixel(x, y) = ColorLookup[color];
		}
	}
}

void Graphics_drawLine(int x1, int y1, int x2, int y2, Color color)
{
	if (x1 == x2) {
		int yMin = y1 < y2 ? y1 : y2;
		int yMax = y1 < y2 ? y2 : y1;
		for (int y = yMin; y <= yMax; y++) {
			drawDot(x1, y, color);
		}
	} else if (y1 == y2) {
		int xMin = x1 < x2 ? x1 : x2;
		int xMax = x1 < x2 ? x2 : x1;
		for (int x = xMin; x <= xMax; x++) {
			drawDot(x, y1, color);
		}
	} else {
		// non-straight line: ignore
	}
}

void Graphics_drawRect(int x, int y, int width, int height, Color color)
{
	if (x < 0) {
		x = 0;
	}
	if (width + x >= Data_Screen.width) {
		width = Data_Screen.width - x;
	}
	if (y < 0) {
		y = 0;
	}
	if (height + y >= Data_Screen.height) {
		height = Data_Screen.height - y;
	}
	Graphics_drawLine(x, y, x + width - 1, y, color);
	Graphics_drawLine(x, y + height - 1, x + width - 1, y + height - 1, color);
	Graphics_drawLine(x, y, x, y + height - 1, color);
	Graphics_drawLine(x + width - 1, y, x + width - 1, y + height - 1, color);
}

void Graphics_drawInsetRect(int x, int y, int width, int height)
{
	if (x < 0) {
		x = 0;
	}
	if (width + x >= Data_Screen.width) {
		width = Data_Screen.width - x;
	}
	if (y < 0) {
		y = 0;
	}
	if (height + y >= Data_Screen.height) {
		height = Data_Screen.height - y;
	}
	Graphics_drawLine(x, y, x + width - 1, y, Color_InsetDark);
	Graphics_drawLine(x + width - 1, y, x + width - 1, y + height - 1, Color_InsetLight);
	Graphics_drawLine(x, y + height - 1, x + width - 1, y + height - 1, Color_InsetLight);
	Graphics_drawLine(x, y, x, y + height - 1, Color_InsetDark);
}

void Graphics_fillRect(int x, int y, int width, int height, Color color)
{
	if (x < 0) {
		x = 0;
	}
	if (width + x >= Data_Screen.width) {
		width = Data_Screen.width - x;
	}
	if (y < 0) {
		y = 0;
	}
	if (height + y >= Data_Screen.height) {
		height = Data_Screen.height - y;
	}
	for (int yy = y; yy < height + y; yy++) {
		Graphics_drawLine(x, yy, x + width - 1, yy, color);
	}
}

void Graphics_shadeRect(int x, int y, int width, int height, int darkness)
{
	for (int yy = y; yy < y + height; yy++) {
		for (int xx = x; xx < x + width; xx++) {
			ScreenColor pixel = ScreenPixel(xx, yy);
			int r = (pixel & 0xf80000) >> 19;
			int g = (pixel & 0xfc00) >> 11;
			int b = (pixel & 0xf8) >> 3;
			int grey = (r + g + b) / 3 >> darkness;
			Color newPixel = (Color) (grey << 11 | grey << 6 | grey);
			ScreenPixel(xx, yy) = ColorLookup[newPixel];
		}
	}
}

void Graphics_drawIsometricFootprint(int graphicId, int xOffset, int yOffset, Color colorMask)
{
	Data_Graphics_Index *index = &Data_Graphics_Main.index[graphicId];
	const char *data;
	if (index->isExternal) {
		data = Loader_Graphics_loadExternalImagePixelData(graphicId);
		if (!data) {
			printf("ERR: unable to load external image %d\n", graphicId);
			return;
		}
	} else {
		data = &Data_Graphics_PixelData.main[index->offset];
	}
	if (index->type == 30) { // isometric
		switch (index->width) {
			case 58:
				Graphics_Footprint_drawSize1(graphicId, xOffset, yOffset, colorMask);
				break;
			case 118:
				Graphics_Footprint_drawSize2(graphicId, xOffset, yOffset, colorMask);
				break;
			case 178:
				Graphics_Footprint_drawSize3(graphicId, xOffset, yOffset, colorMask);
				break;
			case 238:
				Graphics_Footprint_drawSize4(graphicId, xOffset, yOffset, colorMask);
				break;
			case 298:
				Graphics_Footprint_drawSize5(graphicId, xOffset, yOffset, colorMask);
				break;
		}
	} else {
		printf("ERROR: trying to draw a non-isometric tile using drawIsometricFootprint\n");
	}
}

void Graphics_drawIsometricTop(int graphicId, int xOffset, int yOffset, Color colorMask)
{
	Data_Graphics_Index *index = &Data_Graphics_Main.index[graphicId];
	if (index->type != 30) { // isometric
		printf("ERROR: trying to draw a non-isometric tile using drawIsometricTop\n");
	}
	if (!index->hasCompressedPart) {
		return;
	}
	const char *data = &Data_Graphics_PixelData.main[index->offset];
	data += index->uncompressedLength;

	int height = index->height;
	switch (index->width) {
		case 58:
			yOffset -= index->height - 30;
			height -= 16;
			break;
		case 118:
			xOffset -= 30;
			yOffset -= index->height - 60;
			height -= 31;
			break;
		case 178:
			xOffset -= 60;
			yOffset -= index->height - 90;
			height -= 46;
			break;
		case 238:
			xOffset -= 90;
			yOffset -= index->height - 120;
			height -= 61;
			break;
		case 298:
			xOffset -= 120;
			yOffset -= index->height - 150;
			height -= 76;
			break;
	}
	drawImageCompressed(index, (unsigned char*)data, xOffset, yOffset, height,
		colorMask, colorMask ? ColorType_And : ColorType_None);
}

void Graphics_drawImage(int graphicId, int xOffset, int yOffset)
{
	Data_Graphics_Index *index = &Data_Graphics_Main.index[graphicId];
	const char *data;
	if (index->isExternal) {
		data = Loader_Graphics_loadExternalImagePixelData(graphicId);
		if (!data) {
			printf("ERR: unable to load external image %d\n", graphicId);
			return;
		}
	} else {
		data = &Data_Graphics_PixelData.main[index->offset];
	}

	if (index->isFullyCompressed) {
		drawImageCompressed(index, (unsigned char*)data, xOffset, yOffset, index->height, 0, ColorType_None);
	} else {
		drawImageUncompressed(index, (Color*)data, xOffset, yOffset, 0, ColorType_None);
	}
}

void Graphics_drawImageMasked(int graphicId, int xOffset, int yOffset, Color colorMask)
{
	Data_Graphics_Index *index = &Data_Graphics_Main.index[graphicId];
	const char *data;
	if (index->isExternal) {
		data = Loader_Graphics_loadExternalImagePixelData(graphicId);
		if (!data) {
			printf("ERR: unable to load external image %d\n", graphicId);
			return;
		}
	} else {
		data = &Data_Graphics_PixelData.main[index->offset];
	}

	if (index->type == 30) { // isometric
		printf("ERROR: use Graphics_drawIsometricFootprint for isometric!\n");
		return;
	}

	if (index->isFullyCompressed) {
		drawImageCompressed(index, (unsigned char*)data, xOffset, yOffset, index->height,
			colorMask, colorMask ? ColorType_And : ColorType_None);
	} else {
		drawImageUncompressed(index, (Color*)data, xOffset, yOffset,
			colorMask, colorMask ? ColorType_And : ColorType_None);
	}
}

void Graphics_drawLetter(int graphicId, int xOffset, int yOffset, Color color)
{
	Data_Graphics_Index *index = &Data_Graphics_Main.index[graphicId];
	const char *data;
	if (index->isExternal) {
		data = Loader_Graphics_loadExternalImagePixelData(graphicId);
		if (!data) {
			printf("ERR: unable to load external image %d\n", graphicId);
			return;
		}
	} else {
		data = &Data_Graphics_PixelData.main[index->offset];
	}

	if (index->isFullyCompressed) {
		drawImageCompressed(index, (unsigned char*)data, xOffset, yOffset, index->height,
			color, color ? ColorType_Set : ColorType_None);
	} else {
		drawImageUncompressed(index, (Color*)data, xOffset, yOffset,
			color, color ? ColorType_Set : ColorType_None);
	}
}

static void drawImageUncompressed(Data_Graphics_Index *index, const Color *data, int xOffset, int yOffset, Color color, ColorType type)
{
	GraphicsClipInfo *clip = Graphics_getClipInfo(
		xOffset, yOffset, index->width, index->height);
	if (!clip->isVisible) {
		return;
	}
	data += index->width * clip->clippedPixelsTop;
	for (int y = clip->clippedPixelsTop; y < index->height - clip->clippedPixelsBottom; y++) {
		data += clip->clippedPixelsLeft;
		for (int x = clip->clippedPixelsLeft; x < index->width - clip->clippedPixelsRight; x++) {
			if (*data != Color_Transparent) {
				ScreenPixel(xOffset + x, yOffset + y) =
					ColorLookup[type == ColorType_None ? *data : (type == ColorType_Set ? color : (color & *data))];
			}
			data++;
		}
		data += clip->clippedPixelsRight;
	}
}

static void drawImageCompressed(Data_Graphics_Index *index, const unsigned char *data, int xOffset, int yOffset, int height, Color color, ColorType type)
{
	GraphicsClipInfo *clip = Graphics_getClipInfo(
		xOffset, yOffset, index->width, height);
	if (!clip->isVisible) {
		return;
	}

	for (int y = 0; y < height - clip->clippedPixelsBottom; y++) {
		int x = 0;
		while (x < index->width) {
			unsigned char b = *data;
			data++;
			if (b == 255) {
				// transparent pixels to skip
				x += *data;
				data++;
			} else if (y < clip->clippedPixelsTop) {
				data += b * 2;
				x += b;
			} else {
				// number of concrete pixels
				Color *pixels = (Color*) data;
				while (b) {
					if (x >= clip->clippedPixelsLeft && x < index->width - clip->clippedPixelsRight) {
						ScreenPixel(xOffset + x, yOffset + y) =
							ColorLookup[type == ColorType_None ? *pixels : (type == ColorType_Set ? color : (color & *pixels))];
					}
					x++;
					pixels++;
					data += 2;
					b--;
				}
			}
		}
	}
}

void Graphics_setClipRectangle(int x, int y, int width, int height)
{
	clipRectangle.xStart = x;
	clipRectangle.xEnd = x + width;
	clipRectangle.yStart = y;
	clipRectangle.yEnd = y + height;
	if (clipRectangle.xEnd > Data_Screen.width) {
		printf("ERROR: clip rectangle x end (%d) > screen width\n", clipRectangle.xEnd);
		clipRectangle.xEnd = Data_Screen.width;
	}
	if (clipRectangle.yEnd > Data_Screen.height) {
		printf("ERROR: clip rectangle y end (%d) > screen height\n", clipRectangle.yEnd);
		clipRectangle.yEnd = Data_Screen.height-15;
	}
}

void Graphics_resetClipRectangle()
{
	clipRectangle.xStart = 0;
	clipRectangle.xEnd = Data_Screen.width;
	clipRectangle.yStart = 0;
	clipRectangle.yEnd = Data_Screen.height;
}

GraphicsClipInfo *Graphics_getClipInfo(int xOffset, int yOffset, int width, int height)
{
	setClipX(xOffset, width);
	setClipY(yOffset, height);
	if (clipInfo.clipX == ClipInvisible || clipInfo.clipY == ClipInvisible) {
		clipInfo.isVisible = 0;
	} else {
		clipInfo.isVisible = 1;
	}
	return &clipInfo;
}

static void setClipX(int xOffset, int width)
{
	clipInfo.clippedPixelsLeft = 0;
	clipInfo.clippedPixelsRight = 0;
	if (width <= 0
		|| xOffset + width <= clipRectangle.xStart
		|| xOffset >= clipRectangle.xEnd) {
		clipInfo.clipX = ClipInvisible;
		clipInfo.visiblePixelsX = 0;
		return;
	}
	if (xOffset < clipRectangle.xStart) {
		// clipped on the left
		clipInfo.clippedPixelsLeft = clipRectangle.xStart - xOffset;
		if (xOffset + width <= clipRectangle.xEnd) {
			clipInfo.clipX = ClipLeft;
		} else {
			clipInfo.clipX = ClipBoth;
			clipInfo.clippedPixelsRight = xOffset + width - clipRectangle.xEnd;
		}
	} else if (xOffset + width > clipRectangle.xEnd) {
		clipInfo.clipX = ClipRight;
		clipInfo.clippedPixelsRight = xOffset + width - clipRectangle.xEnd;
	} else {
		clipInfo.clipX = ClipNone;
	}
	clipInfo.visiblePixelsX = width - clipInfo.clippedPixelsLeft - clipInfo.clippedPixelsRight;
}

static void setClipY(int yOffset, int height)
{
	clipInfo.clippedPixelsTop = 0;
	clipInfo.clippedPixelsBottom = 0;
	if (height <= 0
		|| yOffset + height <= clipRectangle.yStart
		|| yOffset >= clipRectangle.yEnd) {
		clipInfo.clipY = ClipInvisible;
	} else if (yOffset < clipRectangle.yStart) {
		// clipped on the top
		clipInfo.clippedPixelsTop = clipRectangle.yStart - yOffset;
		if (yOffset + height <= clipRectangle.yEnd) {
			clipInfo.clipY = ClipTop;
		} else {
			clipInfo.clipY = ClipBoth;
			clipInfo.clippedPixelsBottom = yOffset + height - clipRectangle.yEnd;
		}
	} else if (yOffset + height > clipRectangle.yEnd) {
		clipInfo.clipY = ClipBottom;
		clipInfo.clippedPixelsBottom = yOffset + height - clipRectangle.yEnd;
	} else {
		clipInfo.clipY = ClipNone;
	}
	clipInfo.visiblePixelsY = height - clipInfo.clippedPixelsTop - clipInfo.clippedPixelsBottom;
}

void Graphics_drawEnemyImage(int graphicId, int xOffset, int yOffset)
{
	if (graphicId <= 0 || graphicId >= 801) {
		return;
	}
	Data_Graphics_Index *index = &Data_Graphics_Enemy.index[graphicId];
	if (index->offset > 0) {
		drawImageCompressed(index, (unsigned char*)&Data_Graphics_PixelData.enemy[index->offset],
			xOffset, yOffset, index->height, 0, ColorType_None);
	}
}

/////debug/////

static void pixel(ScreenColor input, unsigned char *r, unsigned char *g, unsigned char *b)
{
	int rr = (input & 0xff0000) >> 16;
	int gg = (input & 0x00ff00) >> 8;
	int bb = (input & 0x0000ff) >> 0;
	*r = (unsigned char) rr;
	*g = (unsigned char) gg;
	*b = (unsigned char) bb;
}

void Graphics_saveScreenshot(const char *filename)
{
	struct {
		char __filler1;
		char __filler2;
		char B;
		char M;
		int filesize;
		int reserved;
		int dataOffset;

		int dibSize;
		short width;
		short height;
		short planes;
		short bpp;
	} header = {
		0, 0, 'B', 'M', 26 + 800 * 600 * 3, 0, 26,
		12, 800, 600, 1, 24
	};
	unsigned char *pixels = (unsigned char*) malloc(800 * 3);
	FILE *fp = fopen(filename, "wb");
	fwrite(&header.B, 1, 26, fp);
	for (int y = 599; y >= 0; y--) {
		for (int x = 0; x < 800; x++) {
			pixel(((ScreenColor*)Data_Screen.drawBuffer)[y*800+x],
				&pixels[3*x+2], &pixels[3*x+1], &pixels[3*x]);
		}
		fwrite(pixels, 1, 3 * 800, fp);
	}
	fclose(fp);
	free(pixels);
}

