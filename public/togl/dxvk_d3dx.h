//========= D3DX shim for the DXVK native render mechanism ===================//
//
// DXVK does not ship D3DX. shaderapidx9 only needs a small subset (math types
// and helpers, plus a few shader helpers), reimplemented here header-only.
// Conventions follow D3DX: row-major matrices, row vectors (v * M).
//
// Shader *compilation* is not available: the engine ships precompiled
// shaders (shaderlib), D3DXCompileShader is only used for tiny built-in
// fallback shaders and fails cleanly with E_NOTIMPL here.
//
//==========================================================================//
#ifndef TOGL_DXVK_D3DX_H
#define TOGL_DXVK_D3DX_H

#include <math.h>
#include <string.h>

#ifndef D3DXINLINE
#define D3DXINLINE inline
#endif

#define D3DXSHADER_DEBUG                    (1 << 0)
#define D3DXSHADER_AVOID_FLOW_CONTROL       (1 << 9)

// ------------------------------------------------------------------ types --

class D3DXMATRIX : public D3DMATRIX
{
public:
	D3DXMATRIX() {}

	D3DXMATRIX operator*( const D3DXMATRIX &o ) const
	{
		D3DXMATRIX r;
		for ( int i = 0; i < 4; i++ )
			for ( int j = 0; j < 4; j++ )
				r.m[i][j] = m[i][0]*o.m[0][j] + m[i][1]*o.m[1][j] + m[i][2]*o.m[2][j] + m[i][3]*o.m[3][j];
		return r;
	}
	operator FLOAT* ()				{ return &m[0][0]; }
	operator CONST FLOAT* () const	{ return &m[0][0]; }
	float& operator()( int row, int column )				{ return m[row][column]; }
	const float& operator()( int row, int column ) const	{ return m[row][column]; }
	bool operator != ( CONST D3DXMATRIX& src ) const		{ return memcmp( this, &src, sizeof( D3DXMATRIX ) ) != 0; }
	bool operator == ( CONST D3DXMATRIX& src ) const		{ return memcmp( this, &src, sizeof( D3DXMATRIX ) ) == 0; }
};

typedef struct D3DXPLANE
{
	float& operator[]( int i )			{ return (&a)[i]; }
	bool operator==( const D3DXPLANE &o ) { return a == o.a && b == o.b && c == o.c && d == o.d; }
	bool operator!=( const D3DXPLANE &o ) { return !( *this == o ); }
	operator float*()					{ return &a; }
	operator const float*() const		{ return &a; }

	float a, b, c, d;
} D3DXPLANE;

class D3DXVECTOR2
{
public:
	operator FLOAT* ()				{ return &x; }
	operator CONST FLOAT* () const	{ return &x; }

	float x, y;
};

class D3DXVECTOR3 : public D3DVECTOR
{
public:
	D3DXVECTOR3() {}
	D3DXVECTOR3( float a, float b, float c ) { x = a; y = b; z = c; }
	operator FLOAT* ()				{ return &x; }
	operator CONST FLOAT* () const	{ return &x; }
};

class D3DXVECTOR4
{
public:
	D3DXVECTOR4() {}
	D3DXVECTOR4( float a, float b, float c, float d ) : x( a ), y( b ), z( c ), w( d ) {}
	operator FLOAT* ()				{ return &x; }
	operator CONST FLOAT* () const	{ return &x; }

	float x, y, z, w;
};

typedef enum _D3DXINCLUDE_TYPE
{
	D3DXINC_LOCAL,
	D3DXINC_FORCE_DWORD = 0x7fffffff
} D3DXINCLUDE_TYPE;

typedef struct _D3DXMACRO
{
	LPCSTR Name;
	LPCSTR Definition;
} D3DXMACRO, *LPD3DXMACRO;

struct ID3DXInclude
{
	virtual HRESULT Open( D3DXINCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes ) { return E_NOTIMPL; }
	virtual HRESULT Close( LPCVOID pData ) { return E_NOTIMPL; }
};
typedef ID3DXInclude* LPD3DXINCLUDE;

struct ID3DXBuffer : public IUnknown
{
	virtual void* STDMETHODCALLTYPE GetBufferPointer() = 0;
	virtual DWORD STDMETHODCALLTYPE GetBufferSize() = 0;
};
typedef ID3DXBuffer* LPD3DXBUFFER;

class ID3DXConstantTable : public IUnknown {};
typedef ID3DXConstantTable* LPD3DXCONSTANTTABLE;

// -------------------------------------------------------------- matrices --

