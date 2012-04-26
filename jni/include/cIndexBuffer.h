#pragma once

#include <cGraphics.h>

class cIndexBuffer {
protected:
	static cIndexBuffer*	m_last_buffer;

	uint16_t*	m_buffer;
	GLuint		m_id;
	uint32_t	m_size;
	uint32_t	m_capacity;
	uint32_t	m_buffer_size;
	bool		m_dma_lock;
	
public:
	cIndexBuffer();
	virtual ~cIndexBuffer();

	const uint32_t size() const { return m_size; };
	const uint32_t capacity() const { return m_capacity; };

	void reserve( const uint32_t );
	void resize( const uint32_t );

	uint16_t* lock( uint32_t = 0, uint32_t = 0 );
	void unlock();
	void select();
};