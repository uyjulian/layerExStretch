
// Shim class for compatibilty purposes.
#ifndef __LAYERBITMAPINTF_H__
#define __LAYERBITMAPINTF_H__

class tTVPBaseBitmap
{
	tjs_uint width;
	tjs_uint height;
	tjs_int pitch;
	tjs_uint8* bmpdata;
public:
	tTVPBaseBitmap(tjs_uint in_width, tjs_uint in_height, tjs_int in_pitch, tjs_uint8* in_bmpdata)
	{
		width = in_width;
		height = in_height;
		pitch = in_pitch;
		bmpdata = in_bmpdata;
	}

	/* metrics */
	tjs_uint GetWidth() const
	{
		return width;
	}
	tjs_uint GetHeight() const
	{
		return height;
	}

	/* scan line */
	const void * GetScanLine(tjs_uint l) const
	{
		if((tjs_int)l>=height )
		{
			TVPThrowExceptionMessage(TJS_W("Scan line %1 is range over (0 to %2)"), ttstr((tjs_int)l),
				ttstr((tjs_int)height-1));
		}

		return (tjs_uint8*)bmpdata + (l * pitch);
	}
	void * GetScanLineForWrite(tjs_uint l)
	{
		if((tjs_int)l>=height )
		{
			TVPThrowExceptionMessage(TJS_W("Scan line %1 is range over (0 to %2)"), ttstr((tjs_int)l),
				ttstr((tjs_int)height-1));
		}

		return (tjs_uint8*)bmpdata + (l * pitch);
	}
	tjs_int GetPitchBytes() const
	{
		return pitch;
	}
};

#endif
