#include <gxm/blending.h>
#include "SPK_VitaRenderer.h"
#include <libdbg.h>

extern const SceGxmProgram _binary_particle_v_gxp_start;
extern const SceGxmProgram _binary_particle_f_gxp_start;
extern const SceGxmProgram _binary_particle_red_f_gxp_start;
extern const SceGxmProgram _binary_particle_green_f_gxp_start;
extern const SceGxmProgram _binary_particle_blue_f_gxp_start;
extern const SceGxmProgram _binary_particle_alpha_f_gxp_start;

namespace SPK
{
namespace Vita
{

	SceGxmShaderPatcherId			VitaRenderer::s_vertexShaderId = NULL;
	SceGxmVertexProgram*			VitaRenderer::s_vertexShader = NULL;
	const SceGxmProgramParameter*	VitaRenderer::s_modelViewProjectionParam = NULL;

	VitaRenderer::BlendModeInput VitaRenderer::s_blendModeShaderInputs[] = {
		{ &_binary_particle_f_gxp_start }, 
		{ &_binary_particle_red_f_gxp_start },
		{ &_binary_particle_green_f_gxp_start },
		{ &_binary_particle_blue_f_gxp_start },
		{ &_binary_particle_alpha_f_gxp_start },
	};

	VitaRenderer::BlendMode VitaRenderer::s_blendModeShaders[11] = {
		{ &VitaRenderer::s_blendModeShaderInputs[0] }, // BLENDING_NONE
		{ &VitaRenderer::s_blendModeShaderInputs[0] }, // BLENDING_ADD
		{ &VitaRenderer::s_blendModeShaderInputs[0] }, // BLENDING_ALPHA
		{ &VitaRenderer::s_blendModeShaderInputs[1] }, // BLENDING_ADD_MASK_R
		{ &VitaRenderer::s_blendModeShaderInputs[2] }, // BLENDING_ADD_MASK_G
		{ &VitaRenderer::s_blendModeShaderInputs[3] }, // BLENDING_ADD_MASK_B
		{ &VitaRenderer::s_blendModeShaderInputs[4] }, // BLENDING_ADD_MASK_A
		{ &VitaRenderer::s_blendModeShaderInputs[1] }, // BLENDING_ALPHA_MASK_R
		{ &VitaRenderer::s_blendModeShaderInputs[2] }, // BLENDING_ALPHA_MASK_G
		{ &VitaRenderer::s_blendModeShaderInputs[3] }, // BLENDING_ALPHA_MASK_B
		{ &VitaRenderer::s_blendModeShaderInputs[4] }, // BLENDING_ALPHA_MASK_A
	};

	VitaRenderer::VitaRenderer() :
		m_blendingMode(BLENDING_ADD)
	{}

	VitaRenderer::~VitaRenderer() {VitaInfo::VitaReleaseRenderer(this);}

