
#include "LayerBitmapUtility.h"

bool GetBltMethodFromOperationModeAndDrawFace(
		tTVPDrawFace drawface,
		tTVPBBBltMethod & result,
		tTVPBlendOperationMode mode)
{
	// resulting corresponding  tTVPBBBltMethod value of mode and current drawface.
	// returns whether the method is known.
	tTVPBBBltMethod met;
	bool met_set = false;
	switch(mode)
	{
	case omPsNormal:			met_set = true; met = bmPsNormal;			break;
	case omPsAdditive:			met_set = true; met = bmPsAdditive;			break;
	case omPsSubtractive:		met_set = true; met = bmPsSubtractive;		break;
	case omPsMultiplicative:	met_set = true; met = bmPsMultiplicative;	break;
	case omPsScreen:			met_set = true; met = bmPsScreen;			break;
	case omPsOverlay:			met_set = true; met = bmPsOverlay;			break;
	case omPsHardLight:			met_set = true; met = bmPsHardLight;		break;
	case omPsSoftLight:			met_set = true; met = bmPsSoftLight;		break;
	case omPsColorDodge:		met_set = true; met = bmPsColorDodge;		break;
	case omPsColorDodge5:		met_set = true; met = bmPsColorDodge5;		break;
	case omPsColorBurn:			met_set = true; met = bmPsColorBurn;		break;
	case omPsLighten:			met_set = true; met = bmPsLighten;			break;
	case omPsDarken:			met_set = true; met = bmPsDarken;			break;
	case omPsDifference:   		met_set = true; met = bmPsDifference;		break;
	case omPsDifference5:   	met_set = true; met = bmPsDifference5;		break;
	case omPsExclusion:			met_set = true; met = bmPsExclusion;		break;
	case omAdditive:			met_set = true; met = bmAdd;				break;
	case omSubtractive:			met_set = true; met = bmSub;				break;
	case omMultiplicative:		met_set = true; met = bmMul;				break;
	case omDodge:				met_set = true; met = bmDodge;				break;
	case omDarken:				met_set = true; met = bmDarken;				break;
	case omLighten:				met_set = true; met = bmLighten;			break;
	case omScreen:				met_set = true; met = bmScreen;				break;
	case omAlpha:
		if(drawface == dfAlpha)
						{	met_set = true; met = bmAlphaOnAlpha; break;		}
		else if(drawface == dfAddAlpha)
						{	met_set = true; met = bmAlphaOnAddAlpha; break;		}
		else if(drawface == dfOpaque)
						{	met_set = true; met = bmAlpha; break;				}
		break;
	case omAddAlpha:
		if(drawface == dfAlpha)
						{	met_set = true; met = bmAddAlphaOnAlpha; break;		}
		else if(drawface == dfAddAlpha)
						{	met_set = true; met = bmAddAlphaOnAddAlpha; break;	}
		else if(drawface == dfOpaque)
						{	met_set = true; met = bmAddAlpha; break;			}
		break;
	case omOpaque:
		if(drawface == dfAlpha)
						{	met_set = true; met = bmCopyOnAlpha; break;			}
		else if(drawface == dfAddAlpha)
						{	met_set = true; met = bmCopyOnAddAlpha; break;		}
		else if(drawface == dfOpaque)
						{	met_set = true; met = bmCopy; break;				}
		break;
	}

	result = met;

	return met_set;
}

