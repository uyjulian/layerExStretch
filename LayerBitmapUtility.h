
#include "tjsCommHead.h"
#include "LayerBitmapIntf.h"

extern bool GetBltMethodFromOperationModeAndDrawFace(
		tTVPDrawFace drawface,
		tTVPBBBltMethod & result,
		tTVPBlendOperationMode mode);

extern tTVPDrawFace GetDrawFace(tTVPDrawFace Face, tTVPLayerType DisplayType);

extern tTVPBlendOperationMode GetOperationModeFromType(tTVPLayerType DisplayType);

extern void GetBitmapInformationFromObject(tTJSVariantClosure clo, bool for_write, tjs_int *bmpwidth, tjs_int *bmpheight, tjs_int *bmppitch, tjs_uint8 **bmpdata);

extern void GetProvinceBitmapInformationFromObject(tTJSVariantClosure clo, bool for_write, tjs_int *bmpwidth, tjs_int *bmpheight, tjs_int *bmppitch, tjs_uint8 **bmpdata);

extern void GetLayerInformationFromLayerObject(tTJSVariantClosure clo, tTVPDrawFace *Face, tTVPLayerType *DisplayType, tTVPRect *ClipRect, bool *HoldAlpha, tjs_int *ImageLeft, tjs_int *ImageTop, tjs_uint32 *NeutralColor);

extern void SetLayerInformationOnLayerObject(tTJSVariantClosure clo, tTVPDrawFace *Face, tTVPLayerType *DisplayType, tTVPRect *ClipRect, bool *HoldAlpha, tjs_int *ImageLeft, tjs_int *ImageTop, tjs_uint32 *NeutralColor);

extern void UpdateLayerWithLayerObject(tTJSVariantClosure clo, tTVPRect *ur, tjs_int *ImageLeft, tjs_int *ImageTop);

extern void UpdateWholeLayerWithLayerObject(tTJSVariantClosure clo);
