#include "rocketrender.h"

#include <GLES3/gl3.h>

#include <RmlUi/Core.h>
#include <string.h>
#include <type_traits>

RocketRender RocketRender::m_Instance;

struct RocketRender::SavedState
{
	GLint  program, vao, arrayBuffer, activeTexture, texture0;
	GLint  viewport[4], scissorBox[4];
	GLint  blendSrcRGB, blendDstRGB, blendSrcA, blendDstA, blendEqRGB, blendEqA;
	GLint  colorMask[4];
	GLboolean depthMask;
	GLboolean blend, cull, depthTest, stencilTest, scissor, dither;
};

static const char *s_vs =
	"#version 300 es\n"
	"layout(location=0) in vec2 a_pos;\n"
	"layout(location=1) in vec4 a_col;\n"
	"layout(location=2) in vec2 a_uv;\n"
	"uniform mat4 u_proj;\n"
	"uniform mat4 u_tr;\n"
	"uniform vec2 u_off;\n"
	"out vec4 v_col;\n"
	"out vec2 v_uv;\n"
	"void main() {\n"
	"  v_col = a_col;\n"
	"  v_uv = a_uv;\n"
	"  gl_Position = u_proj * u_tr * vec4(a_pos + u_off, 0.0, 1.0);\n"
	"}\n";

static const char *s_fs =
	"#version 300 es\n"
	"precision mediump float;\n"
	"uniform sampler2D u_tex;\n"
	"uniform float u_hastex;\n"
	"in vec4 v_col;\n"
	"in vec2 v_uv;\n"
	"out vec4 o_col;\n"
	"void main() {\n"
	"  vec4 c = v_col;\n"
	"  if (u_hastex > 0.5) c *= texture(u_tex, v_uv);\n"
	"  o_col = c;\n"
	"}\n";

static void SetCap( GLenum cap, GLboolean on )
{
	if ( on )
		glEnable( cap );
	else
		glDisable( cap );
}

static GLuint CompileShader( GLenum type, const char *src )
{
	GLuint s = glCreateShader( type );
	glShaderSource( s, 1, &src, NULL );
	glCompileShader( s );
	GLint ok = 0;
	glGetShaderiv( s, GL_COMPILE_STATUS, &ok );
	if ( !ok )
	{
		char log[512];
		glGetShaderInfoLog( s, sizeof( log ), NULL, log );
		Rml::Log::Message( Rml::Log::LT_ERROR, "RocketUI shader compile failed: %s", log );
		glDeleteShader( s );
		return 0;
	}
	return s;
}

RocketRender::RocketRender() :
	m_width( 1280 ), m_height( 720 ), m_glReady( false ),
	m_program( 0 ), m_vao( 0 ), m_vbo( 0 ), m_ibo( 0 ),
	m_locProj( -1 ), m_locTransform( -1 ), m_locOffset( -1 ), m_locTex( -1 ), m_locHasTex( -1 ),
	m_saved( NULL )
{
	static const float identity[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
	memcpy( m_transform, identity, sizeof( m_transform ) );
}

bool RocketRender::InitGL()
{
	GLuint vs = CompileShader( GL_VERTEX_SHADER, s_vs );
	GLuint fs = CompileShader( GL_FRAGMENT_SHADER, s_fs );
	if ( !vs || !fs )
		return false;

	m_program = glCreateProgram();
	glAttachShader( m_program, vs );
	glAttachShader( m_program, fs );
	glLinkProgram( m_program );
	glDeleteShader( vs );
	glDeleteShader( fs );

	GLint ok = 0;
	glGetProgramiv( m_program, GL_LINK_STATUS, &ok );
	if ( !ok )
	{
		Rml::Log::Message( Rml::Log::LT_ERROR, "RocketUI shader link failed" );
		glDeleteProgram( m_program );
		m_program = 0;
		return false;
	}

	m_locProj = glGetUniformLocation( m_program, "u_proj" );
	m_locTransform = glGetUniformLocation( m_program, "u_tr" );
	m_locOffset = glGetUniformLocation( m_program, "u_off" );
	m_locTex = glGetUniformLocation( m_program, "u_tex" );
	m_locHasTex = glGetUniformLocation( m_program, "u_hastex" );

	glGenVertexArrays( 1, &m_vao );
	glGenBuffers( 1, &m_vbo );
	glGenBuffers( 1, &m_ibo );

	GLint prevVao = 0;
	glGetIntegerv( GL_VERTEX_ARRAY_BINDING, &prevVao );
	glBindVertexArray( m_vao );
	glBindBuffer( GL_ARRAY_BUFFER, m_vbo );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_ibo );
	glEnableVertexAttribArray( 0 );
	glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, sizeof( Rml::Vertex ), (const void *)offsetof( Rml::Vertex, position ) );
	glEnableVertexAttribArray( 1 );
	glVertexAttribPointer( 1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( Rml::Vertex ), (const void *)offsetof( Rml::Vertex, colour ) );
	glEnableVertexAttribArray( 2 );
	glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, sizeof( Rml::Vertex ), (const void *)offsetof( Rml::Vertex, tex_coord ) );
	glBindVertexArray( prevVao );

	return true;
}