tTVPDrawFace GetDrawFace(tTVPDrawFace Face, tTVPLayerType DisplayType)
{
	tTVPDrawFace DrawFace;
	// set DrawFace from Face and Type
	if(Face == dfAuto)
	{
		// DrawFace is chosen automatically from the layer type
		switch(DisplayType)
		{
	//	case ltBinder:
		case ltOpaque:				DrawFace = dfOpaque;			break;
		case ltAlpha:				DrawFace = dfAlpha;				break;
		case ltAdditive:			DrawFace = dfOpaque;			break;
		case ltSubtractive:			DrawFace = dfOpaque;			break;
		case ltMultiplicative:		DrawFace = dfOpaque;			break;
		case ltDodge:				DrawFace = dfOpaque;			break;
		case ltDarken:				DrawFace = dfOpaque;			break;
		case ltLighten:				DrawFace = dfOpaque;			break;
		case ltScreen:				DrawFace = dfOpaque;			break;
		case ltAddAlpha:			DrawFace = dfAddAlpha;			break;
		case ltPsNormal:			DrawFace = dfAlpha;				break;
		case ltPsAdditive:			DrawFace = dfAlpha;				break;
		case ltPsSubtractive:		DrawFace = dfAlpha;				break;
		case ltPsMultiplicative:	DrawFace = dfAlpha;				break;
		case ltPsScreen:			DrawFace = dfAlpha;				break;
		case ltPsOverlay:			DrawFace = dfAlpha;				break;
		case ltPsHardLight:			DrawFace = dfAlpha;				break;
		case ltPsSoftLight:			DrawFace = dfAlpha;				break;
		case ltPsColorDodge:		DrawFace = dfAlpha;				break;
		case ltPsColorDodge5:		DrawFace = dfAlpha;				break;
		case ltPsColorBurn:			DrawFace = dfAlpha;				break;
		case ltPsLighten:			DrawFace = dfAlpha;				break;
		case ltPsDarken:			DrawFace = dfAlpha;				break;
		case ltPsDifference:	 	DrawFace = dfAlpha;				break;
		case ltPsDifference5:	 	DrawFace = dfAlpha;				break;
		case ltPsExclusion:			DrawFace = dfAlpha;				break;
		default:
							DrawFace = dfOpaque;			break;
		}
	}
	else
	{
		DrawFace = Face;
	}
	return DrawFace;
}

tTVPBlendOperationMode GetOperationModeFromType(tTVPLayerType DisplayType)
{
	// returns corresponding blend operation mode from layer type

	switch(DisplayType)
	{
//	case ltBinder:
	case ltOpaque:			return omOpaque;			 
	case ltAlpha:			return omAlpha;
	case ltAdditive:		return omAdditive;
	case ltSubtractive:		return omSubtractive;
	case ltMultiplicative:	return omMultiplicative;	 
	case ltDodge:			return omDodge;
	case ltDarken:			return omDarken;
	case ltLighten:			return omLighten;
	case ltScreen:			return omScreen;
	case ltAddAlpha:		return omAddAlpha;
	case ltPsNormal:		return omPsNormal;
	case ltPsAdditive:		return omPsAdditive;
	case ltPsSubtractive:	return omPsSubtractive;
	case ltPsMultiplicative:return omPsMultiplicative;
	case ltPsScreen:		return omPsScreen;
	case ltPsOverlay:		return omPsOverlay;
	case ltPsHardLight:		return omPsHardLight;
	case ltPsSoftLight:		return omPsSoftLight;
	case ltPsColorDodge:	return omPsColorDodge;
	case ltPsColorDodge5:	return omPsColorDodge5;
	case ltPsColorBurn:		return omPsColorBurn;
	case ltPsLighten:		return omPsLighten;
	case ltPsDarken:		return omPsDarken;
	case ltPsDifference:	return omPsDifference;
	case ltPsDifference5:	return omPsDifference5;
	case ltPsExclusion:		return omPsExclusion;


	default:
							return omOpaque;
	}
}

