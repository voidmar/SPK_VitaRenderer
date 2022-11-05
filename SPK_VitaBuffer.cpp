
#include "SPK_VitaBuffer.h"
#include "cblib\Base.h"

namespace SPK
{
namespace Vita
{

	static const size_t temp_buffer_size = sizeof(float) * 40;
	static char temp_buffer[temp_buffer_size]; // for "swap"

	VitaBuffer::VitaBuffer(size_t elements, size_t elementSize) : 
		m_elements(elements), 
		m_elementSize(elementSize)
	{
		ASSERT(m_elementSize <= temp_buffer_size);

		util::MemblockHeap* gpu_heap = VitaInfo::getGpuHeap();
		m_buffer = gpu_heap->allocate(elements * elementSize);
		ASSERT(m_buffer);
	}

	VitaBuffer::~VitaBuffer()
	{
		ASSERT(m_buffer);
		util::MemblockHeap* gpu_heap = VitaInfo::getGpuHeap();
		gpu_heap->free(m_buffer);
	}

	void VitaBuffer::swap(size_t index0, size_t index1)
	{
		void* ptr0 = (void*)((intptr_t)m_buffer + index0 * m_elementSize);
		void* ptr1 = (void*)((intptr_t)m_buffer + index0 * m_elementSize);
		memcpy(temp_buffer, ptr0,        m_elementSize);
		memcpy(ptr0,        ptr1,        m_elementSize);
		memcpy(ptr1,        temp_buffer, m_elementSize);
	}

	VitaBuffer* VitaBufferCreator::createBuffer(size_t nbParticles, const SPK::Group& group) const
	{
		return new VitaBuffer(nbParticles, m_elementSize);
	}

}
}