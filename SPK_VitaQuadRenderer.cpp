#include "SPK_VitaQuadRenderer.h"
#include "Core/SPK_Particle.h"
#include "Core/SPK_Group.h"
#include <libdbg.h>
#include <gxm.h>
#include "Core/SPK_ArrayBuffer.h"
#include "core_system.h"
#include "SPK_VitaBuffer.h"

#define UTILSALIGN(a, s)		((s + (a-1)) & ~(a-1))

namespace SPK
{
namespace Vita
{
	int VitaQuadRenderer::s_initCount = 0;
	void* VitaQuadRenderer::s_sharedIndexBuffer = NULL;
	uint32_t VitaQuadRenderer::s_maxParticleCount = 0;

	const std::string VitaQuadRenderer::VERTEX_BUFFER_NAME("SPK_VitaQuadRenderer_Vertex");

	VertexFormat* VitaQuadRenderer::vertexBuffer = NULL;
	VertexFormat* VitaQuadRenderer::vertexIterator = NULL;

	void (VitaQuadRenderer::*VitaQuadRenderer::renderParticle)(VertexFormat*&, const Particle&) const = NULL;

	void VitaQuadRenderer::classInit(uint32_t max_particle_count)
	{
		if (s_initCount++ != 0) return;

		s_maxParticleCount = max_particle_count;

		util::MemblockHeap* gpu_heap = VitaInfo::getGpuHeap();
		s_sharedIndexBuffer = gpu_heap->allocate(max_particle_count * 6 * sizeof(uint16_t));

		uint16_t* index_it = (uint16_t*)s_sharedIndexBuffer;
		uint16_t index_offset = 0;
		for (int i = 0; i < max_particle_count; ++i)
		{
			*index_it++ = 0 + index_offset; *index_it++ = 1 + index_offset; *index_it++ = 2 + index_offset;
			*index_it++ = 0 + index_offset; *index_it++ = 2 + index_offset; *index_it++ = 3 + index_offset;
			index_offset += 4;
		}
	}

	void VitaQuadRenderer::classShutdown()
	{
		if (--s_initCount != 0) return;
		
		util::MemblockHeap* gpu_heap = VitaInfo::getGpuHeap();
		if (s_sharedIndexBuffer) gpu_heap->free(s_sharedIndexBuffer);
	}

	VitaQuadRenderer::VitaQuadRenderer(float scaleX,float scaleY) :
		QuadRendererInterface(scaleX,scaleY)
	{
		ASSERT(s_initCount > 0);
    }

	bool VitaQuadRenderer::checkBuffers(const Group& group)
	{
		VitaBuffer* groupVertexBuffer = (VitaBuffer*)group.getBuffer(VERTEX_BUFFER_NAME);
		if (!groupVertexBuffer)
			return false;

		vertexIterator = vertexBuffer = (VertexFormat*)groupVertexBuffer->getBuffer();

		return true;
	}

	void VitaQuadRenderer::createBuffers(const Group& group)
	{
		ASSERT(group.getParticles().getNbReserved() < s_maxParticleCount);

		VitaBuffer* groupVertexBuffer = (VitaBuffer*)group.createBuffer(VERTEX_BUFFER_NAME, VitaBufferCreator(4 * sizeof(VertexFormat)), 0, false);
		vertexIterator = vertexBuffer = (VertexFormat*)groupVertexBuffer->getBuffer();
		
		int max_particles = group.getParticles().getNbReserved();
		if (!group.getModel()->isEnabled(PARAM_TEXTURE_INDEX))
		{
			VertexFormat* vertex_it = vertexIterator;
			for (int i = 0; i < max_particles; ++i)
			{
				vertex_it->u = 1.0f;
				vertex_it->v = 0.0f;
				++vertex_it;

				vertex_it->u = 0.0f;
				vertex_it->v = 0.0f;
				++vertex_it;

				vertex_it->u = 0.0f;
				vertex_it->v = 1.0f;
				++vertex_it;

				vertex_it->u = 1.0f;
				vertex_it->v = 1.0f;
				++vertex_it;
			}
		}
	}

