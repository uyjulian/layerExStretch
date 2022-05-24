
// Shim class for compatibilty purposes.
#ifndef __LAYERBITMAPINTF_H__
#define __LAYERBITMAPINTF_H__

class tTVPBaseBitmap
{
	tjs_uint width;
	tjs_uint height;
	tjs_int pitch;
	tjs_uint8* bmpdata;
	bool is_32bit;
public:
	tTVPBaseBitmap(tjs_uint in_width, tjs_uint in_height, tjs_int in_pitch, tjs_uint8* in_bmpdata, bool in_is_32bit = true)
	{
		width = in_width;
		height = in_height;
		pitch = in_pitch;
		bmpdata = in_bmpdata;
		is_32bit = in_is_32bit;
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
		if(l>=height )
		{
			TVPThrowExceptionMessage(TJS_W("Scan line %1 is range over (0 to %2)"), ttstr((tjs_int)l),
				ttstr((tjs_int)height-1));
		}

		return (tjs_uint8*)bmpdata + (pitch * (tjs_int)l);
	}
	void * GetScanLineForWrite(tjs_uint l)
	{
		if(l>=height )
		{
			TVPThrowExceptionMessage(TJS_W("Scan line %1 is range over (0 to %2)"), ttstr((tjs_int)l),
				ttstr((tjs_int)height-1));
		}

		return (tjs_uint8*)bmpdata + (pitch * (tjs_int)l);
	}
	tjs_int GetPitchBytes() const
	{
		return pitch;
	}
	bool Is32BPP() const
	{
		return is_32bit;
	}
};

#endif