void RocketRender::BeginFrame()
{
	if ( !m_saved )
		m_saved = new SavedState;
	SavedState &s = *m_saved;

	glGetIntegerv( GL_CURRENT_PROGRAM, &s.program );
	glGetIntegerv( GL_VERTEX_ARRAY_BINDING, &s.vao );
	glGetIntegerv( GL_ARRAY_BUFFER_BINDING, &s.arrayBuffer );
	glGetIntegerv( GL_ACTIVE_TEXTURE, &s.activeTexture );
	glActiveTexture( GL_TEXTURE0 );
	glGetIntegerv( GL_TEXTURE_BINDING_2D, &s.texture0 );
	glGetIntegerv( GL_VIEWPORT, s.viewport );
	glGetIntegerv( GL_SCISSOR_BOX, s.scissorBox );
	glGetIntegerv( GL_BLEND_SRC_RGB, &s.blendSrcRGB );
	glGetIntegerv( GL_BLEND_DST_RGB, &s.blendDstRGB );
	glGetIntegerv( GL_BLEND_SRC_ALPHA, &s.blendSrcA );
	glGetIntegerv( GL_BLEND_DST_ALPHA, &s.blendDstA );
	glGetIntegerv( GL_BLEND_EQUATION_RGB, &s.blendEqRGB );
	glGetIntegerv( GL_BLEND_EQUATION_ALPHA, &s.blendEqA );
	glGetIntegerv( GL_COLOR_WRITEMASK, s.colorMask );
	glGetBooleanv( GL_DEPTH_WRITEMASK, &s.depthMask );
	s.blend = glIsEnabled( GL_BLEND );
	s.cull = glIsEnabled( GL_CULL_FACE );
	s.depthTest = glIsEnabled( GL_DEPTH_TEST );
	s.stencilTest = glIsEnabled( GL_STENCIL_TEST );
	s.scissor = glIsEnabled( GL_SCISSOR_TEST );

	if ( !m_glReady )
		m_glReady = InitGL();
	if ( !m_glReady )
		return;

	glViewport( 0, 0, m_width, m_height );
	glDisable( GL_CULL_FACE );
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_STENCIL_TEST );
	glDisable( GL_SCISSOR_TEST );
	glDepthMask( GL_FALSE );
	glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
	glEnable( GL_BLEND );
	glBlendEquation( GL_FUNC_ADD );
	glBlendFuncSeparate( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA );

	glUseProgram( m_program );
	glBindVertexArray( m_vao );

	// RmlUi: origin top-left, y down.
	const float w = (float)m_width, h = (float)m_height;
	const float proj[16] = {
		2.0f / w, 0, 0, 0,
		0, -2.0f / h, 0, 0,
		0, 0, -1, 0,
		-1, 1, 0, 1
	};
	glUniformMatrix4fv( m_locProj, 1, GL_FALSE, proj );
	glUniform1i( m_locTex, 0 );
	SetTransform( NULL );
}

