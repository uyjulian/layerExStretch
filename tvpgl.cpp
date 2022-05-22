// Copied tvpgl functions, because we can't rely on the core to have them.

#include "tvpgl.h"
#include <simde/x86/mmx.h>

#define LAST_IND(x, part_type) (sizeof(x) / sizeof(part_type) - 1)
#define HIGH_IND(x, part_type) LAST_IND(x, part_type)
#define LOW_IND(x, part_type)  0

#define BYTEn(x, n)  (*((tjs_uint8 *)&(x) + n))
#define WORDn(x, n)  (*((tjs_uint16 *)&(x) + n))
#define DWORDn(x, n) (*((tjs_uint32 *)&(x) + n))

#ifdef LOBYTE
#undef LOBYTE
#endif
#define LOBYTE(x) BYTEn(x, LOW_IND(x, tjs_uint8))
#ifdef HIBYTE
#undef HIBYTE
#endif
#define HIBYTE(x) BYTEn(x, HIGH_IND(x, tjs_uint8))
#define BYTE1(x)  BYTEn(x, 1)
#define BYTE2(x)  BYTEn(x, 2)

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
	tjs_uint32 *  v3; // edi
	tjs_uint32   *v4; // ebp
	tjs_uint32 *  v5; // esi

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		v5 = &dest[len];
		if (dest < v5)
		{
			do
			{
				*v3 = simde_mm_cvtsi64_si32(simde_m_paddusb(simde_mm_cvtsi32_si64(*v3), simde_mm_cvtsi32_si64(*v4)));
				++v3;
				++v4;
			} while (v3 < v5);
		}
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPAddBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	simde__m64    v3; // mm7
	simde__m64    v4; // mm7
	tjs_uint32 *  v5; // edi
	tjs_uint32   *v6; // ebp
	tjs_uint32 *  v7; // esi

	if (len > 0)
	{
		v3 = simde_mm_cvtsi32_si64(0xFFFFFFu);
		v4 = simde_m_punpckldq(v3, v3);
		v5 = dest;
		v6 = (tjs_uint32   *)src;
		v7 = &dest[len];
		if (dest < v7)
		{
			do
			{
				*v5 = simde_mm_cvtsi64_si32(simde_m_paddusb(simde_mm_cvtsi32_si64(*v5), simde_m_pand(simde_mm_cvtsi32_si64(*v6), v4)));
				++v5;
				++v6;
			} while (v5 < v7);
		}
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPAddBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	simde__m64    v5; // mm7
	simde__m64    v6; // mm7
	tjs_uint32 *  v7; // edi
	tjs_uint32   *v8; // ebp
	tjs_uint32 *  v9; // esi

	if (len > 0)
	{
		v5 = simde_mm_set1_pi16(opa);
		v6 = simde_m_psrlqi(v5, 0x10u);
		v7 = dest;
		v8 = (tjs_uint32   *)src;
		v9 = &dest[len];
		if (dest < v9)
		{
			do
			{
				*v7 = simde_mm_cvtsi64_si32(
					simde_m_packuswb(
						simde_m_paddw(
							simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v7), simde_mm_setzero_si64()),
							simde_m_psrlwi(simde_m_pmullw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v8), simde_mm_setzero_si64()), v6), 8u)),
						simde_mm_setzero_si64()));
				++v7;
				++v8;
			} while (v7 < v9);
		}
	}
	simde_m_empty();
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
	TVP_GL_FUNCNAME(TVPAddBlend_HDA_o_c)(dest, src, len, opa);
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

TVP_GL_FUNC_INLINE_DECL(tjs_uint32, TVPAddAlphaBlend_n_a_o, (tjs_uint32 dest, tjs_uint32 src, tjs_int opa))
{
	src = (((src & 0xff00ffu)*opa >> 8u) & 0xff00ffu) + (((src >> 8u) & 0xff00ffu)*opa & 0xff00ff00u);
	return TVP_GL_FUNCNAME(TVPAddAlphaBlend_n_a)(dest, src);
}

TVP_GL_FUNC_INLINE_DECL(tjs_uint32, TVPAddAlphaBlend_HDA_n_a_o, (tjs_uint32 dest, tjs_uint32 src, tjs_int opa))
{
	return (dest & 0xff000000u) + (TVP_GL_FUNCNAME(TVPAddAlphaBlend_n_a_o)(dest, src, opa) & 0xffffffu);
}

static tjs_uint64 mask0000ffffffffffff = 0x0000ffffffffffffull;
static tjs_uint64 mask00ffffff00ffffff = 0x00ffffff00ffffffull;

TVP_GL_FUNC_DECL(void, TVPAdditiveAlphaBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32   *    v3; // edi
	const tjs_uint32 *v4; // ebp
	tjs_uint32 *      v5; // esi
	simde__m64        v7; // mm2
	simde__m64        v8; // mm1

	if (len > 0)
	{
		v3 = dest;
		v4 = src;
		v5 = &dest[len];
		if (dest < v5)
		{
			do
			{
				v7  = simde_mm_set1_pi16(*v4 >> 24);
				v8  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v3), simde_mm_setzero_si64());
				*v3 = simde_mm_cvtsi64_si32(simde_m_paddusb(simde_m_packuswb(simde_m_psubw(v8, simde_m_psrlwi(simde_m_pmullw(v8, v7), 8u)), simde_mm_setzero_si64()), simde_mm_cvtsi32_si64(*v4)));
				++v4;
				++v3;
			} while (v3 < v5);
		}
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPAdditiveAlphaBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32   *    v3; // edi
	const tjs_uint32 *v4; // ebp
	tjs_uint32 *      v5; // esi
	simde__m64        v7; // mm2
	simde__m64        v8; // mm1

	if (len > 0)
	{
		v3 = dest;
		v4 = src;
		v5 = &dest[len];
		if (dest < v5)
		{
			do
			{
				v7  = simde_mm_set1_pi16(*v4 >> 24);
				v8  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v3), simde_mm_setzero_si64());
				*v3 = simde_mm_cvtsi64_si32(
					simde_m_paddusb(
						simde_m_packuswb(
							simde_m_psubw(v8, simde_m_pand(simde_m_psrlwi(simde_m_pmullw(v8, v7), 8u), (simde__m64)mask0000ffffffffffff)),
							simde_mm_setzero_si64()),
						simde_m_pand(simde_mm_cvtsi32_si64(*v4), (simde__m64)mask00ffffff00ffffff)));
				++v4;
				++v3;
			} while (v3 < v5);
		}
	}
	simde_m_empty();
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
	tjs_uint32   *v3; // edi
	tjs_uint32   *v4; // ebp
	tjs_uint32 *  v5; // esi
	simde__m64    v6; // mm3
	simde__m64    v7; // mm4
	simde__m64    v8; // mm4
	simde__m64    v9; // mm1

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		v5 = &dest[len];
		if (dest < v5)
		{
			do
			{
				v6  = simde_mm_cvtsi32_si64(*v4);
				v7  = simde_m_psrlqi(v6, 0x18u);
				v8  = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v7));
				v9  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v3), simde_mm_setzero_si64());
				*v3 = simde_mm_cvtsi64_si32(simde_m_packuswb(simde_m_paddw(simde_m_psubw(v9, simde_m_psrlwi(simde_m_pmullw(v9, v8), 8u)), simde_m_punpcklbw(v6, simde_mm_setzero_si64())), simde_mm_setzero_si64()));
				++v4;
				++v3;
			} while (v3 < v5);
		}
	}
	simde_m_empty();
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
	tjs_uint32   *v4;  // edi
	tjs_uint32   *v5;  // ebp
	simde__m64    v7;  // mm7
	simde__m64    v8;  // mm7
	tjs_uint32 *  v9;  // esi
	simde__m64    v10; // mm2
	simde__m64    v11; // mm4
	simde__m64    v12; // mm1
	simde__m64    v13; // mm1

	if (len > 0)
	{
		v4 = dest;
		v5 = (tjs_uint32   *)src;
		v7 = simde_mm_set1_pi16(((unsigned int)opa >> 7) + opa);
		v8 = v7;
		v9 = &dest[len];
		if (dest < v9)
		{
			do
			{
				v10 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v4), simde_mm_setzero_si64());
				v11 = simde_m_psrlwi(simde_m_pmullw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v5), simde_mm_setzero_si64()), v8), 8u);
				v12 = simde_m_psrlqi(v11, 0x30u);
				v13 = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v12));
				*v4 = simde_mm_cvtsi64_si32(simde_m_packuswb(simde_m_paddw(simde_m_psubw(v10, simde_m_psrlwi(simde_m_pmullw(v10, v13), 8u)), v11), simde_mm_setzero_si64()));
				++v5;
				++v4;
			} while (v4 < v9);
		}
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPAlphaBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32   *    v3; // edi
	const tjs_uint32 *v4; // ebp
	tjs_uint32 *      v5; // esi
	tjs_uint32        v6; // eax
	simde__m64        v8; // mm4
	simde__m64        v9; // mm1

	if (len > 0)
	{
		v3 = dest;
		v4 = src;
		v5 = &dest[len];
		if (dest < v5)
		{
			do
			{
				v6 = *v4;
				if (*v4 < 0xFF000000)
				{
					v8  = simde_mm_set1_pi16(v6 >> 24);
					v9  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v3), simde_mm_setzero_si64());
					*v3 = simde_mm_cvtsi64_si32(
						simde_m_packuswb(
							simde_m_paddb(v9, simde_m_psrlwi(simde_m_pmullw(simde_m_psubw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(v6), simde_mm_setzero_si64()), v9), v8), 8u)),
							simde_mm_setzero_si64()));
				}
				else
				{
					*v3 = v6;
				}
				++v3;
				++v4;
			} while (v3 < v5);
		}
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPAlphaBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	int          v3; // ecx
	tjs_uint32   v4; // eax
	simde__m64   v6; // mm2
	simde__m64   v7; // mm4
	tjs_uint32   v8; // eax
	simde__m64   v9; // mm1

	if (len - 1 >= 0)
	{
		v3 = len - 1;
		do
		{
			v4       = src[v3];
			v6       = simde_mm_cvtsi32_si64(v4);
			v7       = simde_mm_set1_pi16(v4 >> 24);
			v8       = dest[v3];
			v9       = simde_m_punpcklbw(simde_mm_cvtsi32_si64(v8), simde_mm_setzero_si64());
			dest[v3] = (v8 & 0xFF000000) | (simde_mm_cvtsi64_si32(
											 simde_m_packuswb(
												 simde_m_psrlwi(
													 simde_m_paddw(
														 simde_m_psllwi(v9, 8u),
														 simde_m_pmullw(simde_m_psubw(simde_m_punpcklbw(v6, simde_mm_setzero_si64()), v9), v7)),
													 8u),
												 simde_mm_setzero_si64())) &
											 0xFFFFFF);
			--v3;
		} while (v3 >= 0);
	}
	simde_m_empty();
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
	tjs_uint32 *      v3;  // edi
	const tjs_uint32 *v4;  // ebp
	simde__m64        v5;  // mm7
	tjs_uint32 *      v6;  // esi
	int               v7;  // eax
	simde__m64        v8;  // mm1
	simde__m64        v10; // mm4

	if (len > 0)
	{
		v3 = dest;
		v4 = src;
		v5 = simde_mm_cvtsi32_si64(0xFFFFFFu);
		v6 = &dest[len];
		if (dest < v6)
		{
			do
			{
				if (*v4 <= 0xFFFFFF)
				{
					++v3;
					++v4;
					continue;
				}
				v7  = (*v3 >> 24) + ((*v4 >> 16) & 0xFF00);
				v8  = simde_m_punpcklbw(simde_m_pand(simde_mm_cvtsi32_si64(*v3), v5), simde_mm_setzero_si64());
				v10 = simde_mm_set1_pi16(TVPOpacityOnOpacityTable[v7]);
				*v3 = (TVPNegativeMulTable[v7] << 24) | simde_mm_cvtsi64_si32(
															simde_m_packuswb(
																simde_m_psrlwi(
																	simde_m_paddw(
																		simde_m_psllwi(v8, 8u),
																		simde_m_pmullw(
																			simde_m_psubw(
																				simde_m_punpcklbw(simde_m_pand(simde_mm_cvtsi32_si64(*v4), v5), simde_mm_setzero_si64()),
																				v8),
																			v10)),
																	8u),
																simde_mm_setzero_si64()));
				++v3;
				++v4;
			} while (v3 < v6);
		}
	}
	simde_m_empty();
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
	tjs_uint32   *v4; // edi
	tjs_uint32   *v5; // ebp
	tjs_uint32 *  v6; // esi
	simde__m64    v8; // mm4
	simde__m64    v9; // mm1

	if (len > 0)
	{
		v4 = dest;
		v5 = (tjs_uint32   *)src;
		v6 = &dest[len];
		if (dest < v6)
		{
			do
			{
				v8  = simde_mm_set1_pi16((unsigned int)opa * (tjs_uint64)*v5 >> 32);
				v9  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v4), simde_mm_setzero_si64());
				*v4 = simde_mm_cvtsi64_si32(
					simde_m_packuswb(
						simde_m_psrlwi(
							simde_m_paddw(simde_m_psllwi(v9, 8u), simde_m_pmullw(simde_m_psubw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v5), simde_mm_setzero_si64()), v9), v8)),
							8u),
						simde_mm_setzero_si64()));
				++v4;
				++v5;
			} while (v4 < v6);
		}
	}
	simde_m_empty();
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
	tjs_int    v5; // ebx
	simde__m64 v6; // mm4
	simde__m64 i;  // mm4
	simde__m64 v8; // mm1
	simde__m64 v9; // mm2

	if (len > 0)
	{
		v5 = 0;
		v6 = simde_mm_set1_pi16(opa);
		for (i = v6;
			 v5 < len;
			 dest[v5 - 1] = simde_mm_cvtsi64_si32(simde_m_packuswb(simde_m_psrlwi(simde_m_paddw(simde_m_psllwi(v8, 8u), simde_m_pmullw(v9, i)), 8u), simde_mm_setzero_si64())))
		{
			v8 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(dest[v5]), simde_mm_setzero_si64());
			v9 = simde_m_psubw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(src[v5]), simde_mm_setzero_si64()), v8);
			++v5;
		}
	}
	simde_m_empty();
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
	tjs_uint32   *v3; // edi
	tjs_uint32   *v4; // ebp
	tjs_uint32 *  v5; // esi
	simde__m64    v6; // mm1

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		v5 = &dest[len];
		if (dest < v5)
		{
			do
			{
				v6  = simde_mm_cvtsi32_si64(*v3);
				*v3 = simde_mm_cvtsi64_si32(simde_m_psubb(v6, simde_m_psubusb(v6, simde_mm_cvtsi32_si64(*v4))));
				++v3;
				++v4;
			} while (v3 < v5);
		}
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPDarkenBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	simde__m64    v3; // mm7
	simde__m64    v4; // mm7
	tjs_uint32   *v5; // edi
	tjs_uint32   *v6; // ebp
	tjs_uint32 *  v7; // esi
	simde__m64    v8; // mm1
	simde__m64    v9;
	simde__m64    v10;
	simde__m64    v11;
	simde__m64    v12;

	if (len > 0)
	{
		v3 = simde_mm_cvtsi32_si64(0xFFFFFFu);
		v4 = simde_m_punpckldq(v3, v3);
		v5 = dest;
		v6 = (tjs_uint32   *)src;
		v7 = &dest[len];
		if (dest < v7)
		{
			if (!((tjs_uintptr_t)dest & 4))
			{
				v8  = simde_m_por(simde_mm_cvtsi32_si64(*v5), v4);
				*v5 = simde_mm_cvtsi64_si32(simde_m_psubb(v8, simde_m_pand(simde_m_psubusb(v8, simde_mm_cvtsi32_si64(*v6)), v4)));
				++v5;
				++v6;
			}
			do
			{
				v9  = *(simde__m64 *)v5;
				v10 = *((simde__m64 *)v5 + 1);
				v6 += 4;
				v11 = *(simde__m64 *)v5;
				v12 = *((simde__m64 *)v5 + 1);
				v5 += 4;
				*((simde__m64 *)v5 - 2) = simde_m_psubb(v9, simde_m_pand(simde_m_psubusb(v11, *((simde__m64 *)v6 - 2)), v4));
				*((simde__m64 *)v5 - 1) = simde_m_psubb(v10, simde_m_pand(simde_m_psubusb(v12, *((simde__m64 *)v6 - 1)), v4));
			} while (v5 < &dest[len - 4]);
			do
			{
				v8  = simde_m_por(simde_mm_cvtsi32_si64(*v5), v4);
				*v5 = simde_mm_cvtsi64_si32(simde_m_psubb(v8, simde_m_pand(simde_m_psubusb(v8, simde_mm_cvtsi32_si64(*v6)), v4)));
				++v5;
				++v6;
			} while (v5 < v7);
		}
	}
	simde_m_empty();
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
	tjs_uint32 *       v3; // edi
	tjs_uint32   *v4; // ebp
	tjs_uint32 *  v5; // esi

	if (len > 0)
	{
		v3 = (tjs_uint32 *)dest;
		v4 = (tjs_uint32   *)src;
		v5 = &dest[len];
		if (dest < v5)
		{
			do
			{
				*v3 = simde_mm_cvtsi64_si32(simde_m_paddb(simde_m_psubusb(simde_mm_cvtsi32_si64(*v4), simde_mm_cvtsi32_si64(*v3)), simde_mm_cvtsi32_si64(*v3)));
				v3  = (tjs_uint32 *)((char *)v3 + 4);
				++v4;
			} while (v3 < v5);
		}
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPLightenBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	simde__m64  v3; // mm7
	simde__m64  v4; // mm7
	tjs_uint32 *v5; // edi
	simde__m64 *v6; // ebp
	tjs_uint32 *v7; // esi

	if (len > 0)
	{
		v3 = simde_mm_cvtsi32_si64(0xFFFFFFu);
		v4 = simde_m_punpckldq(v3, v3);
		v5 = (tjs_uint32 *)dest;
		v6 = (simde__m64 *)src;
		v7 = &dest[len];
		if (dest < v7)
		{
			do
			{
				*v5 = simde_mm_cvtsi64_si32(simde_m_paddb(simde_m_psubusb(simde_m_pand(*v6, v4), simde_mm_cvtsi32_si64(*v5)), simde_mm_cvtsi32_si64(*v5)));
				v5  = (tjs_uint32 *)((char *)v5 + 4);
				v6  = (simde__m64 *)((char *)v6 + 4);
			} while (v5 < v7);
		}
	}
	simde_m_empty();
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

static tjs_uint64 TVPMulBlendHDA_mulmask       = 0x0000ffffffffffffull;
static tjs_uint64 TVPMulBlendHDA_100bit        = 0x0100000000000000ull;
static tjs_uint64 TVPMulBlendHDA_fullbit       = 0xffffffffffffffffull;
static tjs_uint64 TVPMulBlend_full_bit_aligned = 0xffffffffffffffffull;

TVP_GL_FUNC_DECL(void, TVPMulBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32 *  v3; // edi
	tjs_uint32   *v4; // ebp
	tjs_uint32 *  v5; // esi

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		v5 = &dest[len];
		if (dest < v5)
		{
			do
			{
				*v3 = simde_mm_cvtsi64_si32(
					simde_m_packuswb(
						simde_m_psrlwi(
							simde_m_pmullw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v3), simde_mm_setzero_si64()), simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v4), simde_mm_setzero_si64())),
							8u),
						simde_mm_setzero_si64()));
				++v3;
				++v4;
			} while (v3 < v5);
		}
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPMulBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	simde__m64    v3; // mm6
	simde__m64    v4; // mm7
	tjs_uint32 *  v5; // edi
	tjs_uint32   *v6; // ebp
	tjs_uint32 *  v7; // esi

	if (len > 0)
	{
		v3 = (simde__m64)TVPMulBlendHDA_mulmask;
		v4 = (simde__m64)TVPMulBlendHDA_100bit;
		v5 = dest;
		v6 = (tjs_uint32   *)src;
		v7 = &dest[len];
		if (dest < v7)
		{
			do
			{
				*v5 = simde_mm_cvtsi64_si32(
					simde_m_packuswb(
						simde_m_psrlwi(
							simde_m_pmullw(
								simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v5), simde_mm_setzero_si64()),
								simde_m_por(simde_m_pand(simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v6), simde_mm_setzero_si64()), v3), v4)),
							8u),
						simde_mm_setzero_si64()));
				++v5;
				++v6;
			} while (v5 < v7);
		}
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPMulBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	simde__m64    v5;  // mm5
	simde__m64    v6;  // mm5
	simde__m64    v7;  // mm6
	simde__m64    v8;  // mm7
	tjs_uint32 *  v9;  // edi
	tjs_uint32   *v10; // ebp
	tjs_uint32 *  v11; // esi

	if (len > 0)
	{
		v5  = simde_mm_set1_pi16(opa);
		v6  = v5;
		v7  = (simde__m64)TVPMulBlendHDA_mulmask;
		v8  = (simde__m64)TVPMulBlendHDA_100bit;
		v9  = dest;
		v10 = (tjs_uint32   *)src;
		v11 = &dest[len];
		if (dest < v11)
		{
			do
			{
				*v9 = simde_mm_cvtsi64_si32(
					simde_m_packuswb(
						simde_m_psrlwi(
							simde_m_pmullw(
								simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v9), simde_mm_setzero_si64()),
								simde_m_por(
									simde_m_pand(
										simde_m_psrlwi(
											simde_m_pxor(
												simde_m_pmullw(
													simde_m_punpcklbw(
														simde_m_pxor(
															simde_mm_cvtsi32_si64(*v10),
															(simde__m64)TVPMulBlend_full_bit_aligned),
														simde_mm_setzero_si64()),
													v6),
												(simde__m64)TVPMulBlend_full_bit_aligned),
											8u),
										v7),
									v8)),
							8u),
						simde_mm_setzero_si64()));
				++v9;
				++v10;
			} while (v9 < v11);
		}
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPMulBlend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	simde__m64    v5;  // mm5
	simde__m64    v6;  // mm5
	simde__m64    v7;  // mm6
	tjs_uint32 *  v8;  // edi
	tjs_uint32   *v9;  // ebp
	tjs_uint32 *  v10; // esi

	if (len > 0)
	{
		v5  = simde_mm_set1_pi16(opa);
		v6  = v5;
		v7  = (simde__m64)TVPMulBlendHDA_fullbit;
		v8  = dest;
		v9  = (tjs_uint32   *)src;
		v10 = &dest[len];
		if (dest < v10)
		{
			do
			{
				*v8 = simde_mm_cvtsi64_si32(
					simde_m_packuswb(
						simde_m_psrlwi(
							simde_m_pmullw(
								simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v8), simde_mm_setzero_si64()),
								simde_m_psrlwi(simde_m_pxor(simde_m_pmullw(simde_m_punpcklbw(simde_m_pxor(simde_mm_cvtsi32_si64(*v9), v7), simde_mm_setzero_si64()), v6), v7), 8u)),
							8u),
						simde_mm_setzero_si64()));
				++v8;
				++v9;
			} while (v8 < v10);
		}
	}
	simde_m_empty();
}