D3DXINLINE D3DXMATRIX* D3DXMatrixIdentity( D3DXMATRIX *pOut )
{
	memset( static_cast<D3DMATRIX *>( pOut ), 0, sizeof( D3DMATRIX ) );
	pOut->m[0][0] = pOut->m[1][1] = pOut->m[2][2] = pOut->m[3][3] = 1.0f;
	return pOut;
}

D3DXINLINE D3DXMATRIX* D3DXMatrixMultiply( D3DXMATRIX *pOut, CONST D3DXMATRIX *pM1, CONST D3DXMATRIX *pM2 )
{
	D3DXMATRIX r = (*pM1) * (*pM2); // temp: pOut may alias an input
	*pOut = r;
	return pOut;
}

D3DXINLINE D3DXMATRIX* D3DXMatrixTranspose( D3DXMATRIX *pOut, CONST D3DXMATRIX *pM )
{
	D3DXMATRIX r;
	for ( int i = 0; i < 4; i++ )
		for ( int j = 0; j < 4; j++ )
			r.m[i][j] = pM->m[j][i];
	*pOut = r;
	return pOut;
}

D3DXINLINE D3DXMATRIX* D3DXMatrixInverse( D3DXMATRIX *pOut, FLOAT *pDeterminant, CONST D3DXMATRIX *pM )
{
	const float *a = &pM->m[0][0];
	float inv[16];

	inv[0]  =  a[5]*a[10]*a[15] - a[5]*a[11]*a[14] - a[9]*a[6]*a[15] + a[9]*a[7]*a[14] + a[13]*a[6]*a[11] - a[13]*a[7]*a[10];
	inv[4]  = -a[4]*a[10]*a[15] + a[4]*a[11]*a[14] + a[8]*a[6]*a[15] - a[8]*a[7]*a[14] - a[12]*a[6]*a[11] + a[12]*a[7]*a[10];
	inv[8]  =  a[4]*a[9]*a[15]  - a[4]*a[11]*a[13] - a[8]*a[5]*a[15] + a[8]*a[7]*a[13] + a[12]*a[5]*a[11] - a[12]*a[7]*a[9];
	inv[12] = -a[4]*a[9]*a[14]  + a[4]*a[10]*a[13] + a[8]*a[5]*a[14] - a[8]*a[6]*a[13] - a[12]*a[5]*a[10] + a[12]*a[6]*a[9];
	inv[1]  = -a[1]*a[10]*a[15] + a[1]*a[11]*a[14] + a[9]*a[2]*a[15] - a[9]*a[3]*a[14] - a[13]*a[2]*a[11] + a[13]*a[3]*a[10];
	inv[5]  =  a[0]*a[10]*a[15] - a[0]*a[11]*a[14] - a[8]*a[2]*a[15] + a[8]*a[3]*a[14] + a[12]*a[2]*a[11] - a[12]*a[3]*a[10];
	inv[9]  = -a[0]*a[9]*a[15]  + a[0]*a[11]*a[13] + a[8]*a[1]*a[15] - a[8]*a[3]*a[13] - a[12]*a[1]*a[11] + a[12]*a[3]*a[9];
	inv[13] =  a[0]*a[9]*a[14]  - a[0]*a[10]*a[13] - a[8]*a[1]*a[14] + a[8]*a[2]*a[13] + a[12]*a[1]*a[10] - a[12]*a[2]*a[9];
	inv[2]  =  a[1]*a[6]*a[15]  - a[1]*a[7]*a[14]  - a[5]*a[2]*a[15] + a[5]*a[3]*a[14] + a[13]*a[2]*a[7]  - a[13]*a[3]*a[6];
	inv[6]  = -a[0]*a[6]*a[15]  + a[0]*a[7]*a[14]  + a[4]*a[2]*a[15] - a[4]*a[3]*a[14] - a[12]*a[2]*a[7]  + a[12]*a[3]*a[6];
	inv[10] =  a[0]*a[5]*a[15]  - a[0]*a[7]*a[13]  - a[4]*a[1]*a[15] + a[4]*a[3]*a[13] + a[12]*a[1]*a[7]  - a[12]*a[3]*a[5];
	inv[14] = -a[0]*a[5]*a[14]  + a[0]*a[6]*a[13]  + a[4]*a[1]*a[14] - a[4]*a[2]*a[13] - a[12]*a[1]*a[6]  + a[12]*a[2]*a[5];
	inv[3]  = -a[1]*a[6]*a[11]  + a[1]*a[7]*a[10]  + a[5]*a[2]*a[11] - a[5]*a[3]*a[10] - a[9]*a[2]*a[7]   + a[9]*a[3]*a[6];
	inv[7]  =  a[0]*a[6]*a[11]  - a[0]*a[7]*a[10]  - a[4]*a[2]*a[11] + a[4]*a[3]*a[10] + a[8]*a[2]*a[7]   - a[8]*a[3]*a[6];
	inv[11] = -a[0]*a[5]*a[11]  + a[0]*a[7]*a[9]   + a[4]*a[1]*a[11] - a[4]*a[3]*a[9]  - a[8]*a[1]*a[7]   + a[8]*a[3]*a[5];
	inv[15] =  a[0]*a[5]*a[10]  - a[0]*a[6]*a[9]   - a[4]*a[1]*a[10] + a[4]*a[2]*a[9]  + a[8]*a[1]*a[6]   - a[8]*a[2]*a[5];

	float det = a[0]*inv[0] + a[1]*inv[4] + a[2]*inv[8] + a[3]*inv[12];
	if ( pDeterminant )
		*pDeterminant = det;
	if ( det == 0.0f )
		return NULL;

	float rdet = 1.0f / det;
	float *o = &pOut->m[0][0];
	for ( int i = 0; i < 16; i++ )
		o[i] = inv[i] * rdet;
	return pOut;
}

