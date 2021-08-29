/////////////////////////////////////////////
//                                         //
//    Copyright (C) 2021-2021 Julian Uy    //
//  https://sites.google.com/site/awertyb  //
//                                         //
//   See details of license at "LICENSE"   //
//                                         //
/////////////////////////////////////////////

#include "ncbind/ncbind.hpp"
#include "LayerBitmapIntf.h"
#include "LayerBitmapUtility.h"
#include "ComplexRect.h"
#include "ResampleImage.h"
#include <string.h>
#include <stdio.h>

//---------------------------------------------------------------------------
bool StretchBlt(
		tTVPRect cliprect,
		const tTVPBaseBitmap *dest,
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
#if 0
	tjs_int dw = destrect.get_width(), dh = destrect.get_height();
	tjs_int rw = refrect.get_width(), rh = refrect.get_height();

	if(!dw || !rw || !dh || !rh) return false; // nothing to do
#endif

#if 0
	// quick check for simple blt
	if(dw == rw && dh == rh && destrect.included_in(cliprect))
	{
		return Blt(destrect.left, destrect.top, ref, refrect, method, opa, hda);
			// no stretch; do normal blt
	}

	if(!Is32BPP()) TVPThrowExceptionMessage(TVPInvalidOperationFor8BPP);
#endif

	// check for special case noticed above
	
	//--- extract stretch type
	tTVPBBStretchType type = (tTVPBBStretchType)(mode & stTypeMask);

	//--- pull the dimension
	tjs_int w = dest->GetWidth();
	tjs_int h = dest->GetHeight();
	tjs_int refw = ref->GetWidth();
	tjs_int refh = ref->GetHeight();

	//--- clop clipping rectangle with the image
	tTVPRect cr = cliprect;
	if(cr.left < 0) cr.left = 0;
	if(cr.top < 0) cr.top = 0;
	if(cr.right > w) cr.right = w;
	if(cr.bottom > h) cr.bottom = h;

	if( type < stLinear )
	{
		type = stLinear;
	}

	TVPResampleImage( cliprect, dest, destrect, ref, refrect, type, typeopt, method, opa, hda );
	return true;

#if 0
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
#endif
}

static void PreRegistCallback()
{
	iTJSDispatch2 *global = TVPGetScriptDispatch();
	if (global)
	{
		tTJSVariant layer_val;
		static ttstr Layer_name(TJS_W("Layer"));
		global->PropGet(0, Layer_name.c_str(), Layer_name.GetHint(), &layer_val, global);
		tTJSVariantClosure layer_valclosure = layer_val.AsObjectClosureNoAddRef();
		if (layer_valclosure.Object)
		{
			layer_valclosure.DeleteMember(TJS_IGNOREPROP, TJS_W("stretchCopy"), 0, NULL);
			layer_valclosure.DeleteMember(TJS_IGNOREPROP, TJS_W("operateStretch"), 0, NULL);
		}
	}
}

NCB_PRE_REGIST_CALLBACK(PreRegistCallback);