void GetBitmapInformationFromObject(tTJSVariantClosure clo, bool for_write, tjs_int *bmpwidth, tjs_int *bmpheight, tjs_int *bmppitch, tjs_uint8 **bmpdata)
{
	iTJSDispatch2 *global = TVPGetScriptDispatch();
	if (global)
	{
		tTJSVariant layer_val;
		static ttstr Layer_name(TJS_W("Layer"));
		global->PropGet(0, Layer_name.c_str(), Layer_name.GetHint(), &layer_val, global);
		tTJSVariantClosure layer_valclosure = layer_val.AsObjectClosureNoAddRef();
		if (layer_valclosure.Object && clo.Object)
		{
			tTJSVariant val;
			if (bmpwidth)
			{
				static ttstr imageWidth_name(TJS_W("imageWidth"));
				if (TJS_FAILED(layer_valclosure.PropGet(0, imageWidth_name.c_str(), imageWidth_name.GetHint(), &val, clo.Object)))
				{
					static ttstr width_name(TJS_W("width"));
					clo.PropGet(0, width_name.c_str(), width_name.GetHint(), &val, clo.Object);
				}
				*bmpwidth = (tjs_uint)(tTVInteger)val;
			}
			if (bmpheight)
			{
				static ttstr imageHeight_name(TJS_W("imageHeight"));
				if (TJS_FAILED(layer_valclosure.PropGet(0, imageHeight_name.c_str(), imageHeight_name.GetHint(), &val, clo.Object)))
				{
					static ttstr height(TJS_W("height"));
					clo.PropGet(0, height.c_str(), height.GetHint(), &val, clo.Object);
				}
				*bmpheight = (tjs_uint)(tTVInteger)val;
			}
			if (bmppitch)
			{
				static ttstr mainImageBufferPitch_name(TJS_W("mainImageBufferPitch"));
				if (TJS_FAILED(layer_valclosure.PropGet(0, mainImageBufferPitch_name.c_str(), mainImageBufferPitch_name.GetHint(), &val, clo.Object)))
				{
					static ttstr bufferPitch_name(TJS_W("bufferPitch"));
					clo.PropGet(0, bufferPitch_name.c_str(), bufferPitch_name.GetHint(), &val, clo.Object);
				}
				*bmppitch = (tjs_int)val;
			}
			if (bmpdata)
			{
				if (for_write)
				{
					static ttstr mainImageBufferForWrite_name(TJS_W("mainImageBufferForWrite"));
					if (TJS_FAILED(layer_valclosure.PropGet(0, mainImageBufferForWrite_name.c_str(), mainImageBufferForWrite_name.GetHint(), &val, clo.Object)))
					{
						static ttstr bufferForWrite_name(TJS_W("bufferForWrite"));
						clo.PropGet(0, bufferForWrite_name.c_str(), bufferForWrite_name.GetHint(), &val, clo.Object);
					}
				}
				else
				{
					static ttstr mainImageBuffer_name(TJS_W("mainImageBuffer"));
					if (TJS_FAILED(layer_valclosure.PropGet(0, mainImageBuffer_name.c_str(), mainImageBuffer_name.GetHint(), &val, clo.Object)))
					{
						static ttstr buffer_name(TJS_W("buffer"));
						clo.PropGet(0, buffer_name.c_str(), buffer_name.GetHint(), &val, clo.Object);
					}
				}
				*bmpdata = reinterpret_cast<tjs_uint8*>((tjs_intptr_t)(tjs_int64)val);
			}
		}
	}
}

void GetProvinceBitmapInformationFromObject(tTJSVariantClosure clo, bool for_write, tjs_int *bmpwidth, tjs_int *bmpheight, tjs_int *bmppitch, tjs_uint8 **bmpdata)
{
	iTJSDispatch2 *global = TVPGetScriptDispatch();
	if (global)
	{
		tTJSVariant layer_val;
		static ttstr Layer_name(TJS_W("Layer"));
		global->PropGet(0, Layer_name.c_str(), Layer_name.GetHint(), &layer_val, global);
		tTJSVariantClosure layer_valclosure = layer_val.AsObjectClosureNoAddRef();
		if (layer_valclosure.Object && clo.Object)
		{
			tTJSVariant val;
			if (bmpwidth)
			{
				static ttstr imageWidth_name(TJS_W("imageWidth"));
				layer_valclosure.PropGet(0, imageWidth_name.c_str(), imageWidth_name.GetHint(), &val, clo.Object);
				*bmpwidth = (tjs_uint)(tTVInteger)val;
			}
			if (bmpheight)
			{
				static ttstr imageHeight_name(TJS_W("imageHeight"));
				layer_valclosure.PropGet(0, imageHeight_name.c_str(), imageHeight_name.GetHint(), &val, clo.Object);
				*bmpheight = (tjs_uint)(tTVInteger)val;
			}
			if (bmppitch)
			{
				static ttstr provinceImageBufferPitch_name(TJS_W("provinceImageBufferPitch"));
				layer_valclosure.PropGet(0, provinceImageBufferPitch_name.c_str(), provinceImageBufferPitch_name.GetHint(), &val, clo.Object);
				*bmppitch = (tjs_int)val;
			}
			if (bmpdata)
			{
				if (for_write)
				{
					static ttstr provinceImageBufferForWrite_name(TJS_W("provinceImageBufferForWrite"));
					layer_valclosure.PropGet(0, provinceImageBufferForWrite_name.c_str(), provinceImageBufferForWrite_name.GetHint(), &val, clo.Object);
				}
				else
				{
					static ttstr provinceImageBuffer_name(TJS_W("provinceImageBuffer"));
					layer_valclosure.PropGet(0, provinceImageBuffer_name.c_str(), provinceImageBuffer_name.GetHint(), &val, clo.Object);
				}
				*bmpdata = reinterpret_cast<tjs_uint8*>((tjs_intptr_t)(tjs_int64)val);
			}
		}
	}
}