D3DXINLINE D3DXMATRIX* D3DXMatrixTranslation( D3DXMATRIX *pOut, FLOAT x, FLOAT y, FLOAT z )
{
	D3DXMatrixIdentity( pOut );
	pOut->m[3][0] = x; pOut->m[3][1] = y; pOut->m[3][2] = z;
	return pOut;
}

D3DXINLINE D3DXMATRIX* D3DXMatrixOrthoOffCenterRH( D3DXMATRIX *pOut, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn, FLOAT zf )
{
	D3DXMatrixIdentity( pOut );
	pOut->m[0][0] = 2.0f / ( r - l );
	pOut->m[1][1] = 2.0f / ( t - b );
	pOut->m[2][2] = 1.0f / ( zn - zf );
	pOut->m[3][0] = ( l + r ) / ( l - r );
	pOut->m[3][1] = ( t + b ) / ( b - t );
	pOut->m[3][2] = zn / ( zn - zf );
	return pOut;
}

D3DXINLINE D3DXMATRIX* D3DXMatrixPerspectiveRH( D3DXMATRIX *pOut, FLOAT w, FLOAT h, FLOAT zn, FLOAT zf )
{
	memset( static_cast<D3DMATRIX *>( pOut ), 0, sizeof( D3DMATRIX ) );
	pOut->m[0][0] = 2.0f * zn / w;
	pOut->m[1][1] = 2.0f * zn / h;
	pOut->m[2][2] = zf / ( zn - zf );
	pOut->m[2][3] = -1.0f;
	pOut->m[3][2] = zn * zf / ( zn - zf );
	return pOut;
}

D3DXINLINE D3DXMATRIX* D3DXMatrixPerspectiveOffCenterRH( D3DXMATRIX *pOut, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn, FLOAT zf )
{
	memset( static_cast<D3DMATRIX *>( pOut ), 0, sizeof( D3DMATRIX ) );
	pOut->m[0][0] = 2.0f * zn / ( r - l );
	pOut->m[1][1] = 2.0f * zn / ( t - b );
	pOut->m[2][0] = ( l + r ) / ( r - l );
	pOut->m[2][1] = ( t + b ) / ( t - b );
	pOut->m[2][2] = zf / ( zn - zf );
	pOut->m[2][3] = -1.0f;
	pOut->m[3][2] = zn * zf / ( zn - zf );
	return pOut;
}

// --------------------------------------------------------------- vectors --

D3DXINLINE D3DXVECTOR3* D3DXVec3Subtract( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2 )
{
	pOut->x = pV1->x - pV2->x; pOut->y = pV1->y - pV2->y; pOut->z = pV1->z - pV2->z;
	return pOut;
}

D3DXINLINE D3DXVECTOR3* D3DXVec3Cross( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2 )
{
	D3DXVECTOR3 v( pV1->y*pV2->z - pV1->z*pV2->y, pV1->z*pV2->x - pV1->x*pV2->z, pV1->x*pV2->y - pV1->y*pV2->x );
	*pOut = v;
	return pOut;
}

D3DXINLINE FLOAT D3DXVec3Dot( CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2 )
{
	return pV1->x*pV2->x + pV1->y*pV2->y + pV1->z*pV2->z;
}