class LayerLayerExStretch
{
public:
	static bool operateRect_passthrough(
		tTJSVariantClosure bmpobject_clo,
		tTJSVariantClosure srcbmpobject_clo,
		tTVPRect cliprect,
		tTVPRect destrect,
		tTVPRect refrect,
		tTVPBlendOperationMode mode,
		tjs_int opa
		)
	{
		tjs_int dw = destrect.get_width(), dh = destrect.get_height();
		tjs_int rw = refrect.get_width(), rh = refrect.get_height();

		if(!dw || !rw || !dh || !rh) return true; // nothing to do
		// quick check for simple blt
		if(dw == rw && dh == rh && destrect.included_in(cliprect))
		{
			{
				tTJSVariant args[9];
				args[0] = destrect.left;
				args[1] = destrect.top;
				args[2] = srcbmpobject_clo.Object;
				args[3] = refrect.left;
				args[4] = refrect.top;
				args[5] = refrect.get_width();
				args[6] = refrect.get_height();
				args[7] = (tjs_int)(mode);
				args[8] = opa;
				tTJSVariant *pargs[9] = {args +0, args +1, args +2, args +3, args +4, args +5, args +6, args +7, args +8};
				{
					iTJSDispatch2 *global = TVPGetScriptDispatch();
					if (global)
					{
						tTJSVariant layer_val;
						static ttstr Layer_name(TJS_W("Layer"));
						global->PropGet(0, Layer_name.c_str(), Layer_name.GetHint(), &layer_val, global);
						tTJSVariantClosure layer_valclosure = layer_val.AsObjectClosureNoAddRef();
						if (layer_valclosure.Object && bmpobject_clo.Object)
						{
							tTJSVariant operateRect_val;
							static ttstr operateRect_name(TJS_W("operateRect"));
							layer_valclosure.PropGet(0, operateRect_name.c_str(), operateRect_name.GetHint(), &operateRect_val, NULL);
							if (operateRect_val.Type() == tvtObject)
							{
								tTJSVariant rresult;
								tTJSVariantClosure operateRect_valclosure = operateRect_val.AsObjectClosureNoAddRef();
								if (operateRect_valclosure.Object)
								{
									operateRect_valclosure.FuncCall(0, NULL, 0, &rresult, 9, pargs, bmpobject_clo.Object);
									return true;
								}
							}
						}
					}
				}
			}
		}

		return false;
	}

	static bool operateAffine_passthrough(
		tTJSVariantClosure bmpobject_clo,
		tTJSVariantClosure srcbmpobject_clo,
		tTVPRect destrect,
		tTVPRect refrect,
		tTVPBlendOperationMode mode,
		tjs_int opa,
		tTVPBBStretchType type
		)
	{
		if( type < stLinear )
		{
			tTJSVariant args[15];
			args[0] = srcbmpobject_clo.Object;
			args[1] = refrect.left;
			args[2] = refrect.top;
			args[3] = refrect.get_width();
			args[4] = refrect.get_height();
			args[5] = 0; // points mode
			args[6] = (double)destrect.left - 0.5;
			args[7] = (double)destrect.top - 0.5;
			args[8] = (double)destrect.right - 0.5;
			args[9] = (double)destrect.top - 0.5;
			args[10] = (double)destrect.left - 0.5;
			args[11] = (double)destrect.bottom - 0.5;
			args[12] = (tjs_int)(mode);
			args[13] = opa;
			args[14] = (tjs_int)(type);
			tTJSVariant *pargs[15] = {args +0, args +1, args +2, args +3, args +4, args +5, args +6, args +7, args +8, args +9, args +10, args +11, args +12, args +13, args +14};
			iTJSDispatch2 *global = TVPGetScriptDispatch();
			if (global)
			{
				tTJSVariant layer_val;
				static ttstr Layer_name(TJS_W("Layer"));
				global->PropGet(0, Layer_name.c_str(), Layer_name.GetHint(), &layer_val, global);
				tTJSVariantClosure layer_valclosure = layer_val.AsObjectClosureNoAddRef();
				if (layer_valclosure.Object && bmpobject_clo.Object)
				{
					tTJSVariant operateAffine_val;
					static ttstr operateAffine_name(TJS_W("operateAffine"));
					layer_valclosure.PropGet(0, operateAffine_name.c_str(), operateAffine_name.GetHint(), &operateAffine_val, NULL);
					if (operateAffine_val.Type() == tvtObject)
					{
						tTJSVariant rresult;
						tTJSVariantClosure operateAffine_valclosure = operateAffine_val.AsObjectClosureNoAddRef();
						if (operateAffine_valclosure.Object)
						{
							operateAffine_valclosure.FuncCall(0, NULL, 0, &rresult, 15, pargs, bmpobject_clo.Object);
							return true;
						}
					}
				}
			}
		}
		return false;
	}

