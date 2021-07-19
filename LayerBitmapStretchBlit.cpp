
//---------------------------------------------------------------------------
bool tTVPBaseBitmap::StretchBlt(tTVPRect cliprect,
		tTVPRect destrect, const tTVPBaseBitmap *ref,
		tTVPRect refrect, tTVPBBBltMethod method, tjs_int opa,
			bool hda, tTVPBBStretchType mode, tjs_real typeopt )
{
	// do stretch blt
	// stFastLinear is enabled only in following condition:
	// -------TODO: write corresponding condition--------

	// stLinear and stCubic mode are enabled only in following condition:
	// any magnification, opa:255, method:bmCopy, hda:false
	// no reverse, destination rectangle is within the image.

	// source and destination check
	tjs_int dw = destrect.get_width(), dh = destrect.get_height();
	tjs_int rw = refrect.get_width(), rh = refrect.get_height();

	if(!dw || !rw || !dh || !rh) return false; // nothing to do

#if 0
	// quick check for simple blt
	if(dw == rw && dh == rh && destrect.included_in(cliprect))
	{
		return Blt(destrect.left, destrect.top, ref, refrect, method, opa, hda);
			// no stretch; do normal blt
	}
#endif

	if(!Is32BPP()) TVPThrowExceptionMessage(TVPInvalidOperationFor8BPP);

	// check for special case noticed above
	
	//--- extract stretch type
	tTVPBBStretchType type = (tTVPBBStretchType)(mode & stTypeMask);

	//--- pull the dimension
	tjs_int w = GetWidth();
	tjs_int h = GetHeight();
	tjs_int refw = ref->GetWidth();
	tjs_int refh = ref->GetHeight();

	//--- clop clipping rectangle with the image
	tTVPRect cr = cliprect;
	if(cr.left < 0) cr.left = 0;
	if(cr.top < 0) cr.top = 0;
	if(cr.right > w) cr.right = w;
	if(cr.bottom > h) cr.bottom = h;

	//--- check mode and other conditions
	if( type >= stLinear )
	{
		// takes another routine
		TVPResampleImage( cliprect, this, destrect, ref, refrect, type, typeopt, method, opa, hda );
		return true;
	}



	// pass to affine operation routine
	tTVPPointD points[3];
	points[0].x = (double)destrect.left - 0.5;
	points[0].y = (double)destrect.top - 0.5;
	points[1].x = (double)destrect.right - 0.5;
	points[1].y = (double)destrect.top - 0.5;
	points[2].x = (double)destrect.left - 0.5;
	points[2].y = (double)destrect.bottom - 0.5;
	return AffineBlt(cliprect, ref, refrect, points, method,
					opa, NULL, hda, mode, false, 0);
}

