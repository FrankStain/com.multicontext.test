#pragma once

#include <vector>
#include "cGraphics.h"

enum eDataUsage {
	duPosition,
	duNormal,
	duColor,
	duTexCoord,
	
	duUndefined = 0xFF
};

struct cVertexFormat {
	int16_t		m_offset;
	eDataUsage	m_usage;
	int8_t		m_usage_index;
	
	cVertexFormat( eDataUsage usage, int16_t offset, int8_t index = 0 ) : m_offset(offset), m_usage(usage), m_usage_index(index) {};
	cVertexFormat( const cVertexFormat& origin ) : m_offset(origin.m_offset), m_usage(origin.m_usage), m_usage_index(origin.m_usage_index) {};
};

class cVertexBuffer {
protected:
	static cVertexBuffer*	m_last_selected;
	uint8_t*				m_buffer;
	GLuint					m_id;
	uint32_t				m_size;
	uint32_t				m_capacity;
	uint32_t				m_stride;
	uint32_t				m_buffer_size;
	bool					m_dma_lock;
	int32_t					m_tex_coords;
	bool					m_states[3];

	std::vector<cVertexFormat>	m_format;

public:
	cVertexBuffer();
	virtual ~cVertexBuffer();

	const uint32_t size() const { return m_size; };
	const uint32_t capacity() const { return m_capacity; };
	const uint32_t stride() const { return m_stride; };

	void reserve( const uint32_t );
	void resize( const uint32_t );

	void set_format( std::vector<cVertexFormat>& );
	void* lock( uint32_t = 0, uint32_t = 0, const bool = true );
	void unlock();
	void select( const uint32_t = 0 );
};