	void VitaQuadRenderer::destroyBuffers(const Group& group)
	{
		group.destroyBuffer(VERTEX_BUFFER_NAME);
	}

	bool VitaQuadRenderer::setTexturingMode(TexturingMode mode)
	{
		texturingMode = mode;
		return true;
	}

	void VitaQuadRenderer::render(const Group& group)
	{
		int nb_part = group.getNbParticles();
		if (nb_part == 0) return;

		if (!prepareBuffers(group)) return;
			
		sce::Vectormath::Simd::Aos::Matrix4 wvi;
		sce::Vectormath::Simd::Aos::Matrix4 wvp;

		if (VitaInfo::world)
		{
			sce::Vectormath::Simd::Aos::Matrix4 wv = VitaInfo::camera->m_viewMatrix * (*VitaInfo::world);
			wvi = orthoInverse(wv);
			wvp = transpose(VitaInfo::camera->m_projMatrix * wv);
		}
		else
		{
			wvi = VitaInfo::camera->m_cameraMatrix;
			wvp = transpose(VitaInfo::camera->m_projMatrix * VitaInfo::camera->m_viewMatrix);
		}
		
		const float* invModelView = (const float*)&wvi;

		if (!group.getModel()->isEnabled(PARAM_TEXTURE_INDEX))
		{
			if (!group.getModel()->isEnabled(PARAM_ANGLE))
				renderParticle = &VitaQuadRenderer::render2D;
			else
				renderParticle = &VitaQuadRenderer::render2DRot;
		}
		else
		{
			if (!group.getModel()->isEnabled(PARAM_ANGLE))
				renderParticle = &VitaQuadRenderer::render2DAtlas;
			else
				renderParticle = &VitaQuadRenderer::render2DAtlasRot;
		}

		bool globalOrientation = precomputeOrientation3D(
			group,
			Vector3D(-invModelView[8],-invModelView[9],-invModelView[10]),
			Vector3D(invModelView[4],invModelView[5],invModelView[6]),
			Vector3D(invModelView[12],invModelView[13],invModelView[14])
		);

		if (globalOrientation)
		{
			computeGlobalOrientation3D();

			for (size_t i = 0; i < group.getNbParticles(); ++i)
				(this->*renderParticle)(vertexIterator, group.getParticle(i));
		}
		else
		{
			for (size_t i = 0; i < group.getNbParticles(); ++i)
			{
				const Particle& particle = group.getParticle(i);
				computeSingleOrientation3D(particle);
				(this->*renderParticle)(vertexIterator, particle);
			}
		}
		
		// bind buffers and draw
		{
			SceGxmContext* context = VitaInfo::getContext();
			sceGxmSetVertexProgram(context, s_vertexShader);
			sceGxmSetFragmentProgram(context, s_blendModeShaders[m_blendingMode].shader);

			void* vertexUniformBuffer=NULL;
			SceGxmErrorCode err = sceGxmReserveVertexDefaultUniformBuffer(context, &vertexUniformBuffer);
			SCE_DBG_ASSERT(err == SCE_OK);

			err = sceGxmSetUniformDataF(vertexUniformBuffer, s_modelViewProjectionParam, 0, 16, (float*)&wvp);
			SCE_DBG_ASSERT(err == SCE_OK);

			///*
			err = sceGxmSetFragmentTexture(context, s_blendModeShaders[m_blendingMode].textureSampler, &m_pTexture->texture());
			SCE_DBG_ASSERT(err == SCE_OK);
			/**/

			sceGxmSetFrontDepthWriteEnable(context, SCE_GXM_DEPTH_WRITE_DISABLED);
			sceGxmSetCullMode(context, SCE_GXM_CULL_NONE);

			err = sceGxmSetVertexStream(context, 0, vertexBuffer);
			SCE_DBG_ASSERT(err == SCE_OK);

			sceGxmDraw(context, SCE_GXM_PRIMITIVE_TRIANGLES, SCE_GXM_INDEX_FORMAT_U16, s_sharedIndexBuffer, nb_part * 6);

			// restore some default states == this should be fixed !!!!
			// TODO - remove and handle better
			sceGxmSetCullMode(context, SCE_GXM_CULL_CW);
			sceGxmSetFrontDepthWriteEnable(context, SCE_GXM_DEPTH_WRITE_ENABLED);
		}
		//---------------------------------------------------------------------------
	}

