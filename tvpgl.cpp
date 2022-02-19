// Copied tvpgl functions, because we can't rely on the core to have them.

#include "tvpgl.h"

#include <cmath>

static unsigned char TVPOpacityOnOpacityTable[256*256];
static unsigned char TVPNegativeMulTable[256*256];
static tjs_uint32 TVPRecipTable256[256]; /* 1/x  table  ( 65536 ) multiplied */
static unsigned char TVPPsTableSoftLight[256][256];
static unsigned char TVPPsTableColorDodge[256][256];
static unsigned char TVPPsTableColorBurn[256][256];

TVP_GL_FUNC_STATIC_DECL(void, TVPInitDitherTable, (void))
{
	tjs_int i;

	/* create TVPRecipTable256 */
	TVPRecipTable256[0] = 65536;
	for(i = 1; i < 256; i++)
	{
		TVPRecipTable256[i] = 65536/i;
	}
}

TVP_GL_FUNC_STATIC_DECL(void, TVPPsMakeTable, (void))
{
	int s, d;
	for (s=0; s<256; s++) {
		for (d=0; d<256; d++) {
			TVPPsTableSoftLight[s][d]  = (s>=128) ?
				( ((unsigned char)(pow(d/255.0, 128.0/s)*255.0)) ) :
				( ((unsigned char)(pow(d/255.0, (1.0-s/255.0)/0.5)*255.0)) );
			TVPPsTableColorDodge[s][d] = ((255-s)<=d) ? (0xff) : ((d*255)/(255-s));
			TVPPsTableColorBurn[s][d]  = (s<=(255-d)) ? (0x00) : (255-((255-d)*255)/s);
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPCreateTable, (void))
{
	int a,b;

	for(a=0; a<256; a++)
	{
		for(b=0; b<256; b++)
		{
			float c;
			int ci;
			int addr = b*256+ a;

			if(a)
			{
				float at = (float)(a/255.0), bt = (float)(b/255.0);
				c = bt / at;
				c /= (float)( (1.0 - bt + c) );
				ci = (int)(c*255);
				if(ci>=256) ci = 255; /* will not overflow... */
			}
			else
			{
				ci=255;
			}

			TVPOpacityOnOpacityTable[addr]=(unsigned char)ci;
				/* higher byte of the index is source opacity */
				/* lower byte of the index is destination opacity */
		
			TVPNegativeMulTable[addr] = (unsigned char)
				( 255 - (255-a)*(255-b)/ 255 ); 
		}
	}

	TVP_GL_FUNCNAME(TVPInitDitherTable)();
	TVP_GL_FUNCNAME(TVPPsMakeTable)();
}

TVP_GL_FUNC_DECL(void, TVPAddBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	register tjs_uint32 tmp;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			tmp = (  ( *src & *dest ) + ( ((*src^*dest)>>1u) & 0x7f7f7f7fu)  ) & 0x80808080u;
			tmp = (tmp<<1u) - (tmp>>7u);
			*dest= (*src + *dest - tmp) | tmp;
			dest++;
			src++;
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPAddBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	register tjs_uint32 tmp;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			tmp = (  ( *src & *dest ) + ( ((*src^*dest)>>1u) & 0x7f7f7f7fu)  ) & 0x80808080u;
			tmp = (tmp<<1u) - (tmp>>7u);
			*dest= (((*src + *dest - tmp) | tmp) & 0xffffffu) | (*dest & 0xff000000u) ;
			dest++;
			src++;
		}

	}
}

TVP_GL_FUNC_DECL(void, TVPAddBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	register tjs_uint32 s;
	register tjs_uint32 tmp;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = ( ((*src&0x00ff00u)  * opa >> 8u)&0x00ff00u) +
				(( (*src&0xff00ffu) * opa >> 8u)&0xff00ffu);
			tmp = (  ( s & *dest ) + ( ((s^*dest)>>1u) & 0x7f7f7f7fu)  ) & 0x80808080u;
			src++;
			tmp = (tmp<<1u) - (tmp>>7u);
			*dest= (((s + *dest - tmp) | tmp) & 0xffffffu) + (*dest & 0xff000000u) ;
			dest++;
		}

	}
}

TVP_GL_FUNC_INLINE_DECL(tjs_uint32, TVPMulColor, (tjs_uint32 color, tjs_uint32 fac))
{
	return (((((color & 0x00ff00u) * fac) & 0x00ff0000u) +
			(((color & 0xff00ffu) * fac) & 0xff00ff00u) ) >> 8u);
}

TVP_GL_FUNC_INLINE_DECL(tjs_uint32, TVPAlphaAndColorToAdditiveAlpha, (tjs_uint32 alpha, tjs_uint32 color))
{
	return TVP_GL_FUNCNAME(TVPMulColor)(color, alpha) + (color & 0xff000000u);
}

TVP_GL_FUNC_INLINE_DECL(tjs_uint32, TVPAlphaToAdditiveAlpha, (tjs_uint32 a))
{
	return TVP_GL_FUNCNAME(TVPAlphaAndColorToAdditiveAlpha)(a >> 24u, a);
}

TVP_GL_FUNC_DECL(void, TVPAddBlend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	register tjs_uint32 s;
	register tjs_uint32 tmp;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = ( ((*src&0x00ff00u)  * opa >> 8u)&0x00ff00u) +
				(( (*src&0xff00ffu) * opa >> 8u)&0xff00ffu);
			tmp = (  ( s & *dest ) + ( ((s^*dest)>>1u) & 0x7f7f7f7fu)  ) & 0x80808080u;
			src++;
			tmp = (tmp<<1u) - (tmp>>7u);
			*dest= (s + *dest - tmp) | tmp;
			dest++;
		}

	}
}