static tjs_uint64 TVPScreenMulBlend_full_bit_aligned = 0xffffffffffffffffull;
static tjs_uint64 TVPScreenBlendHDA_alphamask        = 0xff000000ff000000ull;
static tjs_uint64 TVPScreenBlendHDA_mulmask          = 0x00ffffff00ffffffull;
static tjs_uint64 TVPScreenBlendHDA_mul_100bit       = 0x0100000000000000ull;

TVP_GL_FUNC_DECL(void, TVPScreenBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	simde__m64    v3; // mm6
	tjs_uint32 *  v4; // edi
	tjs_uint32   *v5; // ebp
	tjs_uint32 *  v6; // esi

	if (len > 0)
	{
		v3 = (simde__m64)TVPScreenMulBlend_full_bit_aligned;
		v4 = dest;
		v5 = (tjs_uint32   *)src;
		v6 = &dest[len];
		if (dest < v6)
		{
			do
			{
				*v4 = simde_mm_cvtsi64_si32(
					simde_m_pxor(
						simde_m_packuswb(
							simde_m_psrlwi(
								simde_m_pmullw(
									simde_m_punpcklbw(simde_m_pxor(simde_mm_cvtsi32_si64(*v4), v3), simde_mm_setzero_si64()),
									simde_m_punpcklbw(simde_m_pxor(simde_mm_cvtsi32_si64(*v5), v3), simde_mm_setzero_si64())),
								8u),
							simde_mm_setzero_si64()),
						v3));
				++v4;
				++v5;
			} while (v4 < v6);
		}
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPScreenBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	simde__m64    v3; // mm6
	simde__m64    v4; // mm7
	tjs_uint32 *  v5; // edi
	tjs_uint32   *v6; // ebp
	tjs_uint32 *  v7; // esi

	if (len > 0)
	{
		v3 = (simde__m64)TVPScreenBlendHDA_mul_100bit;
		v4 = (simde__m64)TVPScreenBlendHDA_mulmask;
		v5 = dest;
		v6 = (tjs_uint32   *)src;
		v7 = &dest[len];
		if (dest < v7)
		{
			do
			{
				*v5 = simde_mm_cvtsi64_si32(
					simde_m_pxor(
						simde_m_packuswb(
							simde_m_psrlwi(
								simde_m_pmullw(
									simde_m_punpcklbw(simde_m_pxor(simde_mm_cvtsi32_si64(*v5), v4), simde_mm_setzero_si64()),
									simde_m_por(simde_m_punpcklbw(simde_m_pand(simde_m_pxor(simde_mm_cvtsi32_si64(*v6), v4), v4), simde_mm_setzero_si64()), v3)),
								8u),
							simde_mm_setzero_si64()),
						v4));
				++v5;
				++v6;
			} while (v5 < v7);
		}
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPScreenBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	simde__m64    v5;  // mm5
	simde__m64    v6;  // mm5
	simde__m64    v7;  // mm6
	simde__m64    v8;  // mm7
	tjs_uint32 *  v9;  // edi
	tjs_uint32   *v10; // ebp
	tjs_uint32 *  v11; // esi

	if (len > 0)
	{
		v5  = simde_mm_set1_pi16(opa);
		v6  = v5;
		v7  = (simde__m64)TVPScreenBlendHDA_alphamask;
		v8  = (simde__m64)TVPScreenBlendHDA_mulmask;
		v9  = dest;
		v10 = (tjs_uint32   *)src;
		v11 = &dest[len];
		if (dest < v11)
		{
			if (!((tjs_uintptr_t)dest & 4))
			{
				*v9 = simde_mm_cvtsi64_si32(
					simde_m_por(
						simde_m_pand(
							simde_m_pxor(
								simde_m_packuswb(
									simde_m_psrlwi(
										simde_m_pmullw(
											simde_m_punpcklbw(simde_m_pxor(simde_mm_cvtsi32_si64(*v9), v8), simde_mm_setzero_si64()),
											simde_m_por(
												simde_m_pand(
													simde_m_psrlwi(
														simde_m_pxor(
															simde_m_pmullw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v10), simde_mm_setzero_si64()), v6),
															(simde__m64)TVPScreenMulBlend_full_bit_aligned),
														8u),
													v7),
												v8)),
										8u),
									simde_mm_setzero_si64()),
								v8),
							v8),
						simde_m_pand(simde_mm_cvtsi32_si64(*v9), v7)));
				++v9;
				++v10;
			}
			// XXX: This will cause test failures when run immediately after the NASM version
			do
			{
				*v9 = simde_mm_cvtsi64_si32(simde_m_por(
					simde_m_pand(
						simde_m_pxor(
							simde_m_packuswb(
								simde_m_psrlwi(
									simde_m_pmullw(
										simde_m_punpcklbw(simde_m_pxor(simde_mm_cvtsi32_si64(*v9), v8), simde_mm_setzero_si64()),
										simde_m_psrlwi(
											simde_m_pxor(
												simde_m_pmullw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v10), simde_mm_setzero_si64()), v6),
												(simde__m64)TVPScreenMulBlend_full_bit_aligned),
											8u)),
									8u),
								simde_m_psrlwi(
									simde_m_pmullw(
										simde_m_punpckhbw(simde_m_pxor(simde_mm_cvtsi32_si64(*v9), v8), simde_mm_setzero_si64()),
										simde_m_psrlwi(
											simde_m_pxor(
												simde_m_pmullw(simde_m_punpckhbw(simde_mm_cvtsi32_si64(*v10), simde_mm_setzero_si64()), v6),
												(simde__m64)TVPScreenMulBlend_full_bit_aligned),
											0u)),
									0u)),
							v8),
						v8),
					simde_m_pand(simde_mm_cvtsi32_si64(*v9), v7)));
				++v9;
				++v10;
				*v9 = simde_mm_cvtsi64_si32(simde_m_por(
					simde_m_pand(
						simde_m_pxor(
							simde_m_packuswb(
								simde_m_psrlwi(
									simde_m_pmullw(
										simde_m_punpcklbw(simde_m_pxor(simde_mm_cvtsi32_si64(*v9), v8), simde_mm_setzero_si64()),
										simde_m_psrlwi(
											simde_m_pxor(
												simde_m_pmullw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v10), simde_mm_setzero_si64()), v6),
												(simde__m64)TVPScreenMulBlend_full_bit_aligned),
											8u)),
									8u),
								simde_m_psrlwi(
									simde_m_pmullw(
										simde_m_punpckhbw(simde_m_pxor(simde_mm_cvtsi32_si64(*v9), v8), simde_mm_setzero_si64()),
										simde_m_psrlwi(
											simde_m_pxor(
												simde_m_pmullw(simde_m_punpckhbw(simde_mm_cvtsi32_si64(*v10), simde_mm_setzero_si64()), v6),
												(simde__m64)TVPScreenMulBlend_full_bit_aligned),
											0u)),
									0u)),
							v8),
						v8),
					simde_m_pand(simde_mm_cvtsi32_si64(*v9), v7)));
				++v9;
				++v10;
			} while (v9 < &dest[len - 4]);
			do
			{
				*v9 = simde_mm_cvtsi64_si32(
					simde_m_por(
						simde_m_pand(
							simde_m_pxor(
								simde_m_packuswb(
									simde_m_psrlwi(
										simde_m_pmullw(
											simde_m_punpcklbw(simde_m_pxor(simde_mm_cvtsi32_si64(*v9), v8), simde_mm_setzero_si64()),
											simde_m_por(
												simde_m_pand(
													simde_m_psrlwi(
														simde_m_pxor(
															simde_m_pmullw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v10), simde_mm_setzero_si64()), v6),
															(simde__m64)TVPScreenMulBlend_full_bit_aligned),
														8u),
													v7),
												v8)),
										8u),
									simde_mm_setzero_si64()),
								v8),
							v8),
						simde_m_pand(simde_mm_cvtsi32_si64(*v9), v7)));
				++v9;
				++v10;
			} while (v9 < v11);
		}
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPScreenBlend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	simde__m64    v5;  // mm5
	simde__m64    v6;  // mm5
	simde__m64    v7;  // mm6
	tjs_uint32 *  v8;  // edi
	tjs_uint32   *v9;  // ebp
	tjs_uint32 *  v10; // esi

	if (len > 0)
	{
		v5  = simde_mm_set1_pi16(opa);
		v6  = v5;
		v7  = (simde__m64)TVPScreenMulBlend_full_bit_aligned;
		v8  = dest;
		v9  = (tjs_uint32   *)src;
		v10 = &dest[len];
		if (dest < v10)
		{
			do
			{
				*v8 = simde_mm_cvtsi64_si32(
					simde_m_pxor(
						simde_m_packuswb(
							simde_m_psrlwi(
								simde_m_pmullw(
									simde_m_punpcklbw(simde_m_pxor(simde_mm_cvtsi32_si64(*v8), v7), simde_mm_setzero_si64()),
									simde_m_psrlwi(simde_m_pxor(simde_m_pmullw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v9), simde_mm_setzero_si64()), v6), v7), 8u)),
								8u),
							simde_m_psrlwi(
								simde_m_pmullw(
									simde_m_punpckhbw(simde_m_pxor(simde_mm_cvtsi32_si64(*v8), v7), simde_mm_setzero_si64()),
									simde_m_psrlwi(simde_m_pxor(simde_m_pmullw(simde_m_punpckhbw(simde_mm_cvtsi32_si64(*v9), simde_mm_setzero_si64()), v6), v7), 8u)),
								8u)),
						v7));
				++v8;
				++v9;
			} while (v8 < v10);
		}
	}
	simde_m_empty();
}

