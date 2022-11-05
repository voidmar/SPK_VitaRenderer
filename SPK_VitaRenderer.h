#ifndef SPK_VITA_RENDERER_H
#define SPK_VITA_RENDERER_H

#include "SPK_VitaInfo.h"
#include "Core/SPK_Renderer.h"
#include "Core/SPK_ArrayBuffer.h"
#include <vectormath.h>

namespace SPK
{
namespace Vita
{
	struct VertexFormat
	{
		float x, y, z;
		float r, g, b, a;
		float u, v;
	};

	/**
	* @class VitaRenderer
	*/
	class VitaRenderer : public Renderer
	{
	public :
	
		static void initShaders(SceGxmShaderPatcher* pShaderPatcher);

		/////////////////
		// Constructor //
		/////////////////

		/** @brief Constructor of VitaRenderer */
		VitaRenderer();

		////////////////
		// Destructor //
		////////////////

		/** @brief Destructor of VitaRenderer */
		virtual ~VitaRenderer();

		/////////////
		// Setters //
		/////////////

		virtual void setBlending(BlendingMode blendMode) { m_blendingMode = blendMode; }

	protected:
		BlendingMode m_blendingMode;

		static SceGxmShaderPatcherId			s_vertexShaderId;
		static SceGxmVertexProgram*				s_vertexShader;
		static const SceGxmProgramParameter*	s_modelViewProjectionParam;

		struct BlendModeInput
		{
			const SceGxmProgram*	program;
			SceGxmShaderPatcherId	shaderId;
		};

		struct BlendMode
		{
			BlendModeInput*			input;
			SceGxmFragmentProgram*	shader;
			int						textureSampler;
		};

		static BlendModeInput	s_blendModeShaderInputs[];
		static BlendMode		s_blendModeShaders[];
	};

}}

#endif