	static bool copyRect_passthrough(
		tTJSVariantClosure bmpobject_clo,
		tTJSVariantClosure srcbmpobject_clo,
		tTVPRect cliprect,
		tTVPRect destrect,
		tTVPRect refrect
		)
	{
		tjs_int dw = destrect.get_width(), dh = destrect.get_height();
		tjs_int rw = refrect.get_width(), rh = refrect.get_height();

		if(!dw || !rw || !dh || !rh) return true; // nothing to do
		// quick check for simple blt
		if(dw == rw && dh == rh && destrect.included_in(cliprect))
		{
			{
				tTJSVariant args[7];
				args[0] = destrect.left;
				args[1] = destrect.top;
				args[2] = srcbmpobject_clo.Object;
				args[3] = refrect.left;
				args[4] = refrect.top;
				args[5] = refrect.get_width();
				args[6] = refrect.get_height();
				tTJSVariant *pargs[7] = {args +0, args +1, args +2, args +3, args +4, args +5, args +6};
				iTJSDispatch2 *global = TVPGetScriptDispatch();
				if (global)
				{
					tTJSVariant layer_val;
					static ttstr Layer_name(TJS_W("Layer"));
					global->PropGet(0, Layer_name.c_str(), Layer_name.GetHint(), &layer_val, global);
					tTJSVariantClosure layer_valclosure = layer_val.AsObjectClosureNoAddRef();
					if (layer_valclosure.Object && bmpobject_clo.Object)
					{
						tTJSVariant copyRect_val;
						static ttstr copyRect_name(TJS_W("copyRect"));
						layer_valclosure.PropGet(0, copyRect_name.c_str(), copyRect_name.GetHint(), &copyRect_val, NULL);
						if (copyRect_val.Type() == tvtObject)
						{
							tTJSVariant rresult;
							tTJSVariantClosure copyRect_valclosure = copyRect_val.AsObjectClosureNoAddRef();
							if (copyRect_valclosure.Object)
							{
								copyRect_valclosure.FuncCall(0, NULL, 0, &rresult, 7, pargs, bmpobject_clo.Object);
								return true;
							}
						}
					}
				}
			}
		}

		return false;
	}

	static bool affineCopy_passthrough(
		tTJSVariantClosure bmpobject_clo,
		tTJSVariantClosure srcbmpobject_clo,
		tTVPRect destrect,
		tTVPRect refrect,
		tTVPBBStretchType type
		)
	{
		if( type < stLinear )
		{
			tTJSVariant args[14];
			args[0] = srcbmpobject_clo.Object;
			args[1] = refrect.left;
			args[2] = refrect.top;
			args[3] = refrect.get_width();
			args[4] = refrect.get_height();
			args[5] = 0; // points mode
			args[6] = (double)destrect.left - 0.5;
			args[7] = (double)destrect.top - 0.5;
			args[8] = (double)destrect.right - 0.5;
			args[9] = (double)destrect.top - 0.5;
			args[10] = (double)destrect.left - 0.5;
			args[11] = (double)destrect.bottom - 0.5;
			args[12] = (tjs_int)(type);
			args[13] = 0; // don't clear
			tTJSVariant *pargs[14] = {args +0, args +1, args +2, args +3, args +4, args +5, args +6, args +7, args +8, args +9, args +10, args +11, args +12, args +13};
			iTJSDispatch2 *global = TVPGetScriptDispatch();
			if (global)
			{
				tTJSVariant layer_val;
				static ttstr Layer_name(TJS_W("Layer"));
				global->PropGet(0, Layer_name.c_str(), Layer_name.GetHint(), &layer_val, global);
				tTJSVariantClosure layer_valclosure = layer_val.AsObjectClosureNoAddRef();
				if (layer_valclosure.Object && bmpobject_clo.Object)
				{
					tTJSVariant affineCopy_val;
					static ttstr affineCopy_name(TJS_W("affineCopy"));
					layer_valclosure.PropGet(0, affineCopy_name.c_str(), affineCopy_name.GetHint(), &affineCopy_val, NULL);
					if (affineCopy_val.Type() == tvtObject)
					{
						tTJSVariant rresult;
						tTJSVariantClosure affineCopy_valclosure = affineCopy_val.AsObjectClosureNoAddRef();
						if (affineCopy_valclosure.Object)
						{
							affineCopy_valclosure.FuncCall(0, NULL, 0, &rresult, 14, pargs, bmpobject_clo.Object);
							return true;
						}
					}
				}
			}
		}
		return false;
	}