D3DXINLINE D3DXVECTOR3* D3DXVec3TransformCoord( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV, CONST D3DXMATRIX *pM )
{
	const float x = pV->x, y = pV->y, z = pV->z;
	float w = x*pM->m[0][3] + y*pM->m[1][3] + z*pM->m[2][3] + pM->m[3][3];
	float iw = ( w != 0.0f ) ? 1.0f / w : 0.0f;
	pOut->x = ( x*pM->m[0][0] + y*pM->m[1][0] + z*pM->m[2][0] + pM->m[3][0] ) * iw;
	pOut->y = ( x*pM->m[0][1] + y*pM->m[1][1] + z*pM->m[2][1] + pM->m[3][1] ) * iw;
	pOut->z = ( x*pM->m[0][2] + y*pM->m[1][2] + z*pM->m[2][2] + pM->m[3][2] ) * iw;
	return pOut;
}

D3DXINLINE D3DXVECTOR4* D3DXVec4Transform( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV, CONST D3DXMATRIX *pM )
{
	D3DXVECTOR4 v( pV->x, pV->y, pV->z, pV->w );
	pOut->x = v.x*pM->m[0][0] + v.y*pM->m[1][0] + v.z*pM->m[2][0] + v.w*pM->m[3][0];
	pOut->y = v.x*pM->m[0][1] + v.y*pM->m[1][1] + v.z*pM->m[2][1] + v.w*pM->m[3][1];
	pOut->z = v.x*pM->m[0][2] + v.y*pM->m[1][2] + v.z*pM->m[2][2] + v.w*pM->m[3][2];
	pOut->w = v.x*pM->m[0][3] + v.y*pM->m[1][3] + v.z*pM->m[2][3] + v.w*pM->m[3][3];
	return pOut;
}

D3DXINLINE D3DXVECTOR4* D3DXVec4Normalize( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV )
{
	float l = sqrtf( pV->x*pV->x + pV->y*pV->y + pV->z*pV->z + pV->w*pV->w );
	float il = ( l > 0.0f ) ? 1.0f / l : 0.0f;
	pOut->x = pV->x*il; pOut->y = pV->y*il; pOut->z = pV->z*il; pOut->w = pV->w*il;
	return pOut;
}

// ---------------------------------------------------------------- planes --

D3DXINLINE D3DXPLANE* D3DXPlaneNormalize( D3DXPLANE *pOut, CONST D3DXPLANE *pP )
{
	float l = sqrtf( pP->a*pP->a + pP->b*pP->b + pP->c*pP->c );
	if ( l > 0.0f )
	{
		float il = 1.0f / l;
		pOut->a = pP->a*il; pOut->b = pP->b*il; pOut->c = pP->c*il; pOut->d = pP->d*il;
	}
	else
	{
		pOut->a = pOut->b = pOut->c = pOut->d = 0.0f;
	}
	return pOut;
}

// The matrix is the inverse transpose of the actual transformation (D3DX convention).
D3DXINLINE D3DXPLANE* D3DXPlaneTransform( D3DXPLANE *pOut, CONST D3DXPLANE *pP, CONST D3DXMATRIX *pM )
{
	D3DXPLANE p = *pP;
	pOut->a = p.a*pM->m[0][0] + p.b*pM->m[1][0] + p.c*pM->m[2][0] + p.d*pM->m[3][0];
	pOut->b = p.a*pM->m[0][1] + p.b*pM->m[1][1] + p.c*pM->m[2][1] + p.d*pM->m[3][1];
	pOut->c = p.a*pM->m[0][2] + p.b*pM->m[1][2] + p.c*pM->m[2][2] + p.d*pM->m[3][2];
	pOut->d = p.a*pM->m[0][3] + p.b*pM->m[1][3] + p.c*pM->m[2][3] + p.d*pM->m[3][3];
	return pOut;
}

// --------------------------------------------------------------- shaders --

D3DXINLINE const char* D3DXGetPixelShaderProfile( IDirect3DDevice9 * ) { return "ps_3_0"; }

// The first DWORD of a D3D9 shader token stream is the version token.
D3DXINLINE DWORD D3DXGetShaderVersion( CONST DWORD *pFunction ) { return pFunction ? *pFunction : 0; }

D3DXINLINE HRESULT D3DXDisassembleShader( CONST DWORD *, BOOL, LPCSTR, LPD3DXBUFFER *ppDisassembly )
{
	if ( ppDisassembly ) *ppDisassembly = NULL;
	return E_NOTIMPL;
}

D3DXINLINE HRESULT D3DXCompileShader( LPCSTR, UINT, CONST D3DXMACRO *, LPD3DXINCLUDE, LPCSTR, LPCSTR, DWORD,
									  LPD3DXBUFFER *ppShader, LPD3DXBUFFER *ppErrorMsgs, LPD3DXCONSTANTTABLE * )
{
	if ( ppShader ) *ppShader = NULL;
	if ( ppErrorMsgs ) *ppErrorMsgs = NULL;
	return E_NOTIMPL;
}

#endif // TOGL_DXVK_D3DX_H