void GetLayerInformationFromLayerObject(tTJSVariantClosure clo, tTVPDrawFace *Face, tTVPLayerType *DisplayType, tTVPRect *ClipRect, bool *HoldAlpha, tjs_int *ImageLeft, tjs_int *ImageTop, tjs_uint32 *NeutralColor)
{
	iTJSDispatch2 *global = TVPGetScriptDispatch();
	if (global)
	{
		tTJSVariant layer_val;
		static ttstr Layer_name(TJS_W("Layer"));
		global->PropGet(0, Layer_name.c_str(), Layer_name.GetHint(), &layer_val, global);
		tTJSVariantClosure layer_valclosure = layer_val.AsObjectClosureNoAddRef();
		if (layer_valclosure.Object && clo.Object)
		{
			tTJSVariant val;

			if (Face)
			{
				static ttstr face_name(TJS_W("face"));
				layer_valclosure.PropGet(0, face_name.c_str(), face_name.GetHint(), &val, clo.Object);
				*Face = (tTVPDrawFace)(tjs_int)val;
			}
			if (DisplayType)
			{
				static ttstr type_name(TJS_W("type"));
				layer_valclosure.PropGet(0, type_name.c_str(), type_name.GetHint(), &val, clo.Object);
				*DisplayType = (tTVPLayerType)(tjs_int)val;
			}

			if (ClipRect)
			{
				static ttstr clipLeft_name(TJS_W("clipLeft"));
				layer_valclosure.PropGet(0, clipLeft_name.c_str(), clipLeft_name.GetHint(), &val, clo.Object);
				ClipRect->left = (tjs_int)val;
				static ttstr clipTop_name(TJS_W("clipTop"));
				layer_valclosure.PropGet(0, clipTop_name.c_str(), clipTop_name.GetHint(), &val, clo.Object);
				ClipRect->top = (tjs_int)val;
				static ttstr clipWidth_name(TJS_W("clipWidth"));
				layer_valclosure.PropGet(0, clipWidth_name.c_str(), clipWidth_name.GetHint(), &val, clo.Object);
				ClipRect->right = ClipRect->left + ((tjs_int)val);
				static ttstr clipHeight_name(TJS_W("clipHeight"));
				layer_valclosure.PropGet(0, clipHeight_name.c_str(), clipHeight_name.GetHint(), &val, clo.Object);
				ClipRect->bottom = ClipRect->top + ((tjs_int)val);
			}

			if (HoldAlpha)
			{
				static ttstr holdAlpha_name(TJS_W("holdAlpha"));
				layer_valclosure.PropGet(0, holdAlpha_name.c_str(), holdAlpha_name.GetHint(), &val, clo.Object);
				*HoldAlpha = ((tjs_int)val) != 0;
			}

			if (ImageLeft)
			{
				static ttstr imageLeft_name(TJS_W("imageLeft"));
				layer_valclosure.PropGet(0, imageLeft_name.c_str(), imageLeft_name.GetHint(), &val, clo.Object);
				*ImageLeft = (tjs_int)val;
			}
			if (ImageTop)
			{
				static ttstr imageTop_name(TJS_W("imageTop"));
				layer_valclosure.PropGet(0, imageTop_name.c_str(), imageTop_name.GetHint(), &val, clo.Object);
				*ImageTop = (tjs_int)val;
			}

			if (NeutralColor)
			{
				static ttstr neutralColor_name(TJS_W("neutralColor"));
				layer_valclosure.PropGet(0, neutralColor_name.c_str(), neutralColor_name.GetHint(), &val, clo.Object);
				*NeutralColor = ((tjs_int64)val) & 0xffffffff;
			}
		}
	}
}

