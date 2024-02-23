/////////////////////////////////////////////
//                                         //
//    Copyright (C) 2023-2023 Julian Uy    //
//  https://sites.google.com/site/awertyb  //
//                                         //
//   See details of license at "LICENSE"   //
//                                         //
/////////////////////////////////////////////

#include <math.h>
#include <stdint.h>
float roundevenf(float v)
{
	float rounded = roundf(v);
	float diff = rounded - v;
	if ((fabsf(diff) == 0.5f) && (((int32_t)rounded) & 1))
	{
		rounded = v - diff;
	}
	return rounded;
}