TVP_GL_FUNC_INLINE_DECL(tjs_uint32, TVPSaturatedAdd, (tjs_uint32 a, tjs_uint32 b))
{
	/* Add each byte of packed 8bit values in two 32bit uint32, with saturation. */
	tjs_uint32 tmp = (  ( a & b ) + ( ((a ^ b)>>1u) & 0x7f7f7f7fu)  ) & 0x80808080u;
	tmp = (tmp<<1u) - (tmp>>7u);
	return (a + b - tmp) | tmp;
}

TVP_GL_FUNC_INLINE_DECL(tjs_uint32, TVPAddAlphaBlend_a_a, (tjs_uint32 dest, tjs_uint32 src))
{
	/*
		Di = sat(Si, (1-Sa)*Di)
		Da = Sa + Da - SaDa
	*/

	tjs_uint32 dopa = dest >> 24u;
	tjs_uint32 sopa = src >> 24u;
	dopa = dopa + sopa - (dopa*sopa >> 8u);
	dopa -= (dopa >> 8u); /* adjust alpha */
	sopa ^= 0xffu;
	src &= 0xffffffu;
	return (dopa << 24u) + 
		TVP_GL_FUNCNAME(TVPSaturatedAdd)((((dest & 0xff00ffu)*sopa >> 8u) & 0xff00ffu) +
			(((dest & 0xff00u)*sopa >> 8u) & 0xff00u), src);
}

TVP_GL_FUNC_INLINE_DECL(tjs_uint32, TVPAddAlphaBlend_a_a_o, (tjs_uint32 dest, tjs_uint32 src, tjs_int opa))
{
	src = (((src & 0xff00ffu)*opa >> 8u) & 0xff00ffu) + (((src >> 8u) & 0xff00ffu)*opa & 0xff00ff00u);
	return TVP_GL_FUNCNAME(TVPAddAlphaBlend_a_a)(dest, src);
}

TVP_GL_FUNC_INLINE_DECL(tjs_uint32, TVPAddAlphaBlend_a_d, (tjs_uint32 dest, tjs_uint32 src))
{
	return TVP_GL_FUNCNAME(TVPAddAlphaBlend_a_a)(dest, TVP_GL_FUNCNAME(TVPAlphaToAdditiveAlpha)(src));
}

TVP_GL_FUNC_INLINE_DECL(tjs_uint32, TVPAddAlphaBlend_a_d_o, (tjs_uint32 dest, tjs_uint32 src, tjs_int opa))
{
	src = (src & 0xffffffu) + ((((src >> 24u) * opa) >> 8u) << 24u);
	return TVP_GL_FUNCNAME(TVPAddAlphaBlend_a_d)(dest, src);
}

TVP_GL_FUNC_INLINE_DECL(tjs_uint32, TVPAddAlphaBlend_n_a, (tjs_uint32 dest, tjs_uint32 src))
{
	tjs_uint32 sopa = (~src) >> 24u;
	return TVP_GL_FUNCNAME(TVPSaturatedAdd)((((dest & 0xff00ffu)*sopa >> 8u) & 0xff00ffu) + 
		(((dest & 0xff00u)*sopa >> 8u) & 0xff00u), src);
}

TVP_GL_FUNC_INLINE_DECL(tjs_uint32, TVPAddAlphaBlend_HDA_n_a, (tjs_uint32 dest, tjs_uint32 src))
{
	return (dest & 0xff000000u) + (TVP_GL_FUNCNAME(TVPAddAlphaBlend_n_a)(dest, src) & 0xffffffu);
}

TVP_GL_FUNC_INLINE_DECL(tjs_uint32, TVPAddAlphaBlend_n_a_o, (tjs_uint32 dest, tjs_uint32 src, tjs_int opa))
{
	src = (((src & 0xff00ffu)*opa >> 8u) & 0xff00ffu) + (((src >> 8u) & 0xff00ffu)*opa & 0xff00ff00u);
	return TVP_GL_FUNCNAME(TVPAddAlphaBlend_n_a)(dest, src);
}

TVP_GL_FUNC_INLINE_DECL(tjs_uint32, TVPAddAlphaBlend_HDA_n_a_o, (tjs_uint32 dest, tjs_uint32 src, tjs_int opa))
{
	return (dest & 0xff000000u) + (TVP_GL_FUNCNAME(TVPAddAlphaBlend_n_a_o)(dest, src, opa) & 0xffffffu);
}


