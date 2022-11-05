#pragma once

#include <gxm.h>
#include "SPK_VitaRenderer.h"
#include "Extensions/Renderers/SPK_QuadRendererInterface.h"
#include "Extensions/Renderers/SPK_Oriented3DRendererInterface.h"
#include "Asset/AssetTexture.h"


namespace SPK
{
namespace Vita
{
	class VitaQuadRenderer : public VitaRenderer, public QuadRendererInterface, public Oriented3DRendererInterface
	{
	public:
		SPK_IMPLEMENT_REGISTERABLE(VitaQuadRenderer)

		static void classInit(uint32_t max_particle_count);
		static void classShutdown();

		//////////////////
		// Constructors //
		//////////////////

		VitaQuadRenderer(float scaleX = 1.0f, float scaleY = 1.0f);

		static VitaQuadRenderer* create(float scaleX = 1.0f, float scaleY = 1.0f);

		/////////////
		// Setters //
		/////////////

		virtual bool setTexturingMode(TexturingMode mode);
		void setTexture(AssetTexturePtr texture);

		///////////////
		// Interface //
		///////////////

		virtual void createBuffers(const Group& group);
		virtual void destroyBuffers(const Group& group);

		virtual void render(const Group& group);

	protected:

		virtual bool checkBuffers(const Group& group);

	private :

		static int		s_initCount;
		static void*	s_sharedIndexBuffer;
		static uint32_t s_maxParticleCount;

		AssetTexturePtr m_pTexture;

		// vertex buffers and iterators
		static VertexFormat* vertexBuffer;
		static VertexFormat* vertexIterator;

		// buffers names
		static const std::string VERTEX_BUFFER_NAME;

		void VitaCallColorAndVertex(VertexFormat* vertex_it, const Particle& particle) const;	// Vita calls for color and position
		void VitaCallTexture2DAtlas(VertexFormat* vertex_it, const Particle& particle) const;	// Vita calls for 2D atlastexturing 

		static void (VitaQuadRenderer::*renderParticle)(VertexFormat*&, const Particle&)  const;	// pointer to the right render method

		void render2D(VertexFormat*& vertex_it, const Particle& particle) const;			// Rendering for particles with texture 2D or no texture
		void render2DRot(VertexFormat*& vertex_it, const Particle& particle) const;		// Rendering for particles with texture 2D or no texture and rotation
		void render2DAtlas(VertexFormat*& vertex_it, const Particle& particle) const;		// Rendering for particles with texture 2D atlas
		void render2DAtlasRot(VertexFormat*& vertex_it, const Particle& particle) const;	// Rendering for particles with texture 2D atlas and rotation

	protected:

		void allocResources();
	};

	inline VitaQuadRenderer* VitaQuadRenderer::create( float scaleX /*= 1.0f*/, float scaleY /*= 1.0f*/ )
	{
		VitaQuadRenderer* obj = new VitaQuadRenderer(scaleX,scaleY);
		registerObject(obj);
		VitaInfo::VitaRegisterRenderer(obj);
		return obj;
	}

	inline void VitaQuadRenderer::setTexture(AssetTexturePtr texture)
	{
		m_pTexture = texture;
	}
}
}