	void VitaRenderer::initShaders(SceGxmShaderPatcher* pShaderPatcher)
	{
		SceGxmErrorCode err;
		//m_pShaderPatcher = pShaderPatcher;

		// register vertex shader
		if (s_vertexShaderId == NULL)
		{
			err = sceGxmShaderPatcherRegisterProgram(pShaderPatcher, &_binary_particle_v_gxp_start, &s_vertexShaderId);
			SCE_DBG_ASSERT(err == SCE_OK);
		}

		// There are 3 attributes in the vertex stream: position, color, and texture coordinates.  
		// Fill out the SceGxmVertexAttribute structure for each one of these attributes.
		SceGxmVertexAttribute particleVertexAttributes[3];	

		// position
		int offset=0;	
		particleVertexAttributes[0].streamIndex = 0;
		particleVertexAttributes[0].offset = offset;
		particleVertexAttributes[0].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		particleVertexAttributes[0].componentCount = 3;
		const SceGxmProgramParameter *param = sceGxmProgramFindParameterByName( &_binary_particle_v_gxp_start, "aPosition" );
		SCE_DBG_ASSERT_MSG( param , "Error during looking up the parameter \n");
		SCE_DBG_ASSERT_MSG( ( sceGxmProgramParameterGetCategory(param) == SCE_GXM_PARAMETER_CATEGORY_ATTRIBUTE), "Incorrect parameter category attribute\n");
		particleVertexAttributes[0].regIndex = sceGxmProgramParameterGetResourceIndex( param );
		offset += sizeof(float) * particleVertexAttributes[0].componentCount;

		// color
		particleVertexAttributes[1].streamIndex = 0;
		particleVertexAttributes[1].offset = offset;
		particleVertexAttributes[1].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		particleVertexAttributes[1].componentCount = 4;
		param = sceGxmProgramFindParameterByName( &_binary_particle_v_gxp_start, "aColor" );
		SCE_DBG_ASSERT_MSG( param , "Error during looking up the parameter \n");
		SCE_DBG_ASSERT_MSG( ( sceGxmProgramParameterGetCategory(param) == SCE_GXM_PARAMETER_CATEGORY_ATTRIBUTE), "Incorrect parameter category attribute\n");
		particleVertexAttributes[1].regIndex = sceGxmProgramParameterGetResourceIndex( param );
		offset += sizeof(float) * particleVertexAttributes[1].componentCount;

		// texture coordinate
		particleVertexAttributes[2].streamIndex = 0;
		particleVertexAttributes[2].offset = offset;
		particleVertexAttributes[2].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		particleVertexAttributes[2].componentCount = 2;
		param = sceGxmProgramFindParameterByName( &_binary_particle_v_gxp_start, "aTexcoord" );
		SCE_DBG_ASSERT_MSG( param , "Error during looking up the parameter \n");
		SCE_DBG_ASSERT_MSG( ( sceGxmProgramParameterGetCategory(param) == SCE_GXM_PARAMETER_CATEGORY_ATTRIBUTE), "Incorrect parameter category attribute\n");
		particleVertexAttributes[2].regIndex = sceGxmProgramParameterGetResourceIndex( param );

		// The attributes are all going to be interleaved in the buffer, so we are only going to have 1 vertex stream
		SceGxmVertexStream particleVertexStreams;
		particleVertexStreams.stride = sizeof(VertexFormat);
		particleVertexStreams.indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

		// create a patched vertex programs
		err = sceGxmShaderPatcherCreateVertexProgram(
			pShaderPatcher,						// Shader patcher
			s_vertexShaderId,					// ID returned from sceGxmShaderPatcherRegisterProgram
			particleVertexAttributes,			// The vertex attributes description created previously
			3,									// Number of attributes
			&particleVertexStreams,				// vertex stream
			1,									// number of vertex streams
			&s_vertexShader);
		SCE_DBG_ASSERT(err == SCE_OK);

		// get vertex uniform parameter 
		s_modelViewProjectionParam = sceGxmProgramFindParameterByName(&_binary_particle_v_gxp_start, "modelViewProj" );
		SCE_DBG_ASSERT_MSG( s_modelViewProjectionParam , "Error during looking up the parameter \n");
		SCE_DBG_ASSERT_MSG( ( sceGxmProgramParameterGetCategory(s_modelViewProjectionParam) == SCE_GXM_PARAMETER_CATEGORY_UNIFORM), "Incorrect parameter category uniform\n");

		// traverse through the different types of fragment shaders and create patched ones
		for (uint32_t i=0; i <= BLENDING_ALPHA_MASK_A; i++)
		{
			BlendMode& shader = s_blendModeShaders[i];

			// register fragment shader
			if (shader.input->shaderId == NULL)
			{
				err = sceGxmShaderPatcherRegisterProgram(pShaderPatcher, shader.input->program, &shader.input->shaderId);
				SCE_DBG_ASSERT(err == SCE_OK);
			}

			// create blend mode
			SceGxmBlendInfo	blendInfo;
			SceGxmBlendInfo* blendInfoPtr;
			switch(i)
			{
			case BLENDING_NONE:
				blendInfoPtr = NULL;
				break;
			case BLENDING_ADD:
			case BLENDING_ADD_MASK_R:
			case BLENDING_ADD_MASK_G:
			case BLENDING_ADD_MASK_B:
			case BLENDING_ADD_MASK_A:
				blendInfo.colorFunc = SCE_GXM_BLEND_FUNC_ADD;
				blendInfo.alphaFunc = SCE_GXM_BLEND_FUNC_ADD;
				blendInfo.colorSrc = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
				blendInfo.colorDst = SCE_GXM_BLEND_FACTOR_ONE;
				blendInfo.alphaSrc = SCE_GXM_BLEND_FACTOR_ONE;
				blendInfo.alphaDst = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				blendInfo.colorMask = SCE_GXM_COLOR_MASK_ALL;
				blendInfoPtr = &blendInfo;
				break;
			case BLENDING_ALPHA:
			case BLENDING_ALPHA_MASK_R:
			case BLENDING_ALPHA_MASK_G:
			case BLENDING_ALPHA_MASK_B:
			case BLENDING_ALPHA_MASK_A:
				blendInfo.colorFunc = SCE_GXM_BLEND_FUNC_ADD;
				blendInfo.alphaFunc = SCE_GXM_BLEND_FUNC_ADD;
				blendInfo.colorSrc = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
				blendInfo.colorDst = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				blendInfo.alphaSrc = SCE_GXM_BLEND_FACTOR_ONE;
				blendInfo.alphaDst = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				blendInfo.colorMask = SCE_GXM_COLOR_MASK_ALL;
				blendInfoPtr = &blendInfo;
				break;
			default:
				blendInfoPtr = NULL;
				SCE_DBG_ASSERT(0);
			}

			
			// get texture sampler parameters
			///*
			param = sceGxmProgramFindParameterByName(shader.input->program, "partTexture" );
			SCE_DBG_ASSERT_MSG( param , "Error during looking up the parameter \n");
			SCE_DBG_ASSERT_MSG( ( sceGxmProgramParameterGetCategory(param) == SCE_GXM_PARAMETER_CATEGORY_SAMPLER), "Incorrect parameter category texture unit\n");
			shader.textureSampler = sceGxmProgramParameterGetResourceIndex( param );
			/**/

			// create fragment program
			err = sceGxmShaderPatcherCreateFragmentProgram(
				pShaderPatcher,										// Shader Patcher
				shader.input->shaderId,								// Registered fragment shader ID
				SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
				SCE_GXM_MULTISAMPLE_NONE,							//  MSAA
				blendInfoPtr,										// Blend equation
				&_binary_particle_v_gxp_start,
				&shader.shader);
			SCE_DBG_ASSERT(err == SCE_OK);
		}
	}


}}