	static tjs_error TJS_INTF_METHOD stretchCopy(
		tTJSVariant	*result,
		tjs_int numparams,
		tTJSVariant **param,
		iTJSDispatch2 *objthis)
	{
		if (numparams < 9)
		{
			return TJS_E_BADPARAMCOUNT;
		}

		if(objthis == NULL) TVPThrowExceptionMessage(TJS_W("Specify Layer or Bitmap class object"));
		tTJSVariant bmpobject = tTJSVariant(objthis, objthis);
		tTJSVariantClosure bmpobject_clo = bmpobject.AsObjectClosureNoAddRef();

		tTJSVariantClosure srcbmpobject_clo = param[4]->AsObjectClosureNoAddRef();

		tTVPRect destrect(*param[0], *param[1], *param[2], *param[3]);
		destrect.right += destrect.left;
		destrect.bottom += destrect.top;

		tTVPRect srcrect(*param[5], *param[6], *param[7], *param[8]);
		srcrect.right += srcrect.left;
		srcrect.bottom += srcrect.top;

		tTVPBBStretchType type = stNearest;
		if(numparams >= 10)
			type = (tTVPBBStretchType)(tjs_int)*param[9];

		tjs_real typeopt = 0.0;
		if(numparams >= 11)
			typeopt = (tjs_real)*param[10];
		else if( type == stFastCubic || type == stCubic )
			typeopt = -1.0;

		tTVPRect ClipRect;

		// Check bounds first
		GetLayerInformationFromLayerObject(bmpobject_clo, NULL, NULL, &ClipRect, NULL, NULL, NULL, NULL);
		// stretching copy
		tTVPRect ur = destrect;
		if(ur.right < ur.left) std::swap(ur.right, ur.left);
		if(ur.bottom < ur.top) std::swap(ur.bottom, ur.top);
		if(!TVPIntersectRect(&ur, ur, ClipRect)) return TJS_S_OK; // out of the clipping rectangle

		// source and destination check
		tjs_int dw = destrect.get_width(), dh = destrect.get_height();
		tjs_int rw = srcrect.get_width(), rh = srcrect.get_height();

		if(!dw || !rw || !dh || !rh) return TJS_S_OK; // nothing to do

		if (copyRect_passthrough(bmpobject_clo, srcbmpobject_clo, ClipRect, destrect, srcrect))
		{
			return TJS_S_OK;
		}

		if (affineCopy_passthrough(bmpobject_clo, srcbmpobject_clo, destrect, srcrect, type))
		{
			return TJS_S_OK;
		}

		// Now get information (this will independ the bitmap)
		tjs_int bmpwidth = 0;
		tjs_int bmpheight = 0;
		tjs_int bmppitch = 0;
		tjs_uint8* bmpdata = NULL;
		GetBitmapInformationFromObject(bmpobject_clo, true, &bmpwidth, &bmpheight, &bmppitch, &bmpdata);
		if(!bmpdata) TVPThrowExceptionMessage(TJS_W("Specify Layer or Bitmap class object"));
		tTVPBaseBitmap bmpinfo(bmpwidth, bmpheight, bmppitch, bmpdata);

		tjs_int srcbmpwidth = 0;
		tjs_int srcbmpheight = 0;
		tjs_int srcbmppitch = 0;
		tjs_uint8* srcbmpdata = NULL;
		GetBitmapInformationFromObject(srcbmpobject_clo, false, &srcbmpwidth, &srcbmpheight, &srcbmppitch, &srcbmpdata);
		if(!srcbmpdata) TVPThrowExceptionMessage(TJS_W("Specify Layer or Bitmap class object"));
		tTVPBaseBitmap srcbmpinfo(srcbmpwidth, srcbmpheight, srcbmppitch, srcbmpdata);

		tTVPDrawFace Face = dfAuto; // (outward) current drawing layer face
		tTVPLayerType DisplayType = ltOpaque; // actual Type
		bool HoldAlpha = false;
		tjs_int ImageLeft = 0;
		tjs_int ImageTop = 0;

		GetLayerInformationFromLayerObject(bmpobject_clo, &Face, &DisplayType, NULL, &HoldAlpha, &ImageLeft, &ImageTop, NULL);

		tTVPDrawFace DrawFace = GetDrawFace(Face, DisplayType); // (actual) current drawing layer face

		switch(DrawFace)
		{
		case dfAlpha:
		case dfAddAlpha:
			if(!bmpdata) TVPThrowExceptionMessage(TJS_W("Not drawable layer type"));
			if(!srcbmpdata) TVPThrowExceptionMessage(TJS_W("Source layer has no image"));
			StretchBlt(ClipRect, &bmpinfo, destrect, &srcbmpinfo, srcrect, bmCopy, 255, false, type, typeopt);
			break;

		case dfOpaque:
			if(!bmpdata) TVPThrowExceptionMessage(TJS_W("Not drawable layer type"));
			if(!srcbmpdata) TVPThrowExceptionMessage(TJS_W("Source layer has no image"));
			StretchBlt(ClipRect, &bmpinfo, destrect, &srcbmpinfo, srcrect, bmCopy, 255, HoldAlpha, type, typeopt);
			break;

		default:
			TVPThrowExceptionMessage(TJS_W("Not drawble face type %1"), TJS_W("stretchCopy"));
		}

		UpdateLayerWithLayerObject(bmpobject_clo, &ur, &ImageLeft, &ImageTop);
		return TJS_S_OK;
	}