void RocketRender::EndFrame()
{
	if ( !m_saved )
		return;
	SavedState &s = *m_saved;

	glBindVertexArray( s.vao );
	glUseProgram( s.program );
	glBindBuffer( GL_ARRAY_BUFFER, s.arrayBuffer );
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, s.texture0 );
	glActiveTexture( s.activeTexture );

	glViewport( s.viewport[0], s.viewport[1], s.viewport[2], s.viewport[3] );
	glScissor( s.scissorBox[0], s.scissorBox[1], s.scissorBox[2], s.scissorBox[3] );
	glBlendEquationSeparate( s.blendEqRGB, s.blendEqA );
	glBlendFuncSeparate( s.blendSrcRGB, s.blendDstRGB, s.blendSrcA, s.blendDstA );
	glColorMask( s.colorMask[0], s.colorMask[1], s.colorMask[2], s.colorMask[3] );
	glDepthMask( s.depthMask );

	SetCap( GL_BLEND, s.blend );
	SetCap( GL_CULL_FACE, s.cull );
	SetCap( GL_DEPTH_TEST, s.depthTest );
	SetCap( GL_STENCIL_TEST, s.stencilTest );
	SetCap( GL_SCISSOR_TEST, s.scissor );
}

void RocketRender::Shutdown()
{
	if ( m_glReady )
	{
		glDeleteBuffers( 1, &m_vbo );
		glDeleteBuffers( 1, &m_ibo );
		glDeleteVertexArrays( 1, &m_vao );
		glDeleteProgram( m_program );
		m_glReady = false;
	}
	delete m_saved;
	m_saved = NULL;
}

void RocketRender::RenderGeometry( Rml::Vertex *vertices, int num_vertices, int *indices, int num_indices,
								   Rml::TextureHandle texture, const Rml::Vector2f &translation )
{
	if ( !m_glReady )
		return;

	glUniform2f( m_locOffset, translation.x, translation.y );

	if ( texture )
	{
		glUniform1f( m_locHasTex, 1.0f );
		glBindTexture( GL_TEXTURE_2D, (GLuint)texture );
	}
	else
	{
		glUniform1f( m_locHasTex, 0.0f );
	}

	glBindBuffer( GL_ARRAY_BUFFER, m_vbo );
	glBufferData( GL_ARRAY_BUFFER, num_vertices * sizeof( Rml::Vertex ), vertices, GL_STREAM_DRAW );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_ibo );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, num_indices * sizeof( int ), indices, GL_STREAM_DRAW );
	glDrawElements( GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, 0 );
}

void RocketRender::EnableScissorRegion( bool enable )
{
	if ( enable )
		glEnable( GL_SCISSOR_TEST );
	else
		glDisable( GL_SCISSOR_TEST );
}

void RocketRender::SetScissorRegion( int x, int y, int width, int height )
{
	// RmlUi is top-left origin, GL scissor is bottom-left.
	glScissor( x, m_height - ( y + height ), width, height );
}

#pragma pack(push, 1)
struct TGAHeader
{
	char idLength;
	char colourMapType;
	char dataType;
	short colourMapOrigin;
	short colourMapLength;
	char colourMapDepth;
	short xOrigin;
	short yOrigin;
	short width;
	short height;
	char bitsPerPixel;
	char imageDescriptor;
};
#pragma pack(pop)