	void VitaQuadRenderer::render2D(VertexFormat*& vertex, const Particle& particle) const
	{
		scaleQuadVectors(particle,scaleX, scaleY);
		VitaCallColorAndVertex(vertex, particle);
		vertex += 4;
	}

	void VitaQuadRenderer::render2DRot(VertexFormat*& vertex, const Particle& particle) const
	{
		rotateAndScaleQuadVectors(particle,scaleX,scaleY);
		VitaCallColorAndVertex(vertex, particle);
		vertex += 4;
	}

	void VitaQuadRenderer::render2DAtlas(VertexFormat*& vertex, const Particle& particle) const
	{
		scaleQuadVectors(particle,scaleX,scaleY);
		VitaCallColorAndVertex(vertex, particle);
		VitaCallTexture2DAtlas(vertex, particle);
		vertex += 4;
	}

	void VitaQuadRenderer::render2DAtlasRot(VertexFormat*& vertex, const Particle& particle) const
	{
		rotateAndScaleQuadVectors(particle,scaleX,scaleY);
		VitaCallColorAndVertex(vertex, particle);
		VitaCallTexture2DAtlas(vertex, particle);
		vertex += 4;
	}



	void VitaQuadRenderer::VitaCallColorAndVertex(VertexFormat* vertex_it, const Particle& particle) const
	{
		float x = particle.position().x;
		float y = particle.position().y;
		float z = particle.position().z;

		float r = particle.getR();
		float g = particle.getG();
		float b = particle.getB();
		float a = particle.getParamCurrentValue(PARAM_ALPHA);

		// top left vertex
		vertex_it->x = x + quadSide().x + quadUp().x;
		vertex_it->y = y + quadSide().y + quadUp().y;
		vertex_it->z = z + quadSide().z + quadUp().z;
		vertex_it->r = r; vertex_it->g = g; vertex_it->b = b; vertex_it->a = a;
		++vertex_it;

		// top right vertex
		vertex_it->x = x - quadSide().x + quadUp().x;
		vertex_it->y = y - quadSide().y + quadUp().y;
		vertex_it->z = z - quadSide().z + quadUp().z;
		vertex_it->r = r; vertex_it->g = g; vertex_it->b = b; vertex_it->a = a;
		++vertex_it;

		// bottom right vertex
		vertex_it->x = x - quadSide().x - quadUp().x;
		vertex_it->y = y - quadSide().y - quadUp().y;
		vertex_it->z = z - quadSide().z - quadUp().z;
		vertex_it->r = r; vertex_it->g = g; vertex_it->b = b; vertex_it->a = a;
		++vertex_it;

		// bottom left vertex
		vertex_it->x = x + quadSide().x - quadUp().x;
		vertex_it->y = y + quadSide().y - quadUp().y;
		vertex_it->z = z + quadSide().z - quadUp().z;
		vertex_it->r = r; vertex_it->g = g; vertex_it->b = b; vertex_it->a = a;
		++vertex_it;
	}

	void VitaQuadRenderer::VitaCallTexture2DAtlas( VertexFormat* vertex_it, const Particle& particle ) const
	{
		computeAtlasCoordinates(particle);

		vertex_it->u = textureAtlasU0();
		vertex_it->v = textureAtlasV0();
		++vertex_it;

		vertex_it->u = textureAtlasU1();
		vertex_it->v = textureAtlasV0();
		++vertex_it;

		vertex_it->u = textureAtlasU1();
		vertex_it->v = textureAtlasV1();
		++vertex_it;

		vertex_it->u = textureAtlasU0();
		vertex_it->v = textureAtlasV1();
		++vertex_it;
	}
}}
