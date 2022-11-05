#pragma once

#include "SPK_VitaInfo.h"
#include "Core/SPK_Buffer.h"

namespace SPK
{
namespace Vita
{
	class VitaBuffer : public SPK::Buffer
	{
	public:

		void* getBuffer() const;

	private:

		friend class VitaBufferCreator;
		size_t m_elements;
		size_t m_elementSize;
		void* m_buffer;

		VitaBuffer(size_t elements, size_t elementSize);
		virtual ~VitaBuffer();

		virtual void swap(size_t index0, size_t index1);
	};

	inline void* VitaBuffer::getBuffer() const
	{
		return m_buffer;
	}

	class VitaBufferCreator : public SPK::BufferCreator
	{
	public:

		VitaBufferCreator(size_t elementSize);

	private:

		size_t m_elementSize;

		virtual VitaBuffer* createBuffer(size_t nbParticles, const Group& group) const;

	};

	inline VitaBufferCreator::VitaBufferCreator(size_t elementSize): m_elementSize(elementSize)
	{
	}

}
}