bool RocketRender::LoadTexture( Rml::TextureHandle &texture_handle, Rml::Vector2i &texture_dimensions, const Rml::String &source )
{
	Rml::FileInterface *fi = Rml::GetFileInterface();
	Rml::FileHandle fh = fi->Open( source );
	if ( !fh )
		return false;

	fi->Seek( fh, 0, SEEK_END );
	size_t size = fi->Tell( fh );
	fi->Seek( fh, 0, SEEK_SET );

	if ( size <= sizeof( TGAHeader ) )
	{
		fi->Close( fh );
		Rml::Log::Message( Rml::Log::LT_ERROR, "Texture %s is too small to be a TGA", source.c_str() );
		return false;
	}

	char *buffer = new char[size];
	fi->Read( buffer, size, fh );
	fi->Close( fh );

	TGAHeader header;
	memcpy( &header, buffer, sizeof( TGAHeader ) );

	const int colorMode = header.bitsPerPixel / 8;
	if ( header.dataType != 2 || colorMode < 3 )
	{
		Rml::Log::Message( Rml::Log::LT_ERROR, "Texture %s: only 24/32 bit uncompressed TGAs are supported", source.c_str() );
		delete[] buffer;
		return false;
	}

	const size_t imageSize = (size_t)header.width * header.height * 4;
	const size_t dataOffset = sizeof( TGAHeader ) + header.idLength;
	if ( size < dataOffset + (size_t)header.width * header.height * colorMode )
	{
		Rml::Log::Message( Rml::Log::LT_ERROR, "Texture %s is truncated", source.c_str() );
		delete[] buffer;
		return false;
	}

	const unsigned char *src = (const unsigned char *)buffer + dataOffset;
	unsigned char *dest = new unsigned char[imageSize];

	// TGA is BGR(A); convert to RGBA and flip to top-down unless the file already is.
	const bool topDown = ( header.imageDescriptor & 32 ) != 0;
	for ( int y = 0; y < header.height; y++ )
	{
		const unsigned char *row = src + (size_t)y * header.width * colorMode;
		unsigned char *out = dest + (size_t)( topDown ? y : header.height - y - 1 ) * header.width * 4;
		for ( int x = 0; x < header.width; x++ )
		{
			out[0] = row[2];
			out[1] = row[1];
			out[2] = row[0];
			out[3] = ( colorMode == 4 ) ? row[3] : 255;
			out += 4;
			row += colorMode;
		}
	}

	texture_dimensions.x = header.width;
	texture_dimensions.y = header.height;
	bool ok = GenerateTexture( texture_handle, dest, texture_dimensions );

	delete[] dest;
	delete[] buffer;
	return ok;
}

bool RocketRender::GenerateTexture( Rml::TextureHandle &texture_handle, const Rml::byte *source, const Rml::Vector2i &source_dimensions )
{
	GLint prevActive = 0, prevTex = 0;
	glGetIntegerv( GL_ACTIVE_TEXTURE, &prevActive );
	glActiveTexture( GL_TEXTURE0 );
	glGetIntegerv( GL_TEXTURE_BINDING_2D, &prevTex );

	GLuint id = 0;
	glGenTextures( 1, &id );
	if ( !id )
	{
		glActiveTexture( prevActive );
		return false;
	}

	glBindTexture( GL_TEXTURE_2D, id );
	glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, source_dimensions.x, source_dimensions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, source );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	glBindTexture( GL_TEXTURE_2D, prevTex );
	glActiveTexture( prevActive );

	texture_handle = (Rml::TextureHandle)id;
	return true;
}

void RocketRender::ReleaseTexture( Rml::TextureHandle texture )
{
	GLuint id = (GLuint)texture;
	glDeleteTextures( 1, &id );
}

void RocketRender::SetTransform( const Rml::Matrix4f *transform )
{
	if ( !m_glReady )
		return;

	if ( !transform )
	{
		static const float identity[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
		memcpy( m_transform, identity, sizeof( m_transform ) );
	}
	else if ( std::is_same<Rml::Matrix4f, Rml::ColumnMajorMatrix4f>::value )
	{
		memcpy( m_transform, transform->data(), sizeof( m_transform ) );
	}
	else
	{
		memcpy( m_transform, transform->Transpose().data(), sizeof( m_transform ) );
	}
	glUniformMatrix4fv( m_locTransform, 1, GL_FALSE, m_transform );
}