static tjs_uint64 TVPSubBlend_full_bit_one = 0xffffffffffffffffull;

TVP_GL_FUNC_DECL(void, TVPSubBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	simde__m64    v3; // mm7
	simde__m64    v4; // mm7
	tjs_uint32 *  v5; // edi
	tjs_uint32   *v6; // ebp
	tjs_uint32 *  v7; // esi

	if (len > 0)
	{
		v3 = simde_mm_cvtsi32_si64(0xFFFFFFFF);
		v4 = simde_m_punpckldq(v3, v3);
		v5 = dest;
		v6 = (tjs_uint32   *)src;
		v7 = &dest[len];
		if (dest < v7)
		{
			do
			{
				*v5 = simde_mm_cvtsi64_si32(simde_m_psubusb(simde_mm_cvtsi32_si64(*v5), simde_m_pxor(simde_mm_cvtsi32_si64(*v6), v4)));
				++v5;
				++v6;
			} while (v5 < v7);
		}
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPSubBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	simde__m64    v3; // mm7
	simde__m64    v4; // mm7
	simde__m64    v5; // mm6
	simde__m64    v6; // mm6
	tjs_uint32 *  v7; // edi
	tjs_uint32   *v8; // ebp
	tjs_uint32 *  v9; // esi

	if (len > 0)
	{
		v3 = simde_mm_cvtsi32_si64(0xFFFFFFFF);
		v4 = simde_m_punpckldq(v3, v3);
		v5 = simde_mm_cvtsi32_si64(0xFFFFFFu);
		v6 = simde_m_punpckldq(v5, v5);
		v7 = dest;
		v8 = (tjs_uint32   *)src;
		v9 = &dest[len];
		if (dest < v9)
		{
			do
			{
				*v7 = simde_mm_cvtsi64_si32(simde_m_psubusb(simde_mm_cvtsi32_si64(*v7), simde_m_pand(simde_m_pxor(simde_mm_cvtsi32_si64(*v8), v4), v6)));
				++v7;
				++v8;
			} while (v7 < v9);
		}
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPSubBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	simde__m64    v5; // mm7
	simde__m64    v6; // mm7
	tjs_uint32 *  v7; // edi
	tjs_uint32   *v8; // ebp
	tjs_uint32 *  v9; // esi

	if (len > 0)
	{
		v5 = simde_mm_set1_pi16(opa);
		v6 = simde_m_psrlqi(v5, 0x10u);
		v7 = dest;
		v8 = (tjs_uint32   *)src;
		v9 = &dest[len];
		if (dest < v9)
		{
			do
			{
				*v7 = simde_mm_cvtsi64_si32(
					simde_m_packuswb(
						simde_m_psubw(
							simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v7), simde_mm_setzero_si64()),
							simde_m_psrlwi(
								simde_m_pmullw(
									simde_m_punpcklbw(
										simde_m_pxor(
											simde_mm_cvtsi32_si64(*v8),
											(simde__m64)TVPSubBlend_full_bit_one),
										simde_mm_setzero_si64()),
									v6),
								8u)),
						simde_mm_setzero_si64()));
				++v7;
				++v8;
			} while (v7 < v9);
		}
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPSubBlend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	TVP_GL_FUNCNAME(TVPSubBlend_HDA_o_c)(dest, src, len, opa);
}


TVP_GL_FUNC_DECL(void, TVPPsAlphaBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32   *v3; // edi
	tjs_uint32   *v4; // esi
	simde__m64    v5; // mm2
	simde__m64    v6; // mm0
	simde__m64    v7; // mm2
	simde__m64    v8; // mm1
	simde__m64    v9; // mm2

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		do
		{
			v5 = simde_mm_cvtsi32_si64(*v4);
			v6 = v5;
			v7 = simde_m_psrldi(v5, 0x19u);
			v8 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v3), simde_mm_setzero_si64());
			v9 = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v7));
			++v4;
			++v3;
			*(v3 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(v8, simde_m_psrawi(simde_m_pmullw(simde_m_psubw(simde_m_punpcklbw(v6, simde_mm_setzero_si64()), v8), simde_m_punpckldq(v9, v9)), 7u)),
					simde_mm_setzero_si64()));
		} while (v4 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsAlphaBlend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32   *v4;  // edi
	tjs_uint32   *v5;  // esi
	simde__m64    v6;  // mm4
	simde__m64    v7;  // mm2
	simde__m64    v8;  // mm0
	simde__m64    v9;  // mm1
	simde__m64    v10; // mm2
	simde__m64    v11; // mm2

	if (len > 0)
	{
		v4 = dest;
		v5 = (tjs_uint32   *)src;
		v6 = simde_mm_cvtsi32_si64(opa);
		do
		{
			v7  = simde_mm_cvtsi32_si64(*v5);
			v8  = v7;
			v9  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v4), simde_mm_setzero_si64());
			v10 = simde_m_psrldi(simde_m_pmullw(simde_m_psrldi(v7, 0x19u), v6), 8u);
			v11 = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v10));
			++v5;
			++v4;
			*(v4 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v9,
						simde_m_psrawi(simde_m_pmullw(simde_m_psubw(simde_m_punpcklbw(v8, simde_mm_setzero_si64()), v9), simde_m_punpckldq(v11, v11)), 7u)),
					simde_mm_setzero_si64()));
		} while (v5 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsAlphaBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32   *v3; // edi
	tjs_uint32   *v4; // esi
	simde__m64    v5; // mm2
	simde__m64    v6; // mm0
	simde__m64    v7; // mm2
	simde__m64    v8; // mm1

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		do
		{
			v5 = simde_mm_cvtsi32_si64(*v4);
			v6 = v5;
			v7 = simde_m_psrldi(v5, 0x19u);
			v8 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v3), simde_mm_setzero_si64());
			++v4;
			++v3;
			*(v3 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v8,
						simde_m_psrawi(
							simde_m_pmullw(simde_m_psubw(simde_m_punpcklbw(v6, simde_mm_setzero_si64()), v8), simde_m_punpckldq(simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v7)), v7)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v4 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsAlphaBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32   *v4;  // edi
	tjs_uint32   *v5;  // esi
	simde__m64    v6;  // mm4
	simde__m64    v7;  // mm2
	simde__m64    v8;  // mm0
	simde__m64    v9;  // mm1
	simde__m64    v10; // mm2

	if (len > 0)
	{
		v4 = dest;
		v5 = (tjs_uint32   *)src;
		v6 = simde_mm_cvtsi32_si64(opa);
		do
		{
			v7  = simde_mm_cvtsi32_si64(*v5);
			v8  = v7;
			v9  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v4), simde_mm_setzero_si64());
			v10 = simde_m_psrldi(simde_m_pmullw(simde_m_psrldi(v7, 0x19u), v6), 8u);
			++v5;
			++v4;
			*(v4 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v9,
						simde_m_psrawi(
							simde_m_pmullw(simde_m_psubw(simde_m_punpcklbw(v8, simde_mm_setzero_si64()), v9), simde_m_punpckldq(simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v10)), v10)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v5 < &src[len]);
	}
	simde_m_empty();
}


