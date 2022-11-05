#ifndef SPK_VITAINFO_H
#define SPK_VITAINFO_H

#include "Core/SPK_Group.h"

#include <gxm/context.h>
#include <vector>
#include <map>
#include <gxm/shader_patcher.h>
#include "util.h"

namespace object 
{ 
	class Camera; 
}

namespace SPK
{
namespace Vita
{
	class VitaRenderer;

	class VitaInfo
	{
	private:
		static SceGxmContext* pContext;
		static SceGxmShaderPatcher* pShaderPatcher;
        static util::MemblockHeap gpuHeap;

		static std::vector<VitaRenderer *> renderers;

	public:
		static object::Camera* camera;
		static const sce::Vectormath::Simd::Aos::Matrix4* world;

		static SceGxmContext* getContext();
		static SceGxmShaderPatcher* getShaderPatcher();
        static util::MemblockHeap* getGpuHeap();

		static void setContext(SceGxmContext* context);
		static void setShaderPatcher(SceGxmShaderPatcher* patcher);

		static void VitaRegisterRenderer(VitaRenderer *renderer);
		static void VitaReleaseRenderer(VitaRenderer *renderer);

        static void InitHeap();
        static void DestroyHeap();
	};

	inline void VitaInfo::VitaRegisterRenderer(VitaRenderer *renderer)
	{
		renderers.push_back(renderer);
	}

	inline void VitaInfo::VitaReleaseRenderer(VitaRenderer *renderer)
	{
		for(std::vector<VitaRenderer *>::iterator it = renderers.begin(); it != renderers.end(); )
		{
			if (*it == renderer)
				it = renderers.erase(it);
			else
				++it;
		}
	}

	inline SceGxmContext* VitaInfo::getContext()
	{
		return VitaInfo::pContext;
	}

	inline void VitaInfo::setContext(SceGxmContext* context)
	{
		VitaInfo::pContext = context;
	}

	inline SceGxmShaderPatcher* VitaInfo::getShaderPatcher()
	{
		return VitaInfo::pShaderPatcher;
	}

	inline util::MemblockHeap* VitaInfo::getGpuHeap()
	{
		return &VitaInfo::gpuHeap;
	}
}}

#endif
