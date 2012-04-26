#include <algorithm>
#include "cIndexBuffer.h"

using namespace std;

#ifdef USE_GRAPHICS_BUFFERS_LOGS
#define G_LOG_I			NT_LOG_INFO
#define G_LOG_W			NT_LOG_WARN
#define G_LOG_D			NT_LOG_DBG
#else
#define G_LOG_I(...)
#define G_LOG_W(...)
#define G_LOG_D(...)
#endif

#define	G_LOG_E			NT_LOG_ERR
#define G_ENTER(...)	G_LOG_D( "ENTER" )
#define G_EXIT(...)		G_LOG_D( "EXIT" )
#define G_FAIL(...)		G_LOG_W( "EXIT" )

extern bool g_ext_mapbuffer;
extern bool g_ext_bufferobject;

extern void* glMapBufferOES( GLenum target, GLenum access );
extern GLboolean glUnmapBufferOES( GLenum target );

cIndexBuffer*	cIndexBuffer::m_last_buffer = NULL;

cIndexBuffer::cIndexBuffer() : m_buffer(NULL), m_id(0), m_size(0), m_capacity(0), m_buffer_size(0), m_dma_lock(false) {
	G_ENTER();
	cGraphics::lock();
	glGenBuffers( 1, &m_id );
	G_LOG_I( "new buffer id : %d", m_id );
	cGraphics::unlock();
	G_EXIT();
};

cIndexBuffer::~cIndexBuffer(){
	G_ENTER();
	if( m_id ){
		cGraphics::lock();
		G_LOG_I( "remove buffer #%d", m_id );
		glDeleteBuffers( 1, &m_id );
		cGraphics::unlock();
	};
	G_EXIT();
};

void cIndexBuffer::reserve( const uint32_t size ){
	G_ENTER();
	if( size != m_capacity ){
		G_LOG_I( "change length from %d to %d for buffer %d", m_capacity, size, m_id );
		m_capacity = size;

		if( m_buffer ){
			G_LOG_I( "remove DMA pointer for buffer %d", m_id );
			delete[] m_buffer;
			m_buffer = NULL;
			m_buffer_size = 0;
		};

		cGraphics::lock();
		if( m_capacity ){
			G_LOG_I( "set new length for buffer %d", m_id );
			glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_id );
			glBufferData( GL_ELEMENT_ARRAY_BUFFER, m_capacity * sizeof( uint16_t ), 0, GL_STATIC_DRAW );
			glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

			if( !g_ext_mapbuffer ){
				G_LOG_I( "create new DMA pointer for buffer %d", m_id );
				m_buffer = new uint16_t[ m_capacity ];
				m_buffer_size = m_capacity * sizeof( uint16_t );
				memset( m_buffer, 0, m_buffer_size );
			};
		};
		cGraphics::unlock();
	};
	G_EXIT();
};

void cIndexBuffer::resize( const uint32_t size ){
	G_ENTER();
	if( size != m_size ){
		G_LOG_I( "change size from %d to %d for buffer %d", m_size, size, m_id );
		m_size = size;
		if( m_size > m_capacity ){
			reserve( m_size );
		};
	};
	G_EXIT();
};

uint16_t* cIndexBuffer::lock( uint32_t offset, uint32_t size ){
	if( m_size <= offset ){
		return NULL;
	};

	size = ( size )? min( m_size - offset, size ) : ( m_size - offset );
	if( !size ){
		return NULL;
	};

	uint16_t* ptr = NULL;
	if( g_ext_mapbuffer ){
		cGraphics::lock();
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_id );
		ptr = (uint16_t*)glMapBufferOES( GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY_OES );
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
		cGraphics::unlock();
	};

	m_dma_lock = false;
	if( !ptr ){
		ptr = m_buffer;
		m_dma_lock = true;
	};

	return ( ptr )? &ptr[ offset ] : NULL;
};

void cIndexBuffer::unlock(){
	cGraphics::lock();
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_id );

	if( m_dma_lock ){
		glBufferData( GL_ELEMENT_ARRAY_BUFFER, m_buffer_size, m_buffer, GL_STATIC_DRAW );
		m_dma_lock = false;
	}else{
		glUnmapBufferOES( GL_ELEMENT_ARRAY_BUFFER );
	};

	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
	cGraphics::unlock();
};

void cIndexBuffer::select(){
	if( m_last_buffer != this ){
		G_LOG_I( "bind IBO %d", m_id );
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_id );
		m_last_buffer = this;
	};
};