void SetLayerInformationOnLayerObject(tTJSVariantClosure clo, tTVPDrawFace *Face, tTVPLayerType *DisplayType, tTVPRect *ClipRect, bool *HoldAlpha, tjs_int *ImageLeft, tjs_int *ImageTop, tjs_uint32 *NeutralColor)
{
	iTJSDispatch2 *global = TVPGetScriptDispatch();
	if (global)
	{
		tTJSVariant layer_val;
		static ttstr Layer_name(TJS_W("Layer"));
		global->PropGet(0, Layer_name.c_str(), Layer_name.GetHint(), &layer_val, global);
		tTJSVariantClosure layer_valclosure = layer_val.AsObjectClosureNoAddRef();
		if (layer_valclosure.Object && clo.Object)
		{
			tTJSVariant val;

			if (Face)
			{
				static ttstr face_name(TJS_W("face"));
				val = (tjs_int)*Face;
				layer_valclosure.PropSet(0, face_name.c_str(), face_name.GetHint(), &val, clo.Object);
			}
			if (DisplayType)
			{
				static ttstr type_name(TJS_W("type"));
				val = (tjs_int)*DisplayType;
				layer_valclosure.PropSet(0, type_name.c_str(), type_name.GetHint(), &val, clo.Object);
			}

			if (ClipRect)
			{
				static ttstr clipLeft_name(TJS_W("clipLeft"));
				val = (tjs_int)(ClipRect->left);
				layer_valclosure.PropSet(0, clipLeft_name.c_str(), clipLeft_name.GetHint(), &val, clo.Object);
				static ttstr clipTop_name(TJS_W("clipTop"));
				val = (tjs_int)(ClipRect->top);
				layer_valclosure.PropSet(0, clipTop_name.c_str(), clipTop_name.GetHint(), &val, clo.Object);
				static ttstr clipWidth_name(TJS_W("clipWidth"));
				val = (tjs_int)(ClipRect->right - ClipRect->left);
				layer_valclosure.PropSet(0, clipWidth_name.c_str(), clipWidth_name.GetHint(), &val, clo.Object);
				static ttstr clipHeight_name(TJS_W("clipHeight"));
				val = (tjs_int)(ClipRect->bottom - ClipRect->top);
				layer_valclosure.PropSet(0, clipHeight_name.c_str(), clipHeight_name.GetHint(), &val, clo.Object);
			}

			if (HoldAlpha)
			{
				static ttstr holdAlpha_name(TJS_W("holdAlpha"));
				val = (tjs_int)(*HoldAlpha ? 1 : 0);
				layer_valclosure.PropSet(0, holdAlpha_name.c_str(), holdAlpha_name.GetHint(), &val, clo.Object);
			}

			if (ImageLeft)
			{
				static ttstr imageLeft_name(TJS_W("imageLeft"));
				val = (tjs_int)*ImageLeft;
				layer_valclosure.PropSet(0, imageLeft_name.c_str(), imageLeft_name.GetHint(), &val, clo.Object);
			}
			if (ImageTop)
			{
				static ttstr imageTop_name(TJS_W("imageTop"));
				val = (tjs_int)*ImageTop;
				layer_valclosure.PropSet(0, imageTop_name.c_str(), imageTop_name.GetHint(), &val, clo.Object);
			}

			if (NeutralColor)
			{
				static ttstr neutralColor_name(TJS_W("neutralColor"));
				val = (tjs_int64)*NeutralColor;
				layer_valclosure.PropSet(0, neutralColor_name.c_str(), neutralColor_name.GetHint(), &val, clo.Object);
			}
		}
	}
}

void UpdateLayerWithLayerObject(tTJSVariantClosure clo, tTVPRect *ur, tjs_int *ImageLeft, tjs_int *ImageTop)
{
	tTVPRect update_rect = *ur;
	if (ImageLeft != 0 || ImageTop != 0)
	{
		update_rect.add_offsets(*ImageLeft, *ImageTop);
	}
	if (!update_rect.is_empty())
	{
		tTJSVariant args[4];
		args[0] = (tjs_int)update_rect.left;
		args[1] = (tjs_int)update_rect.top;
		args[2] = (tjs_int)update_rect.get_width();
		args[3] = (tjs_int)update_rect.get_height();
		tTJSVariant *pargs[4] = {args +0, args +1, args +2, args +3};
		if (clo.Object)
		{
			static ttstr update_name(TJS_W("update"));
			clo.FuncCall(0, update_name.c_str(), update_name.GetHint(), NULL, 4, pargs, NULL);
		}
	}
}

void UpdateWholeLayerWithLayerObject(tTJSVariantClosure clo)
{
	{
		if (clo.Object)
		{
			static ttstr update_name(TJS_W("update"));
			clo.FuncCall(0, update_name.c_str(), update_name.GetHint(), NULL, 0, NULL, NULL);
		}
	}
}
