#ifndef CSSO_ROCKETRENDER_H
#define CSSO_ROCKETRENDER_H

#include <RmlUi/Core/RenderInterface.h>

// GLES3 renderer for RmlUi. Shader based, one dynamic VBO/IBO, own VAO.
// BeginFrame()/EndFrame() save and restore every piece of GL state we touch, so the engine's
// (ToGLES) state cache stays valid without calling into it.
class RocketRender : public Rml::RenderInterface
{
public:
	static RocketRender m_Instance;
	RocketRender();

	void SetScreenSize( int width, int height ) { m_width = width; m_height = height; }
	void BeginFrame();
	void EndFrame();
	void Shutdown();

	void RenderGeometry( Rml::Vertex *vertices, int num_vertices, int *indices, int num_indices,
						 Rml::TextureHandle texture, const Rml::Vector2f &translation ) override;
	void EnableScissorRegion( bool enable ) override;
	void SetScissorRegion( int x, int y, int width, int height ) override;
	bool LoadTexture( Rml::TextureHandle &texture_handle, Rml::Vector2i &texture_dimensions, const Rml::String &source ) override;
	bool GenerateTexture( Rml::TextureHandle &texture_handle, const Rml::byte *source, const Rml::Vector2i &source_dimensions ) override;
	void ReleaseTexture( Rml::TextureHandle texture ) override;
	void SetTransform( const Rml::Matrix4f *transform ) override;

private:
	bool InitGL();

	int  m_width, m_height;
	bool m_glReady;

	unsigned int m_program, m_vao, m_vbo, m_ibo;
	int  m_locProj, m_locTransform, m_locOffset, m_locTex, m_locHasTex;
	float m_transform[16];

	// Saved engine state
	struct SavedState;
	SavedState *m_saved;
};

#endif