TVP_GL_FUNC_DECL(void, TVPAdditiveAlphaBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	{
		for(int ___index = 0; ___index < len; ___index++)
		{
			dest[___index] = TVP_GL_FUNCNAME(TVPAddAlphaBlend_n_a)(dest[___index], src[___index]);
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPAdditiveAlphaBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	{
		for(int ___index = 0; ___index < len; ___index++)
		{
			dest[___index] = TVP_GL_FUNCNAME(TVPAddAlphaBlend_HDA_n_a)(dest[___index], src[___index]);
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPAdditiveAlphaBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	{
		for(int ___index = 0; ___index < len; ___index++)
		{
			dest[___index] = TVP_GL_FUNCNAME(TVPAddAlphaBlend_HDA_n_a_o)(dest[___index], src[___index], opa);
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPAdditiveAlphaBlend_a_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	{
		for(int ___index = 0; ___index < len; ___index++)
		{
			dest[___index] = TVP_GL_FUNCNAME(TVPAddAlphaBlend_a_a)(dest[___index], src[___index]);
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPAdditiveAlphaBlend_ao_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	{
		for(int ___index = 0; ___index < len; ___index++)
		{
			dest[___index] = TVP_GL_FUNCNAME(TVPAddAlphaBlend_a_a_o)(dest[___index], src[___index], opa);
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPAdditiveAlphaBlend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	{
		for(int ___index = 0; ___index < len; ___index++)
		{
			dest[___index] = TVP_GL_FUNCNAME(TVPAddAlphaBlend_n_a_o)(dest[___index], src[___index], opa);
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPAlphaBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32 d1, s, d, sopa;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = *src;
			src++;
			d = *dest;
			sopa = s >> 24u;
			d1 = d & 0xff00ffu;
			d1 = (d1 + (((s & 0xff00ffu) - d1) * sopa >> 8u)) & 0xff00ffu;
			d &= 0xff00u;
			s &= 0xff00u;
			*dest = d1 + ((d + ((s - d) * sopa >> 8u)) & 0xff00u);
			dest++;
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPAlphaBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32 d1, s, d, sopa;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = *src;
			src++;
			d = *dest;
			sopa = s >> 24u;
			d1 = d & 0xff00ffu;
			d1 = ((d1 + (((s & 0xff00ffu) - d1) * sopa >> 8u)) & 0xff00ffu) + (d & 0xff000000u); /* hda */
			d &= 0xff00u;
			s &= 0xff00u;
			*dest = d1 + ((d + ((s - d) * sopa >> 8u)) & 0xff00u);
			dest++;
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPAlphaBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32 d1, s, d, sopa;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = *src;
			src++;
			d = *dest;
			sopa = ((s >> 24u) * opa) >> 8u;
			d1 = d & 0xff00ffu;
			d1 = ((d1 + (((s & 0xff00ffu) - d1) * sopa >> 8u)) & 0xff00ffu) + (d & 0xff000000u);
			d &= 0xff00u;
			s &= 0xff00u;
			*dest = d1 + ((d + ((s - d) * sopa >> 8u)) & 0xff00u);
			dest++;
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPAlphaBlend_a_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	{
		for(int ___index = 0; ___index < len; ___index++)
		{
			dest[___index] = TVP_GL_FUNCNAME(TVPAddAlphaBlend_a_d)(dest[___index], src[___index]);
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPAlphaBlend_ao_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	{
		for(int ___index = 0; ___index < len; ___index++)
		{
			dest[___index] = TVP_GL_FUNCNAME(TVPAddAlphaBlend_a_d_o)(dest[___index], src[___index], opa);
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPAlphaBlend_d_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32 d1, s, d, sopa, addr, destalpha;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = *src;
			src++;
			d = *dest;
			addr = ((s >> 16u) & 0xff00u) + (d>>24u);
			destalpha = TVPNegativeMulTable[addr]<<24u;
			sopa = TVPOpacityOnOpacityTable[addr];
			d1 = d & 0xff00ffu;
			d1 = (d1 + (((s & 0xff00ffu) - d1) * sopa >> 8u)) & 0xff00ffu;
			d &= 0xff00u;
			s &= 0xff00u;
			*dest = d1 + ((d + ((s - d) * sopa >> 8u)) & 0xff00u) + destalpha;
			dest++;
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPAlphaBlend_do_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32 d1, s, d, sopa, addr, destalpha;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = *src;
			src++;
			d = *dest;
			addr = (( (s>>24u)*opa) & 0xff00u) + (d>>24u);
			destalpha = TVPNegativeMulTable[addr]<<24u;
			sopa = TVPOpacityOnOpacityTable[addr];
			d1 = d & 0xff00ffu;
			d1 = (d1 + (((s & 0xff00ffu) - d1) * sopa >> 8u)) & 0xff00ffu;
			d &= 0xff00u;
			s &= 0xff00u;
			*dest = d1 + ((d + ((s - d) * sopa >> 8u)) & 0xff00u) + destalpha;
			dest++;
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPAlphaBlend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32 d1, s, d, sopa;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = *src;
			src++;
			d = *dest;
			sopa = ((s >> 24u) * opa) >> 8u;
			d1 = d & 0xff00ffu;
			d1 = (d1 + (((s & 0xff00ffu) - d1) * sopa >> 8u)) & 0xff00ffu;
			d &= 0xff00u;
			s &= 0xff00u;
			*dest = d1 + ((d + ((s - d) * sopa >> 8u)) & 0xff00u);
			dest++;
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPColorDodgeBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	register tjs_uint32 tmp, tmp2, tmp3;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			tmp2 = ~*src;
			tmp = (*dest & 0xffu) * TVPRecipTable256[tmp2 & 0xffu] >> 8u;
			tmp3 = (tmp | ((tjs_int32)~(tmp - 0x100u) >> 31u)) & 0xffu;
			tmp = ((*dest & 0xff00u)>>8u) * TVPRecipTable256[(tmp2 & 0xff00u)>>8u];
			tmp3 |= (tmp | ((tjs_int32)~(tmp - 0x10000u) >> 31u)) & 0xff00u;
			tmp = ((*dest & 0xff0000u)>>16u) * TVPRecipTable256[(tmp2 & 0xff0000u)>>16u];
			tmp3 |= ((tmp | ((tjs_int32)~(tmp - 0x10000u) >> 31u)) & 0xff00u ) << 8u;
			*dest= tmp3;
			dest++;
			src++;
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPColorDodgeBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	register tjs_uint32 tmp, tmp2, tmp3;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			tmp2 = ~*src;
			tmp = (*dest & 0xffu) * TVPRecipTable256[tmp2 & 0xffu] >> 8u;
			tmp3 = (tmp | ((tjs_int32)~(tmp - 0x100u) >> 31u)) & 0xffu;
			tmp = ((*dest & 0xff00u)>>8u) * TVPRecipTable256[(tmp2 & 0xff00u)>>8u];
			tmp3 |= (tmp | ((tjs_int32)~(tmp - 0x10000u) >> 31u)) & 0xff00u;
			tmp = ((*dest & 0xff0000u)>>16u) * TVPRecipTable256[(tmp2 & 0xff0000u)>>16u];
			tmp3 |= ((tmp | ((tjs_int32)~(tmp - 0x10000u) >> 31u)) & 0xff00u ) << 8u;
			*dest= tmp3 + (*dest & 0xff000000u);
			dest++;
			src++;
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPColorDodgeBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	register tjs_uint32 tmp, tmp2, tmp3;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			tmp2 = ~ (( ((*src&0x00ff00u)  * opa >> 8u)&0x00ff00u) +
				(( (*src&0xff00ffu) * opa >> 8u)&0xff00ffu) );
			tmp = (*dest & 0xffu) * TVPRecipTable256[tmp2 & 0xffu] >> 8u;
			tmp3 = (tmp | ((tjs_int32)~(tmp - 0x100u) >> 31u)) & 0xffu;
			tmp = ((*dest & 0xff00u)>>8u) * TVPRecipTable256[(tmp2 & 0xff00u)>>8u];
			tmp3 |= (tmp | ((tjs_int32)~(tmp - 0x10000u) >> 31u)) & 0xff00u;
			tmp = ((*dest & 0xff0000u)>>16u) * TVPRecipTable256[(tmp2 & 0xff0000u)>>16u];
			tmp3 |= ((tmp | ((tjs_int32)~(tmp - 0x10000u) >> 31u)) & 0xff00u ) << 8u;
			*dest= tmp3 + (*dest & 0xff000000u);
			dest++;
			src++;
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPColorDodgeBlend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	register tjs_uint32 tmp, tmp2, tmp3;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			tmp2 = ~ (( ((*src&0x00ff00u)  * opa >> 8u)&0x00ff00u) +
				(( (*src&0xff00ffu) * opa >> 8u)&0xff00ffu) );
			tmp = (*dest & 0xffu) * TVPRecipTable256[tmp2 & 0xffu] >> 8u;
			tmp3 = (tmp | ((tjs_int32)~(tmp - 0x100u) >> 31u)) & 0xffu;
			tmp = ((*dest & 0xff00u)>>8u) * TVPRecipTable256[(tmp2 & 0xff00u)>>8u];
			tmp3 |= (tmp | ((tjs_int32)~(tmp - 0x10000u) >> 31u)) & 0xff00u;
			tmp = ((*dest & 0xff0000u)>>16u) * TVPRecipTable256[(tmp2 & 0xff0000u)>>16u];
			tmp3 |= ((tmp | ((tjs_int32)~(tmp - 0x10000u) >> 31u)) & 0xff00u ) << 8u;
			*dest= tmp3;
			dest++;
			src++;
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPConstAlphaBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32 d1, s, d;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = *src;
			src++;
			d = *dest;
			d1 = d & 0xff00ffu;
			d1 = (d1 + (((s & 0xff00ffu) - d1) * opa >> 8u)) & 0xff00ffu;
			d &= 0xff00u;
			s &= 0xff00u;
			*dest = d1 | ((d + ((s - d) * opa >> 8u)) & 0xff00u);
			dest++;
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPConstAlphaBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32 d1, s, d;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = *src;
			src++;
			d = *dest;
			d1 = d & 0xff00ffu;
			d1 = ((d1 + (((s & 0xff00ffu) - d1) * opa >> 8u)) & 0xff00ffu) + (d & 0xff000000u);
			d &= 0xff00u;
			s &= 0xff00u;
			*dest = d1 | ((d + ((s - d) * opa >> 8u)) & 0xff00u);
			dest++;
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPConstAlphaBlend_a_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	opa <<= 24u;
	{
		for(int ___index = 0; ___index < len; ___index++)
		{
			dest[___index] = TVP_GL_FUNCNAME(TVPAddAlphaBlend_a_a)(dest[___index], (src[___index] & 0xffffffu) | opa);
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPConstAlphaBlend_d_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32 d1, s, d, addr;
	tjs_int alpha;
	opa <<= 8u;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = *src;
			src++;
			d = *dest;
			addr = opa + (d>>24u);
			alpha = TVPOpacityOnOpacityTable[addr];
			d1 = d & 0xff00ffu;
			d1 = ((d1 + (((s & 0xff00ffu) - d1) * alpha >> 8u)) & 0xff00ffu) +
				(TVPNegativeMulTable[addr]<<24u);
			d &= 0xff00u;
			s &= 0xff00u;
			*dest = d1 | ((d + ((s - d) * alpha >> 8u)) & 0xff00u);
			dest++;
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPCopyColor_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	if(dest < src)
	{
		/* backward */
	  	for(int ___index = len; ___index >= 0; ___index--)
		{
			{
				dest[___index] = (dest[___index] & 0xff000000u) +
					(src[___index] & 0x00ffffffu);
			}
		}
	}
	else
	{
		/* forward */
		{
			for(int ___index = 0; ___index < len; ___index++)
			{
				{
					dest[___index] = (dest[___index] & 0xff000000u) +
						(src[___index] & 0x00ffffffu);
				}
			}
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPCopyOpaqueImage_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32 t1;
	{
		for(int ___index = 0; ___index < len; ___index++)
		{
			t1 = src[___index];;
			t1 |= 0xff000000u;;
			dest[___index] = t1;;
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPDarkenBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	register tjs_uint32 tmp, m_src;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			m_src = ~*src;
			tmp = ((m_src & *dest) + (((m_src ^ *dest) >> 1u) & 0x7f7f7f7fu) ) & 0x80808080u;
			tmp = (tmp << 1u) - (tmp >> 7u);
			*dest ^= (*dest ^ *src) & tmp;
			dest++;
			src++;
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPDarkenBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	register tjs_uint32 tmp, m_src;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			m_src = ~*src;
			tmp = ((m_src & *dest) + (((m_src ^ *dest) >> 1u) & 0x7f7f7f7fu) ) & 0x80808080u;
			tmp = (tmp << 1u) - (tmp >> 7u);
			*dest ^= ((*dest ^ *src) & tmp) & 0xffffffu;
			dest++;
			src++;
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPDarkenBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	register tjs_uint32 tmp, m_src, d1;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			m_src = ~*src;
			tmp = ((m_src & *dest) + (((m_src ^ *dest) >> 1u) & 0x7f7f7f7fu) ) & 0x80808080u;
			tmp = (tmp << 1u) - (tmp >> 7u);
			tmp = *dest ^ (((*dest ^ *src) & tmp) & 0xffffffu);
			d1 = *dest & 0xff00ffu;
			d1 = ((d1 + (((tmp & 0xff00ffu) - d1) * opa >> 8u)) & 0xff00ffu) + (*dest & 0xff000000u); /* hda */
			m_src = *dest & 0xff00u;
			tmp &= 0xff00u;
			*dest = d1 + ((m_src + ((tmp - m_src) * opa >> 8u)) & 0xff00u);
			dest++;
			src++;
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPDarkenBlend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	register tjs_uint32 tmp, m_src, d1;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			m_src = ~*src;
			tmp = ((m_src & *dest) + (((m_src ^ *dest) >> 1u) & 0x7f7f7f7fu) ) & 0x80808080u;
			tmp = (tmp << 1u) - (tmp >> 7u);
			tmp = *dest ^ ((*dest ^ *src) & tmp);
			d1 = *dest & 0xff00ffu;
			d1 = (d1 + (((tmp & 0xff00ffu) - d1) * opa >> 8u)) & 0xff00ffu;
			m_src = *dest & 0xff00u;
			tmp &= 0xff00u;
			*dest = d1 + ((m_src + ((tmp - m_src) * opa >> 8u)) & 0xff00u);
			dest++;
			src++;
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPLightenBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	register tjs_uint32 tmp, m_dest;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			m_dest = ~*dest;
			tmp = ((*src & m_dest) + (((*src ^ m_dest) >> 1u) & 0x7f7f7f7fu) ) & 0x80808080u;
			tmp = (tmp << 1u) - (tmp >> 7u);
			*dest ^= (*dest ^ *src) & tmp;
			dest++;
			src++;
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPLightenBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	register tjs_uint32 tmp, m_dest;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			m_dest = ~*dest;
			tmp = ((*src & m_dest) + (((*src ^ m_dest) >> 1u) & 0x7f7f7f7fu) ) & 0x80808080u;
			tmp = (tmp << 1u) - (tmp >> 7u);
			*dest ^= ((*dest ^ *src) & tmp) & 0xffffffu;
			dest++;
			src++;
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPLightenBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	register tjs_uint32 tmp, m_dest, d1;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			m_dest = ~*dest;
			tmp = ((*src & m_dest) + (((*src ^ m_dest) >> 1u) & 0x7f7f7f7fu) ) & 0x80808080u;
			tmp = (tmp << 1u) - (tmp >> 7u);
			tmp = *dest ^ (((*dest ^ *src) & tmp) & 0xffffffu);
			d1 = *dest & 0xff00ffu;
			d1 = ((d1 + (((tmp & 0xff00ffu) - d1) * opa >> 8u)) & 0xff00ffu) + (*dest & 0xff000000u); /* hda */
			m_dest = *dest & 0xff00u;
			tmp &= 0xff00u;
			*dest = d1 + ((m_dest + ((tmp - m_dest) * opa >> 8u)) & 0xff00u);
			dest++;
			src++;
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPLightenBlend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	register tjs_uint32 tmp, m_dest, d1;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			m_dest = ~*dest;
			tmp = ((*src & m_dest) + (((*src ^ m_dest) >> 1u) & 0x7f7f7f7fu) ) & 0x80808080u;
			tmp = (tmp << 1u) - (tmp >> 7u);
			tmp = *dest ^ ((*dest ^ *src) & tmp);
			d1 = *dest & 0xff00ffu;
			d1 = (d1 + (((tmp & 0xff00ffu) - d1) * opa >> 8u)) & 0xff00ffu;
			m_dest = *dest & 0xff00u;
			tmp &= 0xff00u;
			*dest = d1 + ((m_dest + ((tmp - m_dest) * opa >> 8u)) & 0xff00u);
			dest++;
			src++;
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPMulBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	register tjs_uint32 tmp;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			tmp  = (*dest & 0xffu) * (*src & 0xffu) & 0xff00u;
			tmp |= ((*dest & 0xff00u) >> 8u) * (*src & 0xff00u) & 0xff0000u;
			tmp |= ((*dest & 0xff0000u) >> 16u) * (*src & 0xff0000u) & 0xff000000u;
			tmp >>= 8u;
			*dest = tmp;
			dest++;
			src++;
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPMulBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	register tjs_uint32 tmp;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			tmp  = (*dest & 0xffu) * (*src & 0xffu) & 0xff00u;
			tmp |= ((*dest & 0xff00u) >> 8u) * (*src & 0xff00u) & 0xff0000u;
			tmp |= ((*dest & 0xff0000u) >> 16u) * (*src & 0xff0000u) & 0xff000000u;
			tmp >>= 8u;
			*dest = tmp + (*dest & 0xff000000u);
			dest++;
			src++;
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPMulBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	register tjs_uint32 s;
	register tjs_uint32 tmp;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = ~*src;
			s = ~( ( ((s&0x00ff00u)  * opa >> 8u)&0x00ff00u) +
				(( (s&0xff00ffu) * opa >> 8u)&0xff00ffu));
			tmp  = (*dest & 0xffu) * (s & 0xffu) & 0xff00u;
			tmp |= ((*dest & 0xff00u) >> 8u) * (s & 0xff00u) & 0xff0000u;
			tmp |= ((*dest & 0xff0000u) >> 16u) * (s & 0xff0000u) & 0xff000000u;
			tmp >>= 8u;
			*dest = tmp + (*dest & 0xff000000u);
			dest++;
			src++;
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPMulBlend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	register tjs_uint32 s;
	register tjs_uint32 tmp;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = ~*src;
			s = ~( ( ((s&0x00ff00u)  * opa >> 8u)&0x00ff00u) +
				(( (s&0xff00ffu) * opa >> 8u)&0xff00ffu));
			tmp  = (*dest & 0xffu) * (s & 0xffu) & 0xff00u;
			tmp |= ((*dest & 0xff00u) >> 8u) * (s & 0xff00u) & 0xff0000u;
			tmp |= ((*dest & 0xff0000u) >> 16u) * (s & 0xff0000u) & 0xff000000u;
			tmp >>= 8u;
			*dest = tmp;
			dest++;
			src++;
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPScreenBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	register tjs_uint32 tmp, s, d;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = ~*src;
			d = ~*dest;
			tmp  = (d & 0xffu) * (s & 0xffu) & 0xff00u;
			tmp |= ((d & 0xff00u) >> 8u) * (s & 0xff00u) & 0xff0000u;
			tmp |= ((d & 0xff0000u) >> 16u) * (s & 0xff0000u) & 0xff000000u;
			tmp >>= 8u;
			*dest = ~tmp;
			dest++;
			src++;
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPScreenBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	register tjs_uint32 tmp, s, d;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = ~*src;
			d = ~*dest;
			tmp  = (d & 0xffu) * (s & 0xffu) & 0xff00u;
			tmp |= ((d & 0xff00u) >> 8u) * (s & 0xff00u) & 0xff0000u;
			tmp |= ((d & 0xff0000u) >> 16u) * (s & 0xff0000u) & 0xff000000u;
			tmp >>= 8u;
			*dest = ~tmp ^ (d & 0xff000000u);
			dest++;
			src++;
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPScreenBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	register tjs_uint32 s, d;
	register tjs_uint32 tmp;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			d = ~*dest;
			s = *src;
			s = ~( ( ((s&0x00ff00u)  * opa >> 8u)&0x00ff00u) +
				(( (s&0xff00ffu) * opa >> 8u)&0xff00ffu));
			tmp  = (d & 0xffu) * (s & 0xffu) & 0xff00u;
			tmp |= ((d & 0xff00u) >> 8u) * (s & 0xff00u) & 0xff0000u;
			tmp |= ((d & 0xff0000u) >> 16u) * (s & 0xff0000u) & 0xff000000u;
			tmp >>= 8u;
			*dest = ~tmp ^ (d & 0xff000000u);
			dest++;
			src++;
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPScreenBlend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	register tjs_uint32 s, d;
	register tjs_uint32 tmp;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			d = ~*dest;
			s = *src;
			s = ~( ( ((s&0x00ff00u)  * opa >> 8u)&0x00ff00u) +
				(( (s&0xff00ffu) * opa >> 8u)&0xff00ffu));
			tmp  = (d & 0xffu) * (s & 0xffu) & 0xff00u;
			tmp |= ((d & 0xff00u) >> 8u) * (s & 0xff00u) & 0xff0000u;
			tmp |= ((d & 0xff0000u) >> 16u) * (s & 0xff0000u) & 0xff000000u;
			tmp >>= 8u;
			*dest = tmp;
			dest++;
			src++;
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPSubBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	register tjs_uint32 tmp;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			tmp = (  ( *src & *dest ) + ( ((*src ^ *dest)>>1u) & 0x7f7f7f7fu)  ) & 0x80808080u;
			tmp = (tmp << 1u) - (tmp >> 7u);
			*dest = (*src + *dest - tmp) & tmp;
			dest++;
			src++;
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPSubBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	register tjs_uint32 tmp, s;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = *src | 0xff000000u;
			tmp = (  ( s & *dest ) + ( ((s ^ *dest)>>1u) & 0x7f7f7f7fu)  ) & 0x80808080u;
			tmp = (tmp << 1u) - (tmp >> 7u);
			*dest = (s + *dest - tmp) & tmp;
			dest++;
			src++;
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPSubBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	register tjs_uint32 s/*, d*/;
	register tjs_uint32 tmp;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = ~*src;
			s = 0xff000000u | ~ (( ((s&0x00ff00u)  * opa >> 8u)&0x00ff00u) +
				(( (s&0xff00ffu) * opa >> 8u)&0xff00ffu) );
			tmp = (  ( s & *dest ) + ( ((s ^ *dest)>>1u) & 0x7f7f7f7fu)  ) & 0x80808080u;
			tmp = (tmp << 1u) - (tmp >> 7u);
			*dest = (s + *dest - tmp) & tmp;
			dest++;
			src++;
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPSubBlend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	register tjs_uint32 s;
	register tjs_uint32 tmp;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = ~*src;
			s = ~ (( ((s&0x00ff00u)  * opa >> 8u)&0x00ff00u) +
				(( (s&0xff00ffu) * opa >> 8u)&0xff00ffu) );
			tmp = (  ( s & *dest ) + ( ((s ^ *dest)>>1u) & 0x7f7f7f7fu)  ) & 0x80808080u;
			tmp = (tmp << 1u) - (tmp >> 7u);
			*dest = (s + *dest - tmp) & tmp;
			dest++;
			src++;
		}
	}
}

#define TVP_GL_IA32_FUNC_DECL TVP_GL_FUNC_DECL

#define FOREACH_CHANNEL(body, len, src, dest) \
	{ \
		for (tjs_int i = 0; i < len; i += 1) \
		{ \
			__attribute__ ((unused)) const tjs_uint8 *s = (const tjs_uint8 *)&src[i]; \
			__attribute__ ((unused)) const tjs_uint8 *sr = s + 0; \
			__attribute__ ((unused)) const tjs_uint8 *sg = s + 1; \
			__attribute__ ((unused)) const tjs_uint8 *sb = s + 2; \
			__attribute__ ((unused)) const tjs_uint8 *sa = s + 3; \
			__attribute__ ((unused)) tjs_uint8 *d = (tjs_uint8 *)&dest[i]; \
			__attribute__ ((unused)) tjs_uint8 *dr = d + 0; \
			__attribute__ ((unused)) tjs_uint8 *dg = d + 1; \
			__attribute__ ((unused)) tjs_uint8 *db = d + 2; \
			__attribute__ ((unused)) tjs_uint8 *da = d + 3; \
			for (tjs_int j = 0; j < 4; j += 1) \
			{ \
				body \
			} \
		} \
	}

#define TVP_GL_IA32_BLEND_FUNC_BODY(funcname, channelbody, hda) \
	TVP_GL_IA32_FUNC_DECL(void, funcname, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len)) \
	{ \
		FOREACH_CHANNEL( \
			{ \
				if (hda && (j % 4 == 3)) \
				{ \
					continue; \
				} \
				tjs_uint16 sevenbit = src[i] >> 0x19u; \
				channelbody; \
			}, len, src, dest); \
	}

#define TVP_GL_IA32_BLEND_FUNC_BODY_OPACITY(funcname, channelbody, hda) \
	TVP_GL_IA32_FUNC_DECL(void, funcname, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa)) \
	{ \
		FOREACH_CHANNEL( \
			{ \
				if (hda && (j % 4 == 3)) \
				{ \
					continue; \
				} \
				tjs_uint16 sevenbit = src[i] >> 0x19u; \
				sevenbit *= opa; \
				sevenbit >>= 8u; \
				channelbody; \
			}, len, src, dest); \
	}

#define TVP_GL_IA32_BLEND_FUNC(funcname, channelbody) \
	TVP_GL_IA32_BLEND_FUNC_BODY        (TVPPs##funcname##Blend_c,       channelbody, 0) \
	TVP_GL_IA32_BLEND_FUNC_BODY_OPACITY(TVPPs##funcname##Blend_o_c,     channelbody, 0) \
	TVP_GL_IA32_BLEND_FUNC_BODY        (TVPPs##funcname##Blend_HDA_c,   channelbody, 1) \
	TVP_GL_IA32_BLEND_FUNC_BODY_OPACITY(TVPPs##funcname##Blend_HDA_o_c, channelbody, 1)

#define TVP_GL_IA32_BLEND_FUNC_OPACITY_ONLY(funcname, channelbody) \
	TVP_GL_IA32_BLEND_FUNC_BODY_OPACITY(TVPPs##funcname##Blend_o_c,     channelbody, 0) \
	TVP_GL_IA32_BLEND_FUNC_BODY_OPACITY(TVPPs##funcname##Blend_HDA_o_c, channelbody, 1)

#define TVP_GL_IA32_ALPHABLEND(channel, destchannel, sevenbit) \
	{ \
		channel -= destchannel; \
		channel *= sevenbit; \
		channel >>= 7; \
		channel += destchannel; \
		destchannel = channel; \
	}

TVP_GL_IA32_BLEND_FUNC(Alpha, {
	tjs_uint16 k = s[j];
	TVP_GL_IA32_ALPHABLEND(k, d[j], sevenbit);
});

TVP_GL_IA32_BLEND_FUNC(Add, {
	tjs_uint16 k = s[j];
	k += d[j];
	if (k > 0xff)
	{
		k = 0xff;
	}
	TVP_GL_IA32_ALPHABLEND(k, d[j], sevenbit);
});

TVP_GL_IA32_BLEND_FUNC(Sub, {
	tjs_uint16 k = s[j];
	k ^= 0xff;
	k = d[j] - k;
	if (k > 0xff)
	{
		k = 0;
	}
	TVP_GL_IA32_ALPHABLEND(k, d[j], sevenbit);
});

TVP_GL_IA32_BLEND_FUNC(Mul, {
	tjs_uint16 k = s[j];
	k *= d[j];
	k >>= 8u;
	TVP_GL_IA32_ALPHABLEND(k, d[j], sevenbit);
});

TVP_GL_IA32_BLEND_FUNC(Screen, {
	tjs_uint16 k = s[j];
	k *= d[j];
	k >>= 8u;
	tjs_uint16 l = s[j];
	l -= k;
	l *= sevenbit;
	l >>= 7;
	l += d[j];
	d[j] = l;
});

// FIXME: some channel shows up as wrong on regular/HDA versions (not opacity version) on test harness w/ IA32 (asm) only
TVP_GL_IA32_BLEND_FUNC(Overlay, {
	tjs_uint16 k = s[j];
	tjs_uint16 l = s[j];
	l *= d[j];
	l >>= 7;
	if (0x80 > d[j])
	{
		k = l;
	}
	else
	{
		k += d[j];
		k <<= 1;
		k -= l;
		k -= 0xff;
	}
	TVP_GL_IA32_ALPHABLEND(k, d[j], sevenbit);
});

// FIXME: some channel shows up as wrong on regular/HDA versions (not opacity version) on test harness w/ IA32 (asm) only
TVP_GL_IA32_BLEND_FUNC(HardLight, {
	tjs_uint16 k = s[j];
	tjs_uint16 l = s[j];
	l *= d[j];
	l >>= 7u;
	if (0x80u > s[j])
	{
		k = l;
	}
	else
	{
		k += d[j];
		k <<= 1u;
		k -= l;
		k -= 0xff;
	}
	TVP_GL_IA32_ALPHABLEND(k, d[j], sevenbit);
});

TVP_GL_IA32_BLEND_FUNC(SoftLight, {
	tjs_uint16 k = TVPPsTableSoftLight[s[j]][d[j]];
	if (j % 4 == 3)
	{
		k = s[j];
	}
	k -= d[j];
	k *= sevenbit;
	k >>= 7;
	k += d[j];
	d[j] = k;
});

TVP_GL_IA32_BLEND_FUNC(ColorDodge, {
	tjs_uint16 k = TVPPsTableColorDodge[s[j]][d[j]];
	if (j % 4 == 3)
	{
		k = s[j];
	}
	TVP_GL_IA32_ALPHABLEND(k, d[j], sevenbit);
});

TVP_GL_IA32_BLEND_FUNC(ColorDodge5, {
	tjs_uint16 k = s[j];
	k *= sevenbit;
	k >>= 7;
	if (j % 4 != 3)
	{
		k = TVPPsTableColorDodge[k][d[j]];
	}
	d[j] = k;
});

TVP_GL_IA32_BLEND_FUNC(ColorBurn, {
	tjs_uint16 k = TVPPsTableColorBurn[s[j]][d[j]];
	if (j % 4 == 3)
	{
		k = s[j];
	}
	TVP_GL_IA32_ALPHABLEND(k, d[j], sevenbit);
});

TVP_GL_IA32_BLEND_FUNC(Lighten, {
	tjs_uint16 k = d[j];
	k -= s[j];
	if (k > 0xff)
	{
		k = 0;
	}
	k += s[j];
	TVP_GL_IA32_ALPHABLEND(k, d[j], sevenbit);
});

TVP_GL_IA32_BLEND_FUNC(Darken, {
	tjs_uint16 k = s[j];
	k -= d[j];
	if (k > 0xff)
	{
		k = 0;
	}
	k = s[j] - k;
	TVP_GL_IA32_ALPHABLEND(k, d[j], sevenbit);
});

TVP_GL_IA32_BLEND_FUNC(Diff, {
	tjs_uint16 k = s[j];
	k -= d[j];
	if (k > 0xff)
	{
		k = 0;
	}
	tjs_uint16 l = d[j];
	l -= s[j];
	if (l > 0xff)
	{
		l = 0;
	}
	k += l;
	TVP_GL_IA32_ALPHABLEND(k, d[j], sevenbit);
});

TVP_GL_IA32_BLEND_FUNC(Diff5, {
	tjs_uint16 k = s[j];
	k *= sevenbit;
	k >>= 7;
	tjs_uint16 l = k;
	l -= d[j];
	if (l > 0xff)
	{
		l = 0;
	}
	tjs_uint16 m = d[j];
	m -= k;
	if (m > 0xff)
	{
		m = 0;
	}
	l += m;
	d[j] = l;
});

TVP_GL_IA32_BLEND_FUNC(Exclusion, {
	tjs_uint16 k = s[j];
	k *= d[j];
	k >>= 7;
	tjs_uint16 l = s[j];
	l -= k;
	l *= sevenbit;
	l >>= 7;
	l += d[j];
	d[j] = l;
});