	static tjs_error TJS_INTF_METHOD operateStretch(
		tTJSVariant	*result,
		tjs_int numparams,
		tTJSVariant **param,
		iTJSDispatch2 *objthis)
	{
		if (numparams < 9)
		{
			return TJS_E_BADPARAMCOUNT;
		}

		if(objthis == NULL) TVPThrowExceptionMessage(TJS_W("Specify Layer or Bitmap class object"));
		tTJSVariant bmpobject = tTJSVariant(objthis, objthis);
		tTJSVariantClosure bmpobject_clo = bmpobject.AsObjectClosureNoAddRef();

		tTJSVariantClosure srcbmpobject_clo = param[4]->AsObjectClosureNoAddRef();

		tTVPRect destrect(*param[0], *param[1], *param[2], *param[3]);
		destrect.right += destrect.left;
		destrect.bottom += destrect.top;

		tTVPRect srcrect(*param[5], *param[6], *param[7], *param[8]);
		srcrect.right += srcrect.left;
		srcrect.bottom += srcrect.top;

		tTVPBlendOperationMode mode;
		if(numparams >= 10 && param[9]->Type() != tvtVoid)
			mode = (tTVPBlendOperationMode)(tjs_int)(*param[9]);
		else
			mode = omAuto;

		tjs_int opa = 255;

		if(numparams >= 11 && param[10]->Type() != tvtVoid)
			opa = *param[10];

		tTVPBBStretchType type = stNearest;
		if(numparams >= 12 && param[11]->Type() != tvtVoid)
			type = (tTVPBBStretchType)(tjs_int)*param[11];

		tjs_real typeopt = 0.0;
		if(numparams >= 13)
			typeopt = (tjs_real)*param[12];
		else if( type == stFastCubic || type == stCubic )
			typeopt = -1.0;

		tTVPRect ClipRect;

		// Check bounds first
		GetLayerInformationFromLayerObject(bmpobject_clo, NULL, NULL, &ClipRect, NULL, NULL, NULL, NULL);
		// stretching copy
		tTVPRect ur = destrect;
		if(ur.right < ur.left) std::swap(ur.right, ur.left);
		if(ur.bottom < ur.top) std::swap(ur.bottom, ur.top);
		if(!TVPIntersectRect(&ur, ur, ClipRect)) return TJS_S_OK; // out of the clipping rectangle

		// source and destination check
		tjs_int dw = destrect.get_width(), dh = destrect.get_height();
		tjs_int rw = srcrect.get_width(), rh = srcrect.get_height();

		if(!dw || !rw || !dh || !rh) return TJS_S_OK; // nothing to do

		if (operateRect_passthrough(bmpobject_clo, srcbmpobject_clo, ClipRect, destrect, srcrect, mode, opa))
		{
			return TJS_S_OK;
		}

		if (operateAffine_passthrough(bmpobject_clo, srcbmpobject_clo, ur, srcrect, mode, opa, type))
		{
			return TJS_S_OK;
		}

		// Now get information (this will independ the bitmap)
		tjs_int bmpwidth = 0;
		tjs_int bmpheight = 0;
		tjs_int bmppitch = 0;
		tjs_uint8* bmpdata = NULL;
		GetBitmapInformationFromObject(bmpobject_clo, true, &bmpwidth, &bmpheight, &bmppitch, &bmpdata);
		if(!bmpdata) TVPThrowExceptionMessage(TJS_W("Specify Layer or Bitmap class object"));
		tTVPBaseBitmap bmpinfo(bmpwidth, bmpheight, bmppitch, bmpdata);

		tjs_int srcbmpwidth = 0;
		tjs_int srcbmpheight = 0;
		tjs_int srcbmppitch = 0;
		tjs_uint8* srcbmpdata = NULL;
		GetBitmapInformationFromObject(srcbmpobject_clo, false, &srcbmpwidth, &srcbmpheight, &srcbmppitch, &srcbmpdata);
		if(!srcbmpdata) TVPThrowExceptionMessage(TJS_W("Specify Layer or Bitmap class object"));
		tTVPBaseBitmap srcbmpinfo(srcbmpwidth, srcbmpheight, srcbmppitch, srcbmpdata);

		tTVPDrawFace Face = dfAuto; // (outward) current drawing layer face
		tTVPLayerType DisplayType = ltOpaque; // actual Type
		bool HoldAlpha = false;
		tjs_int ImageLeft = 0;
		tjs_int ImageTop = 0;

		GetLayerInformationFromLayerObject(bmpobject_clo, &Face, &DisplayType, NULL, &HoldAlpha, &ImageLeft, &ImageTop, NULL);

		// get correct blend mode if the mode is omAuto
		if(mode == omAuto) mode = GetOperationModeFromType(DisplayType);

		tTVPDrawFace DrawFace = GetDrawFace(Face, DisplayType); // (actual) current drawing layer face

		// convert tTVPBlendOperationMode to tTVPBBBltMethod
		tTVPBBBltMethod met;
		if(!GetBltMethodFromOperationModeAndDrawFace(DrawFace, met, mode))
		{
			// unknown blt mode
			TVPThrowExceptionMessage(TJS_W("Not drawble face type %1"), TJS_W("operateStretch"));
		}

		if(!bmpdata) TVPThrowExceptionMessage(TJS_W("Not drawable layer type"));
		if(!srcbmpdata) TVPThrowExceptionMessage(TJS_W("Source layer has no image"));
		StretchBlt(ClipRect, &bmpinfo, destrect, &srcbmpinfo, srcrect, met, opa, HoldAlpha, type, typeopt);

		UpdateLayerWithLayerObject(bmpobject_clo, &ur, &ImageLeft, &ImageTop);
		return TJS_S_OK;
	}
};

NCB_ATTACH_CLASS(LayerLayerExStretch, Layer)
{
	RawCallback("stretchCopy", &Class::stretchCopy, 0);
	RawCallback("operateStretch", &Class::operateStretch, 0);
};