TVP_GL_FUNC_DECL(void, TVPPsAddBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32   *v3;  // edi
	tjs_uint32   *v4;  // esi
	simde__m64    v5;  // mm2
	simde__m64    v6;  // mm1
	simde__m64    v7;  // mm0
	simde__m64    v8;  // mm2
	simde__m64    v9;  // mm0
	simde__m64    v10; // mm1
	simde__m64    v11; // mm2

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		do
		{
			v5  = simde_mm_cvtsi32_si64(*v4);
			v6  = simde_mm_cvtsi32_si64(*v3);
			v7  = v5;
			v8  = simde_m_psrldi(v5, 0x19u);
			v9  = simde_m_paddusb(v7, v6);
			v10 = simde_m_punpcklbw(v6, simde_mm_setzero_si64());
			v11 = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v8));
			++v4;
			++v3;
			*(v3 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v10,
						simde_m_psrawi(simde_m_pmullw(simde_m_psubw(simde_m_punpcklbw(v9, simde_mm_setzero_si64()), v10), simde_m_punpckldq(v11, v11)), 7u)),
					simde_mm_setzero_si64()));
		} while (v4 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsAddBlend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32   *v4;  // edi
	tjs_uint32   *v5;  // esi
	simde__m64    v6;  // mm4
	simde__m64    v7;  // mm2
	simde__m64    v8;  // mm1
	simde__m64    v9;  // mm0
	simde__m64    v10; // mm1
	simde__m64    v11; // mm2
	simde__m64    v12; // mm2

	if (len > 0)
	{
		v4 = dest;
		v5 = (tjs_uint32   *)src;
		v6 = simde_mm_cvtsi32_si64(opa);
		do
		{
			v7  = simde_mm_cvtsi32_si64(*v5);
			v8  = simde_mm_cvtsi32_si64(*v4);
			v9  = simde_m_paddusb(v7, v8);
			v10 = simde_m_punpcklbw(v8, simde_mm_setzero_si64());
			v11 = simde_m_psrldi(simde_m_pmullw(simde_m_psrldi(v7, 0x19u), v6), 8u);
			v12 = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v11));
			++v5;
			++v4;
			*(v4 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v10,
						simde_m_psrawi(simde_m_pmullw(simde_m_psubw(simde_m_punpcklbw(v9, simde_mm_setzero_si64()), v10), simde_m_punpckldq(v12, v12)), 7u)),
					simde_mm_setzero_si64()));
		} while (v5 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsAddBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32   *v3;  // edi
	tjs_uint32   *v4;  // esi
	simde__m64    v5;  // mm2
	simde__m64    v6;  // mm1
	simde__m64    v7;  // mm0
	simde__m64    v8;  // mm2
	simde__m64    v9;  // mm0
	simde__m64    v10; // mm1

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		do
		{
			v5  = simde_mm_cvtsi32_si64(*v4);
			v6  = simde_mm_cvtsi32_si64(*v3);
			v7  = v5;
			v8  = simde_m_psrldi(v5, 0x19u);
			v9  = simde_m_paddusb(v7, v6);
			v10 = simde_m_punpcklbw(v6, simde_mm_setzero_si64());
			++v4;
			++v3;
			*(v3 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v10,
						simde_m_psrawi(
							simde_m_pmullw(simde_m_psubw(simde_m_punpcklbw(v9, simde_mm_setzero_si64()), v10), simde_m_punpckldq(simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v8)), v8)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v4 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsAddBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32   *v4;  // edi
	tjs_uint32   *v5;  // esi
	simde__m64    v6;  // mm4
	simde__m64    v7;  // mm2
	simde__m64    v8;  // mm1
	simde__m64    v9;  // mm0
	simde__m64    v10; // mm1
	simde__m64    v11; // mm2

	if (len > 0)
	{
		v4 = dest;
		v5 = (tjs_uint32   *)src;
		v6 = simde_mm_cvtsi32_si64(opa);
		do
		{
			v7  = simde_mm_cvtsi32_si64(*v5);
			v8  = simde_mm_cvtsi32_si64(*v4);
			v9  = simde_m_paddusb(v7, v8);
			v10 = simde_m_punpcklbw(v8, simde_mm_setzero_si64());
			v11 = simde_m_psrldi(simde_m_pmullw(simde_m_psrldi(v7, 0x19u), v6), 8u);
			++v5;
			++v4;
			*(v4 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v10,
						simde_m_psrawi(
							simde_m_pmullw(simde_m_psubw(simde_m_punpcklbw(v9, simde_mm_setzero_si64()), v10), simde_m_punpckldq(simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v11)), v11)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v5 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsSubBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32   *v3;  // edi
	tjs_uint32   *v4;  // esi
	simde__m64    v5;  // mm2
	simde__m64    v6;  // mm1
	simde__m64    v7;  // mm7
	simde__m64    v8;  // mm2
	simde__m64    v9;  // mm0
	simde__m64    v10; // mm1
	simde__m64    v11; // mm2

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		do
		{
			v5  = simde_mm_cvtsi32_si64(*v4);
			v6  = simde_mm_cvtsi32_si64(*v3);
			v7  = v5;
			v8  = simde_m_psrldi(v5, 0x19u);
			v9  = simde_m_psubusb(v6, simde_m_pxor(v7, simde_m_pcmpeqd(simde_mm_setzero_si64(), simde_mm_setzero_si64())));
			v10 = simde_m_punpcklbw(v6, simde_mm_setzero_si64());
			v11 = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v8));
			++v4;
			++v3;
			*(v3 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v10,
						simde_m_psrawi(simde_m_pmullw(simde_m_psubw(simde_m_punpcklbw(v9, simde_mm_setzero_si64()), v10), simde_m_punpckldq(v11, v11)), 7u)),
					simde_mm_setzero_si64()));
		} while (v4 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsSubBlend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32   *v4;  // edi
	tjs_uint32   *v5;  // esi
	simde__m64    v6;  // mm4
	simde__m64    v7;  // mm2
	simde__m64    v8;  // mm1
	simde__m64    v9;  // mm0
	simde__m64    v10; // mm1
	simde__m64    v11; // mm2
	simde__m64    v12; // mm2

	if (len > 0)
	{
		v4 = dest;
		v5 = (tjs_uint32   *)src;
		v6 = simde_mm_cvtsi32_si64(opa);
		do
		{
			v7  = simde_mm_cvtsi32_si64(*v5);
			v8  = simde_mm_cvtsi32_si64(*v4);
			v9  = simde_m_psubusb(v8, simde_m_pxor(v7, simde_m_pcmpeqd(simde_mm_setzero_si64(), simde_mm_setzero_si64())));
			v10 = simde_m_punpcklbw(v8, simde_mm_setzero_si64());
			v11 = simde_m_psrldi(simde_m_pmullw(simde_m_psrldi(v7, 0x19u), v6), 8u);
			v12 = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v11));
			++v5;
			++v4;
			*(v4 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v10,
						simde_m_psrawi(simde_m_pmullw(simde_m_psubw(simde_m_punpcklbw(v9, simde_mm_setzero_si64()), v10), simde_m_punpckldq(v12, v12)), 7u)),
					simde_mm_setzero_si64()));
		} while (v5 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsSubBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32   *v3;  // edi
	tjs_uint32   *v4;  // esi
	simde__m64    v5;  // mm2
	simde__m64    v6;  // mm1
	simde__m64    v7;  // mm7
	simde__m64    v8;  // mm2
	simde__m64    v9;  // mm0
	simde__m64    v10; // mm1

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		do
		{
			v5  = simde_mm_cvtsi32_si64(*v4);
			v6  = simde_mm_cvtsi32_si64(*v3);
			v7  = v5;
			v8  = simde_m_psrldi(v5, 0x19u);
			v9  = simde_m_psubusb(v6, simde_m_pxor(v7, simde_m_pcmpeqd(simde_mm_setzero_si64(), simde_mm_setzero_si64())));
			v10 = simde_m_punpcklbw(v6, simde_mm_setzero_si64());
			++v4;
			++v3;
			*(v3 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v10,
						simde_m_psrawi(
							simde_m_pmullw(simde_m_psubw(simde_m_punpcklbw(v9, simde_mm_setzero_si64()), v10), simde_m_punpckldq(simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v8)), v8)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v4 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsSubBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32   *v4;  // edi
	tjs_uint32   *v5;  // esi
	simde__m64    v6;  // mm4
	simde__m64    v7;  // mm2
	simde__m64    v8;  // mm1
	simde__m64    v9;  // mm0
	simde__m64    v10; // mm1
	simde__m64    v11; // mm2

	if (len > 0)
	{
		v4 = dest;
		v5 = (tjs_uint32   *)src;
		v6 = simde_mm_cvtsi32_si64(opa);
		do
		{
			v7  = simde_mm_cvtsi32_si64(*v5);
			v8  = simde_mm_cvtsi32_si64(*v4);
			v9  = simde_m_psubusb(v8, simde_m_pxor(v7, simde_m_pcmpeqd(simde_mm_setzero_si64(), simde_mm_setzero_si64())));
			v10 = simde_m_punpcklbw(v8, simde_mm_setzero_si64());
			v11 = simde_m_psrldi(simde_m_pmullw(simde_m_psrldi(v7, 0x19u), v6), 8u);
			++v5;
			++v4;
			*(v4 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v10,
						simde_m_psrawi(
							simde_m_pmullw(simde_m_psubw(simde_m_punpcklbw(v9, simde_mm_setzero_si64()), v10), simde_m_punpckldq(simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v11)), v11)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v5 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsMulBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32   *v3; // edi
	tjs_uint32   *v4; // esi
	simde__m64    v5; // mm6
	simde__m64    v6; // mm0
	simde__m64    v7; // mm6
	simde__m64    v8; // mm1
	simde__m64    v9; // mm6

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		do
		{
			v5 = simde_mm_cvtsi32_si64(*v4);
			v6 = v5;
			v7 = simde_m_psrldi(v5, 0x19u);
			v8 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v3), simde_mm_setzero_si64());
			v9 = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v7));
			++v4;
			++v3;
			*(v3 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v8,
						simde_m_psrawi(
							simde_m_pmullw(
								simde_m_psubw(simde_m_psrlwi(simde_m_pmullw(simde_m_punpcklbw(v6, simde_mm_setzero_si64()), v8), 8u), v8),
								simde_m_punpckldq(v9, v9)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v4 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsMulBlend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32   *v4;  // edi
	tjs_uint32   *v5;  // esi
	simde__m64    v6;  // mm4
	simde__m64    v7;  // mm6
	simde__m64    v8;  // mm0
	simde__m64    v9;  // mm1
	simde__m64    v10; // mm6
	simde__m64    v11; // mm6

	if (len > 0)
	{
		v4 = dest;
		v5 = (tjs_uint32   *)src;
		v6 = simde_mm_cvtsi32_si64(opa);
		do
		{
			v7  = simde_mm_cvtsi32_si64(*v5);
			v8  = v7;
			v9  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v4), simde_mm_setzero_si64());
			v10 = simde_m_psrldi(simde_m_pmullw(simde_m_psrldi(v7, 0x19u), v6), 8u);
			v11 = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v10));
			++v5;
			++v4;
			*(v4 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v9,
						simde_m_psrawi(
							simde_m_pmullw(
								simde_m_psubw(simde_m_psrlwi(simde_m_pmullw(simde_m_punpcklbw(v8, simde_mm_setzero_si64()), v9), 8u), v9),
								simde_m_punpckldq(v11, v11)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v5 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsMulBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32   *v3; // edi
	tjs_uint32   *v4; // esi
	simde__m64    v5; // mm6
	simde__m64    v6; // mm0
	simde__m64    v7; // mm6
	simde__m64    v8; // mm1

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		do
		{
			v5 = simde_mm_cvtsi32_si64(*v4);
			v6 = v5;
			v7 = simde_m_psrldi(v5, 0x19u);
			v8 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v3), simde_mm_setzero_si64());
			++v4;
			++v3;
			*(v3 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v8,
						simde_m_psrawi(
							simde_m_pmullw(
								simde_m_psubw(simde_m_psrlwi(simde_m_pmullw(simde_m_punpcklbw(v6, simde_mm_setzero_si64()), v8), 8u), v8),
								simde_m_punpckldq(simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v7)), v7)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v4 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsMulBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32   *v4;  // edi
	tjs_uint32   *v5;  // esi
	simde__m64    v6;  // mm4
	simde__m64    v7;  // mm6
	simde__m64    v8;  // mm0
	simde__m64    v9;  // mm1
	simde__m64    v10; // mm6

	if (len > 0)
	{
		v4 = dest;
		v5 = (tjs_uint32   *)src;
		v6 = simde_mm_cvtsi32_si64(opa);
		do
		{
			v7  = simde_mm_cvtsi32_si64(*v5);
			v8  = v7;
			v9  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v4), simde_mm_setzero_si64());
			v10 = simde_m_psrldi(simde_m_pmullw(simde_m_psrldi(v7, 0x19u), v6), 8u);
			++v5;
			++v4;
			*(v4 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v9,
						simde_m_psrawi(
							simde_m_pmullw(
								simde_m_psubw(simde_m_psrlwi(simde_m_pmullw(simde_m_punpcklbw(v8, simde_mm_setzero_si64()), v9), 8u), v9),
								simde_m_punpckldq(simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v10)), v10)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v5 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsScreenBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32   *v3;  // edi
	tjs_uint32   *v4;  // esi
	simde__m64    v5;  // mm2
	simde__m64    v6;  // mm1
	simde__m64    v7;  // mm2
	simde__m64    v8;  // mm1
	simde__m64    v9;  // mm6
	simde__m64    v10; // mm2

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		do
		{
			v5  = simde_mm_cvtsi32_si64(*v4);
			v6  = v5;
			v7  = simde_m_psrldi(v5, 0x19u);
			v8  = simde_m_punpcklbw(v6, simde_mm_setzero_si64());
			v9  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v3), simde_mm_setzero_si64());
			v10 = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v7));
			++v4;
			++v3;
			*(v3 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v9,
						simde_m_psrawi(simde_m_pmullw(simde_m_psubw(v8, simde_m_psrlwi(simde_m_pmullw(v8, v9), 8u)), simde_m_punpckldq(v10, v10)), 7u)),
					simde_mm_setzero_si64()));
		} while (v4 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsScreenBlend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32   *v4;  // edi
	tjs_uint32   *v5;  // esi
	simde__m64    v6;  // mm4
	simde__m64    v7;  // mm2
	simde__m64    v8;  // mm1
	simde__m64    v9;  // mm6
	simde__m64    v10; // mm2
	simde__m64    v11; // mm2

	if (len > 0)
	{
		v4 = dest;
		v5 = (tjs_uint32   *)src;
		v6 = simde_mm_cvtsi32_si64(opa);
		do
		{
			v7  = simde_mm_cvtsi32_si64(*v5);
			v8  = simde_m_punpcklbw(v7, simde_mm_setzero_si64());
			v9  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v4), simde_mm_setzero_si64());
			v10 = simde_m_psrldi(simde_m_pmullw(simde_m_psrldi(v7, 0x19u), v6), 8u);
			v11 = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v10));
			++v5;
			++v4;
			*(v4 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v9,
						simde_m_psrawi(simde_m_pmullw(simde_m_psubw(v8, simde_m_psrlwi(simde_m_pmullw(v8, v9), 8u)), simde_m_punpckldq(v11, v11)), 7u)),
					simde_mm_setzero_si64()));
		} while (v5 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsScreenBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32   *v3; // edi
	tjs_uint32   *v4; // esi
	simde__m64    v5; // mm2
	simde__m64    v6; // mm1
	simde__m64    v7; // mm2
	simde__m64    v8; // mm1
	simde__m64    v9; // mm6

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		do
		{
			v5 = simde_mm_cvtsi32_si64(*v4);
			v6 = v5;
			v7 = simde_m_psrldi(v5, 0x19u);
			v8 = simde_m_punpcklbw(v6, simde_mm_setzero_si64());
			v9 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v3), simde_mm_setzero_si64());
			++v4;
			++v3;
			*(v3 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v9,
						simde_m_psrawi(
							simde_m_pmullw(
								simde_m_psubw(v8, simde_m_psrlwi(simde_m_pmullw(v8, v9), 8u)),
								simde_m_punpckldq(simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v7)), v7)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v4 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsScreenBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32   *v4;  // edi
	tjs_uint32   *v5;  // esi
	simde__m64    v6;  // mm4
	simde__m64    v7;  // mm2
	simde__m64    v8;  // mm1
	simde__m64    v9;  // mm6
	simde__m64    v10; // mm2

	if (len > 0)
	{
		v4 = dest;
		v5 = (tjs_uint32   *)src;
		v6 = simde_mm_cvtsi32_si64(opa);
		do
		{
			v7  = simde_mm_cvtsi32_si64(*v5);
			v8  = simde_m_punpcklbw(v7, simde_mm_setzero_si64());
			v9  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v4), simde_mm_setzero_si64());
			v10 = simde_m_psrldi(simde_m_pmullw(simde_m_psrldi(v7, 0x19u), v6), 8u);
			++v5;
			++v4;
			*(v4 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v9,
						simde_m_psrawi(
							simde_m_pmullw(
								simde_m_psubw(v8, simde_m_psrlwi(simde_m_pmullw(v8, v9), 8u)),
								simde_m_punpckldq(simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v10)), v10)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v5 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsOverlayBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32   *v3;  // edi
	tjs_uint32   *v4;  // esi
	simde__m64    v5;  // mm3
	simde__m64    v6;  // mm2
	simde__m64    v7;  // mm0
	simde__m64    v8;  // mm2
	simde__m64    v9;  // mm1
	simde__m64    v10; // mm0
	simde__m64    v11; // mm6
	simde__m64    v12; // mm0
	simde__m64    v13; // mm7
	simde__m64    v14; // mm2

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		v5 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(0xFFFFFFFF), simde_mm_setzero_si64());
		do
		{
			v6  = simde_mm_cvtsi32_si64(*v4);
			v7  = v6;
			v8  = simde_m_psrldi(v6, 0x19u);
			v9  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v3), simde_mm_setzero_si64());
			v10 = simde_m_punpcklbw(v7, simde_mm_setzero_si64());
			v11 = v10;
			v12 = simde_m_psrlwi(simde_m_pmullw(v10, v9), 7u);
			v13 = simde_m_pcmpgtw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(0x80808080), simde_mm_setzero_si64()), v9);
			v14 = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v8));
			++v4;
			++v3;
			*(v3 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v9,
						simde_m_psrawi(
							simde_m_pmullw(
								simde_m_psubw(
									simde_m_por(
										simde_m_pand(v12, v13),
										simde_m_pandn(v13, simde_m_psubw(simde_m_psubw(simde_m_psllwi(simde_m_paddw(v11, v9), 1u), v12), v5))),
									v9),
								simde_m_punpckldq(v14, v14)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v4 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsOverlayBlend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32   *v4;  // edi
	tjs_uint32   *v5;  // esi
	simde__m64    v6;  // mm4
	simde__m64    v7;  // mm3
	simde__m64    v8;  // mm2
	simde__m64    v9;  // mm1
	simde__m64    v10; // mm0
	simde__m64    v11; // mm6
	simde__m64    v12; // mm0
	simde__m64    v13; // mm7
	simde__m64    v14; // mm2
	simde__m64    v15; // mm2

	if (len > 0)
	{
		v4 = dest;
		v5 = (tjs_uint32   *)src;
		v6 = simde_mm_cvtsi32_si64(opa);
		v7 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(0xFFFFFFFF), simde_mm_setzero_si64());
		do
		{
			v8  = simde_mm_cvtsi32_si64(*v5);
			v9  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v4), simde_mm_setzero_si64());
			v10 = simde_m_punpcklbw(v8, simde_mm_setzero_si64());
			v11 = v10;
			v12 = simde_m_psrlwi(simde_m_pmullw(v10, v9), 7u);
			v13 = simde_m_pcmpgtw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(0x80808080), simde_mm_setzero_si64()), v9);
			v14 = simde_m_psrldi(simde_m_pmullw(simde_m_psrldi(v8, 0x19u), v6), 8u);
			v15 = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v14));
			++v5;
			++v4;
			*(v4 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v9,
						simde_m_psrawi(
							simde_m_pmullw(
								simde_m_psubw(
									simde_m_por(
										simde_m_pand(v12, v13),
										simde_m_pandn(v13, simde_m_psubw(simde_m_psubw(simde_m_psllwi(simde_m_paddw(v11, v9), 1u), v12), v7))),
									v9),
								simde_m_punpckldq(v15, v15)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v5 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsOverlayBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32   *v3;  // edi
	tjs_uint32   *v4;  // esi
	simde__m64    v5;  // mm3
	simde__m64    v6;  // mm2
	simde__m64    v7;  // mm0
	simde__m64    v8;  // mm2
	simde__m64    v9;  // mm1
	simde__m64    v10; // mm0
	simde__m64    v11; // mm6
	simde__m64    v12; // mm0
	simde__m64    v13; // mm7

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		v5 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(0xFFFFFFFF), simde_mm_setzero_si64());
		do
		{
			v6  = simde_mm_cvtsi32_si64(*v4);
			v7  = v6;
			v8  = simde_m_psrldi(v6, 0x19u);
			v9  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v3), simde_mm_setzero_si64());
			v10 = simde_m_punpcklbw(v7, simde_mm_setzero_si64());
			v11 = v10;
			v12 = simde_m_psrlwi(simde_m_pmullw(v10, v9), 7u);
			v13 = simde_m_pcmpgtw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(0x80808080), simde_mm_setzero_si64()), v9);
			++v4;
			++v3;
			*(v3 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v9,
						simde_m_psrawi(
							simde_m_pmullw(
								simde_m_psubw(
									simde_m_por(
										simde_m_pand(v12, v13),
										simde_m_pandn(v13, simde_m_psubw(simde_m_psubw(simde_m_psllwi(simde_m_paddw(v11, v9), 1u), v12), v5))),
									v9),
								simde_m_punpckldq(simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v8)), v8)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v4 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsOverlayBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32   *v4;  // edi
	tjs_uint32   *v5;  // esi
	simde__m64    v6;  // mm4
	simde__m64    v7;  // mm3
	simde__m64    v8;  // mm2
	simde__m64    v9;  // mm1
	simde__m64    v10; // mm0
	simde__m64    v11; // mm6
	simde__m64    v12; // mm0
	simde__m64    v13; // mm7
	simde__m64    v14; // mm2

	if (len > 0)
	{
		v4 = dest;
		v5 = (tjs_uint32   *)src;
		v6 = simde_mm_cvtsi32_si64(opa);
		v7 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(0xFFFFFFFF), simde_mm_setzero_si64());
		do
		{
			v8  = simde_mm_cvtsi32_si64(*v5);
			v9  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v4), simde_mm_setzero_si64());
			v10 = simde_m_punpcklbw(v8, simde_mm_setzero_si64());
			v11 = v10;
			v12 = simde_m_psrlwi(simde_m_pmullw(v10, v9), 7u);
			v13 = simde_m_pcmpgtw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(0x80808080), simde_mm_setzero_si64()), v9);
			v14 = simde_m_psrldi(simde_m_pmullw(simde_m_psrldi(v8, 0x19u), v6), 8u);
			++v5;
			++v4;
			*(v4 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v9,
						simde_m_psrawi(
							simde_m_pmullw(
								simde_m_psubw(
									simde_m_por(
										simde_m_pand(v12, v13),
										simde_m_pandn(v13, simde_m_psubw(simde_m_psubw(simde_m_psllwi(simde_m_paddw(v11, v9), 1u), v12), v7))),
									v9),
								simde_m_punpckldq(simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v14)), v14)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v5 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsHardLightBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32   *v3;  // edi
	tjs_uint32   *v4;  // esi
	simde__m64    v5;  // mm3
	simde__m64    v6;  // mm2
	simde__m64    v7;  // mm0
	simde__m64    v8;  // mm2
	simde__m64    v9;  // mm1
	simde__m64    v10; // mm0
	simde__m64    v11; // mm7
	simde__m64    v12; // mm6
	simde__m64    v13; // mm0
	simde__m64    v14; // mm2

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		v5 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(0xFFFFFFFF), simde_mm_setzero_si64());
		do
		{
			v6  = simde_mm_cvtsi32_si64(*v4);
			v7  = v6;
			v8  = simde_m_psrldi(v6, 0x19u);
			v9  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v3), simde_mm_setzero_si64());
			v10 = simde_m_punpcklbw(v7, simde_mm_setzero_si64());
			v11 = simde_m_pcmpgtw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(0x80808080), simde_mm_setzero_si64()), v10);
			v12 = simde_m_paddw(v10, v9);
			v13 = simde_m_psrlwi(simde_m_pmullw(v10, v9), 7u);
			v14 = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v8));
			++v4;
			++v3;
			*(v3 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v9,
						simde_m_psrawi(
							simde_m_pmullw(
								simde_m_psubw(
									simde_m_por(simde_m_pand(v13, v11), simde_m_pandn(v11, simde_m_psubw(simde_m_psubw(simde_m_psllwi(v12, 1u), v13), v5))),
									v9),
								simde_m_punpckldq(v14, v14)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v4 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsHardLightBlend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32   *v4;  // edi
	tjs_uint32   *v5;  // esi
	simde__m64    v6;  // mm4
	simde__m64    v7;  // mm3
	simde__m64    v8;  // mm2
	simde__m64    v9;  // mm1
	simde__m64    v10; // mm0
	simde__m64    v11; // mm7
	simde__m64    v12; // mm6
	simde__m64    v13; // mm0
	simde__m64    v14; // mm2
	simde__m64    v15; // mm2

	if (len > 0)
	{
		v4 = dest;
		v5 = (tjs_uint32   *)src;
		v6 = simde_mm_cvtsi32_si64(opa);
		v7 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(0xFFFFFFFF), simde_mm_setzero_si64());
		do
		{
			v8  = simde_mm_cvtsi32_si64(*v5);
			v9  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v4), simde_mm_setzero_si64());
			v10 = simde_m_punpcklbw(v8, simde_mm_setzero_si64());
			v11 = simde_m_pcmpgtw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(0x80808080), simde_mm_setzero_si64()), v10);
			v12 = simde_m_paddw(v10, v9);
			v13 = simde_m_psrlwi(simde_m_pmullw(v10, v9), 7u);
			v14 = simde_m_psrldi(simde_m_pmullw(simde_m_psrldi(v8, 0x19u), v6), 8u);
			v15 = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v14));
			++v5;
			++v4;
			*(v4 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v9,
						simde_m_psrawi(
							simde_m_pmullw(
								simde_m_psubw(
									simde_m_por(simde_m_pand(v13, v11), simde_m_pandn(v11, simde_m_psubw(simde_m_psubw(simde_m_psllwi(v12, 1u), v13), v7))),
									v9),
								simde_m_punpckldq(v15, v15)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v5 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsHardLightBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32   *v3;  // edi
	tjs_uint32   *v4;  // esi
	simde__m64    v5;  // mm3
	simde__m64    v6;  // mm2
	simde__m64    v7;  // mm0
	simde__m64    v8;  // mm2
	simde__m64    v9;  // mm1
	simde__m64    v10; // mm0
	simde__m64    v11; // mm7
	simde__m64    v12; // mm6
	simde__m64    v13; // mm0

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		v5 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(0xFFFFFFFF), simde_mm_setzero_si64());
		do
		{
			v6  = simde_mm_cvtsi32_si64(*v4);
			v7  = v6;
			v8  = simde_m_psrldi(v6, 0x19u);
			v9  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v3), simde_mm_setzero_si64());
			v10 = simde_m_punpcklbw(v7, simde_mm_setzero_si64());
			v11 = simde_m_pcmpgtw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(0x80808080), simde_mm_setzero_si64()), v10);
			v12 = simde_m_paddw(v10, v9);
			v13 = simde_m_psrlwi(simde_m_pmullw(v10, v9), 7u);
			++v4;
			++v3;
			*(v3 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v9,
						simde_m_psrawi(
							simde_m_pmullw(
								simde_m_psubw(
									simde_m_por(simde_m_pand(v13, v11), simde_m_pandn(v11, simde_m_psubw(simde_m_psubw(simde_m_psllwi(v12, 1u), v13), v5))),
									v9),
								simde_m_punpckldq(simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v8)), v8)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v4 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsHardLightBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32   *v4;  // edi
	tjs_uint32   *v5;  // esi
	simde__m64    v6;  // mm4
	simde__m64    v7;  // mm3
	simde__m64    v8;  // mm2
	simde__m64    v9;  // mm1
	simde__m64    v10; // mm0
	simde__m64    v11; // mm7
	simde__m64    v12; // mm6
	simde__m64    v13; // mm0
	simde__m64    v14; // mm2

	if (len > 0)
	{
		v4 = dest;
		v5 = (tjs_uint32   *)src;
		v6 = simde_mm_cvtsi32_si64(opa);
		v7 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(0xFFFFFFFF), simde_mm_setzero_si64());
		do
		{
			v8  = simde_mm_cvtsi32_si64(*v5);
			v9  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v4), simde_mm_setzero_si64());
			v10 = simde_m_punpcklbw(v8, simde_mm_setzero_si64());
			v11 = simde_m_pcmpgtw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(0x80808080), simde_mm_setzero_si64()), v10);
			v12 = simde_m_paddw(v10, v9);
			v13 = simde_m_psrlwi(simde_m_pmullw(v10, v9), 7u);
			v14 = simde_m_psrldi(simde_m_pmullw(simde_m_psrldi(v8, 0x19u), v6), 8u);
			++v5;
			++v4;
			*(v4 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v9,
						simde_m_psrawi(
							simde_m_pmullw(
								simde_m_psubw(
									simde_m_por(simde_m_pand(v13, v11), simde_m_pandn(v11, simde_m_psubw(simde_m_psubw(simde_m_psllwi(v12, 1u), v13), v7))),
									v9),
								simde_m_punpckldq(simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v14)), v14)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v5 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsSoftLightBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32   *v3;  // edi
	tjs_uint32   *v4;  // esi
	simde__m64    v5;  // mm2
	simde__m64    v6;  // mm1
	int           v7;  // ebx
	int           v8;  // edx
	tjs_uint16    v9;  // ax
	simde__m64    v10; // mm2
	simde__m64    v11; // mm1
	simde__m64    v12; // mm2

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		do
		{
			v5         = simde_mm_cvtsi32_si64(*v4);
			v6         = simde_mm_cvtsi32_si64(*v3);
			v7         = simde_mm_cvtsi64_si32(v5);
			v8         = simde_mm_cvtsi64_si32(v6);
			HIBYTE(v9) = v7;
			LOBYTE(v9) = v8;
			LOBYTE(v7) = TVPPsTableSoftLight[0][v9];
			HIBYTE(v9) = BYTE1(v7);
			LOBYTE(v9) = BYTE1(v8);
			BYTE1(v7)  = TVPPsTableSoftLight[0][v9];
			v7         = _rotr(v7, 16);
			HIBYTE(v9) = v7;
			LOBYTE(v9) = BYTE2(v8);
			LOBYTE(v7) = TVPPsTableSoftLight[0][v9];
			v10        = simde_m_psrldi(v5, 0x19u);
			v11        = simde_m_punpcklbw(v6, simde_mm_setzero_si64());
			v12        = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v10));
			++v4;
			++v3;
			*(v3 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v11,
						simde_m_psrawi(
							simde_m_pmullw(
								simde_m_psubw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(_rotr(v7, 16)), simde_mm_setzero_si64()), v11),
								simde_m_punpckldq(v12, v12)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v4 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsSoftLightBlend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32   *v4;  // edi
	tjs_uint32   *v5;  // esi
	simde__m64    v6;  // mm4
	simde__m64    v7;  // mm2
	simde__m64    v8;  // mm1
	int           v9;  // ebx
	int           v10; // edx
	tjs_uint16    v11; // ax
	simde__m64    v12; // mm1
	simde__m64    v13; // mm2
	simde__m64    v14; // mm2

	if (len > 0)
	{
		v4 = dest;
		v5 = (tjs_uint32   *)src;
		v6 = simde_mm_cvtsi32_si64(opa);
		do
		{
			v7          = simde_mm_cvtsi32_si64(*v5);
			v8          = simde_mm_cvtsi32_si64(*v4);
			v9          = simde_mm_cvtsi64_si32(v7);
			v10         = simde_mm_cvtsi64_si32(v8);
			HIBYTE(v11) = v9;
			LOBYTE(v11) = v10;
			LOBYTE(v9)  = TVPPsTableSoftLight[0][v11];
			HIBYTE(v11) = BYTE1(v9);
			LOBYTE(v11) = BYTE1(v10);
			BYTE1(v9)   = TVPPsTableSoftLight[0][v11];
			v9          = _rotr(v9, 16);
			HIBYTE(v11) = v9;
			LOBYTE(v11) = BYTE2(v10);
			LOBYTE(v9)  = TVPPsTableSoftLight[0][v11];
			v12         = simde_m_punpcklbw(v8, simde_mm_setzero_si64());
			v13         = simde_m_psrldi(simde_m_pmullw(simde_m_psrldi(v7, 0x19u), v6), 8u);
			v14         = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v13));
			++v5;
			++v4;
			*(v4 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v12,
						simde_m_psrawi(
							simde_m_pmullw(
								simde_m_psubw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(_rotr(v9, 16)), simde_mm_setzero_si64()), v12),
								simde_m_punpckldq(v14, v14)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v5 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsSoftLightBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32   *v3;  // edi
	tjs_uint32   *v4;  // esi
	simde__m64    v5;  // mm2
	simde__m64    v6;  // mm1
	int           v7;  // ebx
	int           v8;  // edx
	tjs_uint16    v9;  // ax
	simde__m64    v10; // mm2
	simde__m64    v11; // mm1

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		do
		{
			v5         = simde_mm_cvtsi32_si64(*v4);
			v6         = simde_mm_cvtsi32_si64(*v3);
			v7         = simde_mm_cvtsi64_si32(v5);
			v8         = simde_mm_cvtsi64_si32(v6);
			HIBYTE(v9) = v7;
			LOBYTE(v9) = v8;
			LOBYTE(v7) = TVPPsTableSoftLight[0][v9];
			HIBYTE(v9) = BYTE1(v7);
			LOBYTE(v9) = BYTE1(v8);
			BYTE1(v7)  = TVPPsTableSoftLight[0][v9];
			v7         = _rotr(v7, 16);
			HIBYTE(v9) = v7;
			LOBYTE(v9) = BYTE2(v8);
			LOBYTE(v7) = TVPPsTableSoftLight[0][v9];
			v10        = simde_m_psrldi(v5, 0x19u);
			v11        = simde_m_punpcklbw(v6, simde_mm_setzero_si64());
			++v4;
			++v3;
			*(v3 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v11,
						simde_m_psrawi(
							simde_m_pmullw(
								simde_m_psubw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(_rotr(v7, 16)), simde_mm_setzero_si64()), v11),
								simde_m_punpckldq(simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v10)), v10)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v4 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsSoftLightBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32   *v4;  // edi
	tjs_uint32   *v5;  // esi
	simde__m64    v6;  // mm4
	simde__m64    v7;  // mm2
	simde__m64    v8;  // mm1
	int           v9;  // ebx
	int           v10; // edx
	tjs_uint16    v11; // ax
	simde__m64    v12; // mm1
	simde__m64    v13; // mm2

	if (len > 0)
	{
		v4 = dest;
		v5 = (tjs_uint32   *)src;
		v6 = simde_mm_cvtsi32_si64(opa);
		do
		{
			v7          = simde_mm_cvtsi32_si64(*v5);
			v8          = simde_mm_cvtsi32_si64(*v4);
			v9          = simde_mm_cvtsi64_si32(v7);
			v10         = simde_mm_cvtsi64_si32(v8);
			HIBYTE(v11) = v9;
			LOBYTE(v11) = v10;
			LOBYTE(v9)  = TVPPsTableSoftLight[0][v11];
			HIBYTE(v11) = BYTE1(v9);
			LOBYTE(v11) = BYTE1(v10);
			BYTE1(v9)   = TVPPsTableSoftLight[0][v11];
			v9          = _rotr(v9, 16);
			HIBYTE(v11) = v9;
			LOBYTE(v11) = BYTE2(v10);
			LOBYTE(v9)  = TVPPsTableSoftLight[0][v11];
			v12         = simde_m_punpcklbw(v8, simde_mm_setzero_si64());
			v13         = simde_m_psrldi(simde_m_pmullw(simde_m_psrldi(v7, 0x19u), v6), 8u);
			++v5;
			++v4;
			*(v4 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v12,
						simde_m_psrawi(
							simde_m_pmullw(
								simde_m_psubw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(_rotr(v9, 16)), simde_mm_setzero_si64()), v12),
								simde_m_punpckldq(simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v13)), v13)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v5 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsColorDodgeBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32   *v3;  // edi
	tjs_uint32   *v4;  // esi
	simde__m64    v5;  // mm2
	simde__m64    v6;  // mm1
	int           v7;  // ebx
	int           v8;  // edx
	tjs_uint16    v9;  // ax
	simde__m64    v10; // mm2
	simde__m64    v11; // mm1
	simde__m64    v12; // mm2

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		do
		{
			v5         = simde_mm_cvtsi32_si64(*v4);
			v6         = simde_mm_cvtsi32_si64(*v3);
			v7         = simde_mm_cvtsi64_si32(v5);
			v8         = simde_mm_cvtsi64_si32(v6);
			HIBYTE(v9) = v7;
			LOBYTE(v9) = v8;
			LOBYTE(v7) = TVPPsTableColorDodge[0][v9];
			HIBYTE(v9) = BYTE1(v7);
			LOBYTE(v9) = BYTE1(v8);
			BYTE1(v7)  = TVPPsTableColorDodge[0][v9];
			v7         = _rotr(v7, 16);
			HIBYTE(v9) = v7;
			LOBYTE(v9) = BYTE2(v8);
			LOBYTE(v7) = TVPPsTableColorDodge[0][v9];
			v10        = simde_m_psrldi(v5, 0x19u);
			v11        = simde_m_punpcklbw(v6, simde_mm_setzero_si64());
			v12        = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v10));
			++v4;
			++v3;
			*(v3 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v11,
						simde_m_psrawi(
							simde_m_pmullw(
								simde_m_psubw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(_rotr(v7, 16)), simde_mm_setzero_si64()), v11),
								simde_m_punpckldq(v12, v12)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v4 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsColorDodgeBlend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32   *v4;  // edi
	tjs_uint32   *v5;  // esi
	simde__m64    v6;  // mm4
	simde__m64    v7;  // mm2
	simde__m64    v8;  // mm1
	int           v9;  // ebx
	int           v10; // edx
	tjs_uint16    v11; // ax
	simde__m64    v12; // mm1
	simde__m64    v13; // mm2
	simde__m64    v14; // mm2

	if (len > 0)
	{
		v4 = dest;
		v5 = (tjs_uint32   *)src;
		v6 = simde_mm_cvtsi32_si64(opa);
		do
		{
			v7          = simde_mm_cvtsi32_si64(*v5);
			v8          = simde_mm_cvtsi32_si64(*v4);
			v9          = simde_mm_cvtsi64_si32(v7);
			v10         = simde_mm_cvtsi64_si32(v8);
			HIBYTE(v11) = v9;
			LOBYTE(v11) = v10;
			LOBYTE(v9)  = TVPPsTableColorDodge[0][v11];
			HIBYTE(v11) = BYTE1(v9);
			LOBYTE(v11) = BYTE1(v10);
			BYTE1(v9)   = TVPPsTableColorDodge[0][v11];
			v9          = _rotr(v9, 16);
			HIBYTE(v11) = v9;
			LOBYTE(v11) = BYTE2(v10);
			LOBYTE(v9)  = TVPPsTableColorDodge[0][v11];
			v12         = simde_m_punpcklbw(v8, simde_mm_setzero_si64());
			v13         = simde_m_psrldi(simde_m_pmullw(simde_m_psrldi(v7, 0x19u), v6), 8u);
			v14         = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v13));
			++v5;
			++v4;
			*(v4 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v12,
						simde_m_psrawi(
							simde_m_pmullw(
								simde_m_psubw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(_rotr(v9, 16)), simde_mm_setzero_si64()), v12),
								simde_m_punpckldq(v14, v14)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v5 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsColorDodgeBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32   *v3;  // edi
	tjs_uint32   *v4;  // esi
	simde__m64    v5;  // mm2
	simde__m64    v6;  // mm1
	int           v7;  // ebx
	int           v8;  // edx
	tjs_uint16    v9;  // ax
	simde__m64    v10; // mm2
	simde__m64    v11; // mm1

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		do
		{
			v5         = simde_mm_cvtsi32_si64(*v4);
			v6         = simde_mm_cvtsi32_si64(*v3);
			v7         = simde_mm_cvtsi64_si32(v5);
			v8         = simde_mm_cvtsi64_si32(v6);
			HIBYTE(v9) = v7;
			LOBYTE(v9) = v8;
			LOBYTE(v7) = TVPPsTableColorDodge[0][v9];
			HIBYTE(v9) = BYTE1(v7);
			LOBYTE(v9) = BYTE1(v8);
			BYTE1(v7)  = TVPPsTableColorDodge[0][v9];
			v7         = _rotr(v7, 16);
			HIBYTE(v9) = v7;
			LOBYTE(v9) = BYTE2(v8);
			LOBYTE(v7) = TVPPsTableColorDodge[0][v9];
			v10        = simde_m_psrldi(v5, 0x19u);
			v11        = simde_m_punpcklbw(v6, simde_mm_setzero_si64());
			++v4;
			++v3;
			*(v3 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v11,
						simde_m_psrawi(
							simde_m_pmullw(
								simde_m_psubw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(_rotr(v7, 16)), simde_mm_setzero_si64()), v11),
								simde_m_punpckldq(simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v10)), v10)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v4 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsColorDodgeBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32   *v4;  // edi
	tjs_uint32   *v5;  // esi
	simde__m64    v6;  // mm4
	simde__m64    v7;  // mm2
	simde__m64    v8;  // mm1
	int           v9;  // ebx
	int           v10; // edx
	tjs_uint16    v11; // ax
	simde__m64    v12; // mm1
	simde__m64    v13; // mm2

	if (len > 0)
	{
		v4 = dest;
		v5 = (tjs_uint32   *)src;
		v6 = simde_mm_cvtsi32_si64(opa);
		do
		{
			v7          = simde_mm_cvtsi32_si64(*v5);
			v8          = simde_mm_cvtsi32_si64(*v4);
			v9          = simde_mm_cvtsi64_si32(v7);
			v10         = simde_mm_cvtsi64_si32(v8);
			HIBYTE(v11) = v9;
			LOBYTE(v11) = v10;
			LOBYTE(v9)  = TVPPsTableColorDodge[0][v11];
			HIBYTE(v11) = BYTE1(v9);
			LOBYTE(v11) = BYTE1(v10);
			BYTE1(v9)   = TVPPsTableColorDodge[0][v11];
			v9          = _rotr(v9, 16);
			HIBYTE(v11) = v9;
			LOBYTE(v11) = BYTE2(v10);
			LOBYTE(v9)  = TVPPsTableColorDodge[0][v11];
			v12         = simde_m_punpcklbw(v8, simde_mm_setzero_si64());
			v13         = simde_m_psrldi(simde_m_pmullw(simde_m_psrldi(v7, 0x19u), v6), 8u);
			++v5;
			++v4;
			*(v4 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v12,
						simde_m_psrawi(
							simde_m_pmullw(
								simde_m_psubw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(_rotr(v9, 16)), simde_mm_setzero_si64()), v12),
								simde_m_punpckldq(simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v13)), v13)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v5 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsColorDodge5Blend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32   *v3;  // edi
	tjs_uint32   *v4;  // esi
	simde__m64    v5;  // mm2
	simde__m64    v6;  // mm0
	simde__m64    v7;  // mm2
	int           v8;  // edx
	simde__m64    v9;  // mm2
	int           v10; // ebx
	tjs_uint16    v11; // ax

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		do
		{
			v5 = simde_mm_cvtsi32_si64(*v4);
			v6 = v5;
			v7 = simde_m_psrldi(v5, 0x19u);
			v8 = simde_mm_cvtsi64_si32(simde_mm_cvtsi32_si64(*v3));
			v9 = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v7));
			++v4;
			v10         = simde_mm_cvtsi64_si32(simde_m_packuswb(simde_m_psrawi(simde_m_pmullw(simde_m_punpcklbw(v6, simde_mm_setzero_si64()), simde_m_punpckldq(v9, v9)), 7u), simde_mm_setzero_si64()));
			HIBYTE(v11) = v10;
			LOBYTE(v11) = v8;
			LOBYTE(v10) = TVPPsTableColorDodge[0][v11];
			HIBYTE(v11) = BYTE1(v10);
			++v3;
			LOBYTE(v11) = BYTE1(v8);
			BYTE1(v10)  = TVPPsTableColorDodge[0][v11];
			v10         = _rotr(v10, 16);
			HIBYTE(v11) = v10;
			LOBYTE(v11) = BYTE2(v8);
			LOBYTE(v10) = TVPPsTableColorDodge[0][v11];
			*(v3 - 1)   = _rotr(v10, 16);
		} while (v4 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsColorDodge5Blend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32   *v4;  // edi
	tjs_uint32   *v5;  // esi
	simde__m64    v6;  // mm4
	simde__m64    v7;  // mm2
	simde__m64    v8;  // mm0
	int           v9;  // edx
	simde__m64    v10; // mm2
	simde__m64    v11; // mm2
	int           v12; // ebx
	tjs_uint16    v13; // ax

	if (len > 0)
	{
		v4 = dest;
		v5 = (tjs_uint32   *)src;
		v6 = simde_mm_cvtsi32_si64(opa);
		do
		{
			v7  = simde_mm_cvtsi32_si64(*v5);
			v8  = v7;
			v9  = simde_mm_cvtsi64_si32(simde_mm_cvtsi32_si64(*v4));
			v10 = simde_m_psrldi(simde_m_pmullw(simde_m_psrldi(v7, 0x19u), v6), 8u);
			v11 = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v10));
			++v5;
			v12         = simde_mm_cvtsi64_si32(simde_m_packuswb(simde_m_psrawi(simde_m_pmullw(simde_m_punpcklbw(v8, simde_mm_setzero_si64()), simde_m_punpckldq(v11, v11)), 7u), simde_mm_setzero_si64()));
			HIBYTE(v13) = v12;
			LOBYTE(v13) = v9;
			LOBYTE(v12) = TVPPsTableColorDodge[0][v13];
			HIBYTE(v13) = BYTE1(v12);
			++v4;
			LOBYTE(v13) = BYTE1(v9);
			BYTE1(v12)  = TVPPsTableColorDodge[0][v13];
			v12         = _rotr(v12, 16);
			HIBYTE(v13) = v12;
			LOBYTE(v13) = BYTE2(v9);
			LOBYTE(v12) = TVPPsTableColorDodge[0][v13];
			*(v4 - 1)   = _rotr(v12, 16);
		} while (v5 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsColorDodge5Blend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32   *v3;  // edi
	tjs_uint32   *v4;  // esi
	simde__m64    v5;  // mm2
	simde__m64    v6;  // mm0
	simde__m64    v7;  // mm2
	tjs_uint32    v8;  // edx
	simde__m64    v9;  // mm2
	int           v10; // ebx
	tjs_uint16    v11; // ax

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		do
		{
			v5 = simde_mm_cvtsi32_si64(*v4);
			v6 = v5;
			v7 = simde_m_psrldi(v5, 0x19u);
			v8 = simde_mm_cvtsi64_si32(simde_mm_cvtsi32_si64(*v3));
			v9 = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v7));
			++v4;
			v10         = simde_mm_cvtsi64_si32(simde_m_packuswb(simde_m_psrawi(simde_m_pmullw(simde_m_punpcklbw(v6, simde_mm_setzero_si64()), simde_m_punpckldq(v9, v9)), 7u), simde_mm_setzero_si64()));
			HIBYTE(v11) = v10;
			LOBYTE(v11) = v8;
			LOBYTE(v10) = TVPPsTableColorDodge[0][v11];
			HIBYTE(v11) = BYTE1(v10);
			++v3;
			LOBYTE(v11) = BYTE1(v8);
			v8 >>= 16;
			BYTE1(v10)  = TVPPsTableColorDodge[0][v11];
			v10         = _rotr(v10, 16);
			HIBYTE(v11) = v10;
			BYTE1(v10)  = BYTE1(v8);
			LOBYTE(v11) = v8;
			LOBYTE(v10) = TVPPsTableColorDodge[0][v11];
			*(v3 - 1)   = _rotr(v10, 16);
		} while (v4 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsColorDodge5Blend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32   *v4;  // edi
	tjs_uint32   *v5;  // esi
	simde__m64    v6;  // mm4
	simde__m64    v7;  // mm2
	simde__m64    v8;  // mm0
	tjs_uint32    v9;  // edx
	simde__m64    v10; // mm2
	simde__m64    v11; // mm2
	int           v12; // ebx
	tjs_uint16    v13; // ax

	if (len > 0)
	{
		v4 = dest;
		v5 = (tjs_uint32   *)src;
		v6 = simde_mm_cvtsi32_si64(opa);
		do
		{
			v7  = simde_mm_cvtsi32_si64(*v5);
			v8  = v7;
			v9  = simde_mm_cvtsi64_si32(simde_mm_cvtsi32_si64(*v4));
			v10 = simde_m_psrldi(simde_m_pmullw(simde_m_psrldi(v7, 0x19u), v6), 8u);
			v11 = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v10));
			++v5;
			v12         = simde_mm_cvtsi64_si32(simde_m_packuswb(simde_m_psrawi(simde_m_pmullw(simde_m_punpcklbw(v8, simde_mm_setzero_si64()), simde_m_punpckldq(v11, v11)), 7u), simde_mm_setzero_si64()));
			HIBYTE(v13) = v12;
			LOBYTE(v13) = v9;
			LOBYTE(v12) = TVPPsTableColorDodge[0][v13];
			HIBYTE(v13) = BYTE1(v12);
			++v4;
			LOBYTE(v13) = BYTE1(v9);
			v9 >>= 16;
			BYTE1(v12)  = TVPPsTableColorDodge[0][v13];
			v12         = _rotr(v12, 16);
			HIBYTE(v13) = v12;
			BYTE1(v12)  = BYTE1(v9);
			LOBYTE(v13) = v9;
			LOBYTE(v12) = TVPPsTableColorDodge[0][v13];
			*(v4 - 1)   = _rotr(v12, 16);
		} while (v5 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsColorBurnBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32   *v3;  // edi
	tjs_uint32   *v4;  // esi
	simde__m64    v5;  // mm2
	simde__m64    v6;  // mm1
	int           v7;  // ebx
	int           v8;  // edx
	tjs_uint16    v9;  // ax
	simde__m64    v10; // mm2
	simde__m64    v11; // mm1
	simde__m64    v12; // mm2

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		do
		{
			v5         = simde_mm_cvtsi32_si64(*v4);
			v6         = simde_mm_cvtsi32_si64(*v3);
			v7         = simde_mm_cvtsi64_si32(v5);
			v8         = simde_mm_cvtsi64_si32(v6);
			HIBYTE(v9) = v7;
			LOBYTE(v9) = v8;
			LOBYTE(v7) = TVPPsTableColorBurn[0][v9];
			HIBYTE(v9) = BYTE1(v7);
			LOBYTE(v9) = BYTE1(v8);
			BYTE1(v7)  = TVPPsTableColorBurn[0][v9];
			v7         = _rotr(v7, 16);
			HIBYTE(v9) = v7;
			LOBYTE(v9) = BYTE2(v8);
			LOBYTE(v7) = TVPPsTableColorBurn[0][v9];
			v10        = simde_m_psrldi(v5, 0x19u);
			v11        = simde_m_punpcklbw(v6, simde_mm_setzero_si64());
			v12        = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v10));
			++v4;
			++v3;
			*(v3 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v11,
						simde_m_psrawi(
							simde_m_pmullw(
								simde_m_psubw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(_rotr(v7, 16)), simde_mm_setzero_si64()), v11),
								simde_m_punpckldq(v12, v12)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v4 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsColorBurnBlend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32   *v4;  // edi
	tjs_uint32   *v5;  // esi
	simde__m64    v6;  // mm4
	simde__m64    v7;  // mm2
	simde__m64    v8;  // mm1
	int           v9;  // ebx
	int           v10; // edx
	tjs_uint16    v11; // ax
	simde__m64    v12; // mm1
	simde__m64    v13; // mm2
	simde__m64    v14; // mm2

	if (len > 0)
	{
		v4 = dest;
		v5 = (tjs_uint32   *)src;
		v6 = simde_mm_cvtsi32_si64(opa);
		do
		{
			v7          = simde_mm_cvtsi32_si64(*v5);
			v8          = simde_mm_cvtsi32_si64(*v4);
			v9          = simde_mm_cvtsi64_si32(v7);
			v10         = simde_mm_cvtsi64_si32(v8);
			HIBYTE(v11) = v9;
			LOBYTE(v11) = v10;
			LOBYTE(v9)  = TVPPsTableColorBurn[0][v11];
			HIBYTE(v11) = BYTE1(v9);
			LOBYTE(v11) = BYTE1(v10);
			BYTE1(v9)   = TVPPsTableColorBurn[0][v11];
			v9          = _rotr(v9, 16);
			HIBYTE(v11) = v9;
			LOBYTE(v11) = BYTE2(v10);
			LOBYTE(v9)  = TVPPsTableColorBurn[0][v11];
			v12         = simde_m_punpcklbw(v8, simde_mm_setzero_si64());
			v13         = simde_m_psrldi(simde_m_pmullw(simde_m_psrldi(v7, 0x19u), v6), 8u);
			v14         = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v13));
			++v5;
			++v4;
			*(v4 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v12,
						simde_m_psrawi(
							simde_m_pmullw(
								simde_m_psubw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(_rotr(v9, 16)), simde_mm_setzero_si64()), v12),
								simde_m_punpckldq(v14, v14)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v5 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsColorBurnBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32   *v3;  // edi
	tjs_uint32   *v4;  // esi
	simde__m64    v5;  // mm2
	simde__m64    v6;  // mm1
	int           v7;  // ebx
	int           v8;  // edx
	tjs_uint16    v9;  // ax
	simde__m64    v10; // mm2
	simde__m64    v11; // mm1

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		do
		{
			v5         = simde_mm_cvtsi32_si64(*v4);
			v6         = simde_mm_cvtsi32_si64(*v3);
			v7         = simde_mm_cvtsi64_si32(v5);
			v8         = simde_mm_cvtsi64_si32(v6);
			HIBYTE(v9) = v7;
			LOBYTE(v9) = v8;
			LOBYTE(v7) = TVPPsTableColorBurn[0][v9];
			HIBYTE(v9) = BYTE1(v7);
			LOBYTE(v9) = BYTE1(v8);
			BYTE1(v7)  = TVPPsTableColorBurn[0][v9];
			v7         = _rotr(v7, 16);
			HIBYTE(v9) = v7;
			LOBYTE(v9) = BYTE2(v8);
			LOBYTE(v7) = TVPPsTableColorBurn[0][v9];
			v10        = simde_m_psrldi(v5, 0x19u);
			v11        = simde_m_punpcklbw(v6, simde_mm_setzero_si64());
			++v4;
			++v3;
			*(v3 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v11,
						simde_m_psrawi(
							simde_m_pmullw(
								simde_m_psubw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(_rotr(v7, 16)), simde_mm_setzero_si64()), v11),
								simde_m_punpckldq(simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v10)), v10)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v4 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsColorBurnBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32   *v4;  // edi
	tjs_uint32   *v5;  // esi
	simde__m64    v6;  // mm4
	simde__m64    v7;  // mm2
	simde__m64    v8;  // mm1
	int           v9;  // ebx
	int           v10; // edx
	tjs_uint16    v11; // ax
	simde__m64    v12; // mm1
	simde__m64    v13; // mm2

	if (len > 0)
	{
		v4 = dest;
		v5 = (tjs_uint32   *)src;
		v6 = simde_mm_cvtsi32_si64(opa);
		do
		{
			v7          = simde_mm_cvtsi32_si64(*v5);
			v8          = simde_mm_cvtsi32_si64(*v4);
			v9          = simde_mm_cvtsi64_si32(v7);
			v10         = simde_mm_cvtsi64_si32(v8);
			HIBYTE(v11) = v9;
			LOBYTE(v11) = v10;
			LOBYTE(v9)  = TVPPsTableColorBurn[0][v11];
			HIBYTE(v11) = BYTE1(v9);
			LOBYTE(v11) = BYTE1(v10);
			BYTE1(v9)   = TVPPsTableColorBurn[0][v11];
			v9          = _rotr(v9, 16);
			HIBYTE(v11) = v9;
			LOBYTE(v11) = BYTE2(v10);
			LOBYTE(v9)  = TVPPsTableColorBurn[0][v11];
			v12         = simde_m_punpcklbw(v8, simde_mm_setzero_si64());
			v13         = simde_m_psrldi(simde_m_pmullw(simde_m_psrldi(v7, 0x19u), v6), 8u);
			++v5;
			++v4;
			*(v4 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v12,
						simde_m_psrawi(
							simde_m_pmullw(
								simde_m_psubw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(_rotr(v9, 16)), simde_mm_setzero_si64()), v12),
								simde_m_punpckldq(simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v13)), v13)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v5 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsLightenBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32   *v3;  // edi
	tjs_uint32   *v4;  // esi
	simde__m64    v5;  // mm6
	simde__m64    v6;  // mm0
	simde__m64    v7;  // mm6
	simde__m64    v8;  // mm1
	simde__m64    v9;  // mm0
	simde__m64    v10; // mm6

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		do
		{
			v5  = simde_mm_cvtsi32_si64(*v4);
			v6  = v5;
			v7  = simde_m_psrldi(v5, 0x19u);
			v8  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v3), simde_mm_setzero_si64());
			v9  = simde_m_punpcklbw(v6, simde_mm_setzero_si64());
			v10 = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v7));
			++v4;
			++v3;
			*(v3 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v8,
						simde_m_psrawi(simde_m_pmullw(simde_m_psubw(simde_m_paddb(v9, simde_m_psubusb(v8, v9)), v8), simde_m_punpckldq(v10, v10)), 7u)),
					simde_mm_setzero_si64()));
		} while (v4 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsLightenBlend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32   *v4;  // edi
	tjs_uint32   *v5;  // esi
	simde__m64    v6;  // mm4
	simde__m64    v7;  // mm6
	simde__m64    v8;  // mm0
	simde__m64    v9;  // mm1
	simde__m64    v10; // mm6
	simde__m64    v11; // mm0
	simde__m64    v12; // mm6

	if (len > 0)
	{
		v4 = dest;
		v5 = (tjs_uint32   *)src;
		v6 = simde_mm_cvtsi32_si64(opa);
		do
		{
			v7  = simde_mm_cvtsi32_si64(*v5);
			v8  = v7;
			v9  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v4), simde_mm_setzero_si64());
			v10 = simde_m_psrldi(simde_m_pmullw(simde_m_psrldi(v7, 0x19u), v6), 8u);
			v11 = simde_m_punpcklbw(v8, simde_mm_setzero_si64());
			v12 = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v10));
			++v5;
			++v4;
			*(v4 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v9,
						simde_m_psrawi(
							simde_m_pmullw(simde_m_psubw(simde_m_paddb(v11, simde_m_psubusb(v9, v11)), v9), simde_m_punpckldq(v12, v12)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v5 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsLightenBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32   *v3; // edi
	tjs_uint32   *v4; // esi
	simde__m64    v5; // mm6
	simde__m64    v6; // mm0
	simde__m64    v7; // mm6
	simde__m64    v8; // mm1
	simde__m64    v9; // mm0

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		do
		{
			v5 = simde_mm_cvtsi32_si64(*v4);
			v6 = v5;
			v7 = simde_m_psrldi(v5, 0x19u);
			v8 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v3), simde_mm_setzero_si64());
			v9 = simde_m_punpcklbw(v6, simde_mm_setzero_si64());
			++v4;
			++v3;
			*(v3 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v8,
						simde_m_psrawi(
							simde_m_pmullw(
								simde_m_psubw(simde_m_paddb(v9, simde_m_psubusb(v8, v9)), v8),
								simde_m_punpckldq(simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v7)), v7)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v4 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsLightenBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32   *v4;  // edi
	tjs_uint32   *v5;  // esi
	simde__m64    v6;  // mm4
	simde__m64    v7;  // mm6
	simde__m64    v8;  // mm0
	simde__m64    v9;  // mm1
	simde__m64    v10; // mm6
	simde__m64    v11; // mm0

	if (len > 0)
	{
		v4 = dest;
		v5 = (tjs_uint32   *)src;
		v6 = simde_mm_cvtsi32_si64(opa);
		do
		{
			v7  = simde_mm_cvtsi32_si64(*v5);
			v8  = v7;
			v9  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v4), simde_mm_setzero_si64());
			v10 = simde_m_psrldi(simde_m_pmullw(simde_m_psrldi(v7, 0x19u), v6), 8u);
			v11 = simde_m_punpcklbw(v8, simde_mm_setzero_si64());
			++v5;
			++v4;
			*(v4 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v9,
						simde_m_psrawi(
							simde_m_pmullw(
								simde_m_psubw(simde_m_paddb(v11, simde_m_psubusb(v9, v11)), v9),
								simde_m_punpckldq(simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v10)), v10)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v5 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsDarkenBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32   *v3;  // edi
	tjs_uint32   *v4;  // esi
	simde__m64    v5;  // mm6
	simde__m64    v6;  // mm0
	simde__m64    v7;  // mm6
	simde__m64    v8;  // mm0
	simde__m64    v9;  // mm1
	simde__m64    v10; // mm6

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		do
		{
			v5  = simde_mm_cvtsi32_si64(*v4);
			v6  = v5;
			v7  = simde_m_psrldi(v5, 0x19u);
			v8  = simde_m_punpcklbw(v6, simde_mm_setzero_si64());
			v9  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v3), simde_mm_setzero_si64());
			v10 = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v7));
			++v4;
			++v3;
			*(v3 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v9,
						simde_m_psrawi(simde_m_pmullw(simde_m_psubw(simde_m_psubb(v8, simde_m_psubusb(v8, v9)), v9), simde_m_punpckldq(v10, v10)), 7u)),
					simde_mm_setzero_si64()));
		} while (v4 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsDarkenBlend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32   *v4;  // edi
	tjs_uint32   *v5;  // esi
	simde__m64    v6;  // mm4
	simde__m64    v7;  // mm6
	simde__m64    v8;  // mm0
	simde__m64    v9;  // mm1
	simde__m64    v10; // mm6
	simde__m64    v11; // mm6

	if (len > 0)
	{
		v4 = dest;
		v5 = (tjs_uint32   *)src;
		v6 = simde_mm_cvtsi32_si64(opa);
		do
		{
			v7  = simde_mm_cvtsi32_si64(*v5);
			v8  = simde_m_punpcklbw(v7, simde_mm_setzero_si64());
			v9  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v4), simde_mm_setzero_si64());
			v10 = simde_m_psrldi(simde_m_pmullw(simde_m_psrldi(v7, 0x19u), v6), 8u);
			v11 = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v10));
			++v5;
			++v4;
			*(v4 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v9,
						simde_m_psrawi(simde_m_pmullw(simde_m_psubw(simde_m_psubb(v8, simde_m_psubusb(v8, v9)), v9), simde_m_punpckldq(v11, v11)), 7u)),
					simde_mm_setzero_si64()));
		} while (v5 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsDarkenBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32   *v3; // edi
	tjs_uint32   *v4; // esi
	simde__m64    v5; // mm6
	simde__m64    v6; // mm0
	simde__m64    v7; // mm6
	simde__m64    v8; // mm0
	simde__m64    v9; // mm1

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		do
		{
			v5 = simde_mm_cvtsi32_si64(*v4);
			v6 = v5;
			v7 = simde_m_psrldi(v5, 0x19u);
			v8 = simde_m_punpcklbw(v6, simde_mm_setzero_si64());
			v9 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v3), simde_mm_setzero_si64());
			++v4;
			++v3;
			*(v3 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v9,
						simde_m_psrawi(
							simde_m_pmullw(
								simde_m_psubw(simde_m_psubb(v8, simde_m_psubusb(v8, v9)), v9),
								simde_m_punpckldq(simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v7)), v7)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v4 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsDarkenBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32   *v4;  // edi
	tjs_uint32   *v5;  // esi
	simde__m64    v6;  // mm4
	simde__m64    v7;  // mm6
	simde__m64    v8;  // mm0
	simde__m64    v9;  // mm1
	simde__m64    v10; // mm6

	if (len > 0)
	{
		v4 = dest;
		v5 = (tjs_uint32   *)src;
		v6 = simde_mm_cvtsi32_si64(opa);
		do
		{
			v7  = simde_mm_cvtsi32_si64(*v5);
			v8  = simde_m_punpcklbw(v7, simde_mm_setzero_si64());
			v9  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v4), simde_mm_setzero_si64());
			v10 = simde_m_psrldi(simde_m_pmullw(simde_m_psrldi(v7, 0x19u), v6), 8u);
			++v5;
			++v4;
			*(v4 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v9,
						simde_m_psrawi(
							simde_m_pmullw(
								simde_m_psubw(simde_m_psubb(v8, simde_m_psubusb(v8, v9)), v9),
								simde_m_punpckldq(simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v10)), v10)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v5 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsDiffBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32   *v3;  // edi
	tjs_uint32   *v4;  // esi
	simde__m64    v5;  // mm2
	simde__m64    v6;  // mm1
	simde__m64    v7;  // mm3
	simde__m64    v8;  // mm2
	simde__m64    v9;  // mm0
	simde__m64    v10; // mm1
	simde__m64    v11; // mm2

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		do
		{
			v5  = simde_mm_cvtsi32_si64(*v4);
			v6  = simde_mm_cvtsi32_si64(*v3);
			v7  = v5;
			v8  = simde_m_psrldi(v5, 0x19u);
			v9  = simde_m_paddb(simde_m_psubusb(v6, v7), simde_m_psubusb(v7, v6));
			v10 = simde_m_punpcklbw(v6, simde_mm_setzero_si64());
			v11 = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v8));
			++v4;
			++v3;
			*(v3 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v10,
						simde_m_psrawi(simde_m_pmullw(simde_m_psubw(simde_m_punpcklbw(v9, simde_mm_setzero_si64()), v10), simde_m_punpckldq(v11, v11)), 7u)),
					simde_mm_setzero_si64()));
		} while (v4 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsDiffBlend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32   *v4;  // edi
	tjs_uint32   *v5;  // esi
	simde__m64    v6;  // mm4
	simde__m64    v7;  // mm2
	simde__m64    v8;  // mm1
	simde__m64    v9;  // mm0
	simde__m64    v10; // mm2
	simde__m64    v11; // mm1
	simde__m64    v12; // mm2

	if (len > 0)
	{
		v4 = dest;
		v5 = (tjs_uint32   *)src;
		v6 = simde_mm_cvtsi32_si64(opa);
		do
		{
			v7  = simde_mm_cvtsi32_si64(*v5);
			v8  = simde_mm_cvtsi32_si64(*v4);
			v9  = simde_m_paddb(simde_m_psubusb(v8, v7), simde_m_psubusb(v7, v8));
			v10 = simde_m_psrldi(simde_m_pmullw(simde_m_psrldi(v7, 0x19u), v6), 8u);
			v11 = simde_m_punpcklbw(v8, simde_mm_setzero_si64());
			v12 = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v10));
			++v5;
			++v4;
			*(v4 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v11,
						simde_m_psrawi(simde_m_pmullw(simde_m_psubw(simde_m_punpcklbw(v9, simde_mm_setzero_si64()), v11), simde_m_punpckldq(v12, v12)), 7u)),
					simde_mm_setzero_si64()));
		} while (v5 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsDiffBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32   *v3;  // edi
	tjs_uint32   *v4;  // esi
	simde__m64    v5;  // mm2
	simde__m64    v6;  // mm1
	simde__m64    v7;  // mm3
	simde__m64    v8;  // mm2
	simde__m64    v9;  // mm0
	simde__m64    v10; // mm1

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		do
		{
			v5  = simde_mm_cvtsi32_si64(*v4);
			v6  = simde_mm_cvtsi32_si64(*v3);
			v7  = v5;
			v8  = simde_m_psrldi(v5, 0x19u);
			v9  = simde_m_paddb(simde_m_psubusb(v6, v7), simde_m_psubusb(v7, v6));
			v10 = simde_m_punpcklbw(v6, simde_mm_setzero_si64());
			++v4;
			++v3;
			*(v3 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v10,
						simde_m_psrawi(
							simde_m_pmullw(simde_m_psubw(simde_m_punpcklbw(v9, simde_mm_setzero_si64()), v10), simde_m_punpckldq(simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v8)), v8)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v4 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsDiffBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32   *v4;  // edi
	tjs_uint32   *v5;  // esi
	simde__m64    v6;  // mm4
	simde__m64    v7;  // mm2
	simde__m64    v8;  // mm1
	simde__m64    v9;  // mm0
	simde__m64    v10; // mm2
	simde__m64    v11; // mm1

	if (len > 0)
	{
		v4 = dest;
		v5 = (tjs_uint32   *)src;
		v6 = simde_mm_cvtsi32_si64(opa);
		do
		{
			v7  = simde_mm_cvtsi32_si64(*v5);
			v8  = simde_mm_cvtsi32_si64(*v4);
			v9  = simde_m_paddb(simde_m_psubusb(v8, v7), simde_m_psubusb(v7, v8));
			v10 = simde_m_psrldi(simde_m_pmullw(simde_m_psrldi(v7, 0x19u), v6), 8u);
			v11 = simde_m_punpcklbw(v8, simde_mm_setzero_si64());
			++v5;
			++v4;
			*(v4 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v11,
						simde_m_psrawi(
							simde_m_pmullw(simde_m_psubw(simde_m_punpcklbw(v9, simde_mm_setzero_si64()), v11), simde_m_punpckldq(simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v10)), v10)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v5 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsDiff5Blend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32   *v3;  // edi
	tjs_uint32   *v4;  // esi
	simde__m64    v5;  // mm6
	simde__m64    v6;  // mm0
	simde__m64    v7;  // mm6
	simde__m64    v8;  // mm6
	simde__m64    v9;  // mm1
	simde__m64    v10; // mm0

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		do
		{
			v5  = simde_mm_cvtsi32_si64(*v4);
			v6  = v5;
			v7  = simde_m_psrldi(v5, 0x19u);
			v8  = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v7));
			v9  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v3), simde_mm_setzero_si64());
			v10 = simde_m_psrawi(simde_m_pmullw(simde_m_punpcklbw(v6, simde_mm_setzero_si64()), simde_m_punpckldq(v8, v8)), 7u);
			++v4;
			++v3;
			*(v3 - 1) = simde_mm_cvtsi64_si32(simde_m_packuswb(simde_m_paddw(simde_m_psubusw(v9, v10), simde_m_psubusw(v10, v9)), simde_mm_setzero_si64()));
		} while (v4 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsDiff5Blend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32   *v4;  // edi
	tjs_uint32   *v5;  // esi
	simde__m64    v6;  // mm4
	simde__m64    v7;  // mm6
	simde__m64    v8;  // mm0
	simde__m64    v9;  // mm6
	simde__m64    v10; // mm6
	simde__m64    v11; // mm1
	simde__m64    v12; // mm0

	if (len > 0)
	{
		v4 = dest;
		v5 = (tjs_uint32   *)src;
		v6 = simde_mm_cvtsi32_si64(opa);
		do
		{
			v7  = simde_mm_cvtsi32_si64(*v5);
			v8  = v7;
			v9  = simde_m_psrldi(simde_m_pmullw(simde_m_psrldi(v7, 0x19u), v6), 8u);
			v10 = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v9));
			v11 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v4), simde_mm_setzero_si64());
			v12 = simde_m_psrawi(simde_m_pmullw(simde_m_punpcklbw(v8, simde_mm_setzero_si64()), simde_m_punpckldq(v10, v10)), 7u);
			++v5;
			++v4;
			*(v4 - 1) = simde_mm_cvtsi64_si32(simde_m_packuswb(simde_m_paddw(simde_m_psubusw(v11, v12), simde_m_psubusw(v12, v11)), simde_mm_setzero_si64()));
		} while (v5 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsDiff5Blend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32   *v3; // edi
	tjs_uint32   *v4; // esi
	simde__m64    v5; // mm6
	simde__m64    v6; // mm0
	simde__m64    v7; // mm6
	simde__m64    v8; // mm1
	simde__m64    v9; // mm0

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		do
		{
			v5 = simde_mm_cvtsi32_si64(*v4);
			v6 = v5;
			v7 = simde_m_psrldi(v5, 0x19u);
			v8 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v3), simde_mm_setzero_si64());
			v9 = simde_m_psrawi(simde_m_pmullw(simde_m_punpcklbw(v6, simde_mm_setzero_si64()), simde_m_punpckldq(simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v7)), v7)), 7u);
			++v4;
			++v3;
			*(v3 - 1) = simde_mm_cvtsi64_si32(simde_m_packuswb(simde_m_paddw(simde_m_psubusw(v8, v9), simde_m_psubusw(v9, v8)), simde_mm_setzero_si64()));
		} while (v4 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsDiff5Blend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32   *v4;  // edi
	tjs_uint32   *v5;  // esi
	simde__m64    v6;  // mm4
	simde__m64    v7;  // mm6
	simde__m64    v8;  // mm0
	simde__m64    v9;  // mm6
	simde__m64    v10; // mm1
	simde__m64    v11; // mm0

	if (len > 0)
	{
		v4 = dest;
		v5 = (tjs_uint32   *)src;
		v6 = simde_mm_cvtsi32_si64(opa);
		do
		{
			v7  = simde_mm_cvtsi32_si64(*v5);
			v8  = v7;
			v9  = simde_m_psrldi(simde_m_pmullw(simde_m_psrldi(v7, 0x19u), v6), 8u);
			v10 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v4), simde_mm_setzero_si64());
			v11 = simde_m_psrawi(simde_m_pmullw(simde_m_punpcklbw(v8, simde_mm_setzero_si64()), simde_m_punpckldq(simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v9)), v9)), 7u);
			++v5;
			++v4;
			*(v4 - 1) = simde_mm_cvtsi64_si32(simde_m_packuswb(simde_m_paddw(simde_m_psubusw(v10, v11), simde_m_psubusw(v11, v10)), simde_mm_setzero_si64()));
		} while (v5 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsExclusionBlend_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32   *v3;  // edi
	tjs_uint32   *v4;  // esi
	simde__m64    v5;  // mm2
	simde__m64    v6;  // mm6
	simde__m64    v7;  // mm2
	simde__m64    v8;  // mm1
	simde__m64    v9;  // mm6
	simde__m64    v10; // mm2

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		do
		{
			v5  = simde_mm_cvtsi32_si64(*v4);
			v6  = v5;
			v7  = simde_m_psrldi(v5, 0x19u);
			v8  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v3), simde_mm_setzero_si64());
			v9  = simde_m_punpcklbw(v6, simde_mm_setzero_si64());
			v10 = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v7));
			++v4;
			++v3;
			*(v3 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v8,
						simde_m_psrawi(simde_m_pmullw(simde_m_psubw(v9, simde_m_psrlwi(simde_m_pmullw(v9, v8), 7u)), simde_m_punpckldq(v10, v10)), 7u)),
					simde_mm_setzero_si64()));
		} while (v4 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsExclusionBlend_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32   *v4;  // edi
	tjs_uint32   *v5;  // esi
	simde__m64    v6;  // mm4
	simde__m64    v7;  // mm2
	simde__m64    v8;  // mm1
	simde__m64    v9;  // mm6
	simde__m64    v10; // mm2
	simde__m64    v11; // mm2

	if (len > 0)
	{
		v4 = dest;
		v5 = (tjs_uint32   *)src;
		v6 = simde_mm_cvtsi32_si64(opa);
		do
		{
			v7  = simde_mm_cvtsi32_si64(*v5);
			v8  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v4), simde_mm_setzero_si64());
			v9  = simde_m_punpcklbw(v7, simde_mm_setzero_si64());
			v10 = simde_m_psrldi(simde_m_pmullw(simde_m_psrldi(v7, 0x19u), v6), 8u);
			v11 = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v10));
			++v5;
			++v4;
			*(v4 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v8,
						simde_m_psrawi(simde_m_pmullw(simde_m_psubw(v9, simde_m_psrlwi(simde_m_pmullw(v9, v8), 7u)), simde_m_punpckldq(v11, v11)), 7u)),
					simde_mm_setzero_si64()));
		} while (v5 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsExclusionBlend_HDA_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len))
{
	tjs_uint32   *v3; // edi
	tjs_uint32   *v4; // esi
	simde__m64    v5; // mm2
	simde__m64    v6; // mm6
	simde__m64    v7; // mm2
	simde__m64    v8; // mm1
	simde__m64    v9; // mm6

	if (len > 0)
	{
		v3 = dest;
		v4 = (tjs_uint32   *)src;
		do
		{
			v5 = simde_mm_cvtsi32_si64(*v4);
			v6 = v5;
			v7 = simde_m_psrldi(v5, 0x19u);
			v8 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v3), simde_mm_setzero_si64());
			v9 = simde_m_punpcklbw(v6, simde_mm_setzero_si64());
			++v4;
			++v3;
			*(v3 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v8,
						simde_m_psrawi(
							simde_m_pmullw(
								simde_m_psubw(v9, simde_m_psrlwi(simde_m_pmullw(v9, v8), 7u)),
								simde_m_punpckldq(simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v7)), v7)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v4 < &src[len]);
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPPsExclusionBlend_HDA_o_c, (tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa))
{
	tjs_uint32   *v4;  // edi
	tjs_uint32   *v5;  // esi
	simde__m64    v6;  // mm4
	simde__m64    v7;  // mm2
	simde__m64    v8;  // mm1
	simde__m64    v9;  // mm6
	simde__m64    v10; // mm2

	if (len > 0)
	{
		v4 = dest;
		v5 = (tjs_uint32   *)src;
		v6 = simde_mm_cvtsi32_si64(opa);
		do
		{
			v7  = simde_mm_cvtsi32_si64(*v5);
			v8  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v4), simde_mm_setzero_si64());
			v9  = simde_m_punpcklbw(v7, simde_mm_setzero_si64());
			v10 = simde_m_psrldi(simde_m_pmullw(simde_m_psrldi(v7, 0x19u), v6), 8u);
			++v5;
			++v4;
			*(v4 - 1) = simde_mm_cvtsi64_si32(
				simde_m_packuswb(
					simde_m_paddw(
						v8,
						simde_m_psrawi(
							simde_m_pmullw(
								simde_m_psubw(v9, simde_m_psrlwi(simde_m_pmullw(v9, v8), 7u)),
								simde_m_punpckldq(simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v10)), v10)),
							7u)),
					simde_mm_setzero_si64()));
		} while (v5 < &src[len]);
	}
	simde_m_empty();
}
