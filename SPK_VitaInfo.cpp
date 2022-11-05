//////////////////////////////////////////////////////////////////////////////////
// SPARK particle engine														//
// Copyright (C) 2009 - foulon matthieu - stardeath@wanadoo.fr					//
//																				//
#include "SPK_VitaInfo.h"
#include "SPK_VitaRenderer.h"
#include "SPK_VitaQuadRenderer.h"

namespace SPK
{
namespace Vita
{
	object::Camera* VitaInfo::camera = NULL;
	const sce::Vectormath::Simd::Aos::Matrix4* VitaInfo::world = NULL;

	SceGxmContext* VitaInfo::pContext = NULL;
	SceGxmShaderPatcher* VitaInfo::pShaderPatcher = NULL;
    util::MemblockHeap VitaInfo::gpuHeap;

	std::vector<VitaRenderer *> VitaInfo::renderers = std::vector<VitaRenderer *>();

	void VitaInfo::setShaderPatcher(SceGxmShaderPatcher* patcher)
	{
		VitaInfo::pShaderPatcher = patcher;
		VitaRenderer::initShaders(patcher);
	}

    void VitaInfo::InitHeap()
    {
	    gpuHeap.initialize("Particle heap", SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE,
		                           40 * 1024 * 1024, NULL, true, SCE_GXM_MEMORY_ATTRIB_READ);

		VitaQuadRenderer::classInit(4000);
    }

    void VitaInfo::DestroyHeap()
    {
        gpuHeap.finalize();

		VitaQuadRenderer::classShutdown();
    }
}}
