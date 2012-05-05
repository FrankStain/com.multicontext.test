#include <algorithm>
#include <cVertexBuffer.h>

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

cVertexBuffer* cVertexBuffer::m_last_selected = NULL;

cVertexBuffer::cVertexBuffer() : m_buffer(NULL), m_id(0), m_size(0), m_capacity(0), m_stride(0), m_buffer_size(0), m_dma_lock(false), m_tex_coords(0) {
	G_ENTER();
	cGraphics::lock();
	glGenBuffers( 1, &m_id );
	G_LOG_I( "new buffer id : %d", m_id );
	cGraphics::unlock();
	G_EXIT();
};

cVertexBuffer::~cVertexBuffer(){
	G_ENTER();
	if( m_id ){
		cGraphics::lock();
		G_LOG_I( "remove buffer #%d", m_id );
		glDeleteBuffers( 1, &m_id );
		cGraphics::unlock();
	};
	G_EXIT();
};

void cVertexBuffer::reserve( const uint32_t size ){
	G_ENTER();
	if( size != m_capacity ){
		G_LOG_I( "change length from %d to %d for buffer %d", m_capacity, size, m_id );
		m_capacity = size;

		if( m_buffer ){
			G_LOG_I( "remove DMA pointer for buffer %d", m_id );
			delete[] m_buffer;
			m_buffer_size = 0;
			m_buffer = NULL;
		};

		cGraphics::lock();
		if( m_capacity ){
			G_LOG_I( "set new length for buffer %d", m_id );
			glBindBuffer( GL_ARRAY_BUFFER, m_id );
			glBufferData( GL_ARRAY_BUFFER, m_capacity * m_stride, 0, GL_STATIC_DRAW );
			glBindBuffer( GL_ARRAY_BUFFER, 0 );

			if( !g_ext_mapbuffer ){
				G_LOG_I( "create new DMA pointer for buffer %d", m_id );
				m_buffer = new uint8_t[ m_capacity * m_stride ];
				m_buffer_size = m_capacity * m_stride;
				memset( m_buffer, 0, m_capacity );
			};
		}else{
			G_LOG_I( "remove buffer #%d", m_id );
			glDeleteBuffers( 1, &m_id );
			glGenBuffers( 1, &m_id );
			G_LOG_I( "new buffer id : %d", m_id );
		};
		cGraphics::unlock();
	};
	G_EXIT();
};

void cVertexBuffer::resize( const uint32_t size ){
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

void* cVertexBuffer::lock( uint32_t offset, uint32_t size, const bool only_write ){
	if( m_size <= offset ){
		return NULL;
	};

	size = ( size )? min( m_size - offset, size ) : ( m_size - offset );
	if( !size ){
		return NULL;
	};

	uint8_t* ptr = NULL;
	if( g_ext_mapbuffer ){
		cGraphics::lock();
		glBindBuffer( GL_ARRAY_BUFFER, m_id );
		ptr = (uint8_t*)glMapBufferOES( GL_ARRAY_BUFFER, GL_WRITE_ONLY_OES );
		glBindBuffer( GL_ARRAY_BUFFER, 0 );
		cGraphics::unlock();
	};

	m_dma_lock = false;
	if( !ptr ){
		ptr = m_buffer;
		m_dma_lock = true;
	};

	return ( ptr )? &ptr[ offset * m_stride ] : NULL;
};

void cVertexBuffer::unlock(){
	cGraphics::lock();
	glBindBuffer( GL_ARRAY_BUFFER, m_id );

	if( m_dma_lock ){
		glBufferData( GL_ARRAY_BUFFER, m_buffer_size, m_buffer, GL_STATIC_DRAW );
		m_dma_lock = false;
	}else{
		glUnmapBufferOES( GL_ARRAY_BUFFER );
	};

	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	cGraphics::unlock();
};

void cVertexBuffer::set_format( vector<cVertexFormat>& format ){
	G_ENTER();
	m_stride = 0;
	m_format.clear();
	m_states[0] = m_states[1] = m_states[2] = false;
	for( vector<cVertexFormat>::iterator fd = format.begin(); format.end() != fd; fd++ ){
		m_format.push_back( *fd );
		switch( fd->m_usage ){
			case duPosition:
				m_stride	+= 3 * sizeof( GLfloat );
				m_states[0]	= true;
				G_LOG_I( "for buffer %d, Vertex3f position at %d offset", m_id, fd->m_offset );
			break;
			case duNormal:
				m_stride	+= sizeof( GLfloat );
				m_states[1]	= true;
				G_LOG_I( "for buffer %d, normal value at %d offset", m_id, fd->m_offset );
			break;
			case duColor:
				m_stride	+= 4;
				m_states[2]	= true;
				G_LOG_I( "for buffer %d, Color4b color at %d offset", m_id, fd->m_offset );
			break;
			case duTexCoord:
				m_stride		+= 2 * sizeof( GLfloat );
				m_tex_coords	= max<int32_t>( m_tex_coords, fd->m_usage_index );
				G_LOG_I( "for buffer %d, #%d texture's UV value at %d offset", m_id, fd->m_usage_index, fd->m_offset );
			break;
		};
	};
	reserve( 0 );
	G_LOG_I( "new vertex size is %d for buffer %d", m_stride, m_id );
	G_EXIT();
};

void cVertexBuffer::select( const uint32_t offset ){
	static int32_t state_def[] = {
		GL_VERTEX_ARRAY, GL_NORMAL_ARRAY, GL_COLOR_ARRAY, 0
	};
	static int32_t last_tex_count = 0;

	if( m_last_selected != this ){
		glBindBuffer( GL_ARRAY_BUFFER, m_id );

		for( int32_t vs = 0; 3 > vs; vs++ ){
			if( m_states[ vs ] ){
				glEnableClientState( state_def[ vs ] );
			}else{
				glDisableClientState( state_def[ vs ] );
			};
		};

		int32_t max_units = 0;
		glGetIntegerv( GL_MAX_TEXTURE_UNITS, (GLint*)&max_units );

		int32_t tex = last_tex_count;
		for( ; ( max_units > tex ) && ( m_tex_coords > tex ); tex++ ){
			glClientActiveTexture( GL_TEXTURE0 + tex );
			glActiveTexture( GL_TEXTURE0 + tex );
			glEnable( GL_TEXTURE_2D );
			glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		};

		tex = m_tex_coords;
		for( ; ( max_units > tex ) && ( last_tex_count > tex ); tex++ ){
			glClientActiveTexture( GL_TEXTURE0 + tex );
			glActiveTexture( GL_TEXTURE0 + tex );
			glEnable( GL_TEXTURE_2D );
			glDisableClientState( GL_TEXTURE_COORD_ARRAY );
		};

		last_tex_count = m_tex_coords;

		uint8_t* ptr = (uint8_t*)( offset * m_stride );
		for( vector<cVertexFormat>::iterator frm = m_format.begin(); m_format.end() != frm; frm++ ){
			switch( frm->m_usage ){
				case duPosition:
					G_LOG_I( "position Vector3f binded to 0x%08X pointer", ptr + frm->m_offset );
					glVertexPointer( 3, GL_FLOAT, m_stride, ptr + frm->m_offset );
				break;
				case duNormal:
					G_LOG_I( "normal Vector1f binded to 0x%08X pointer", ptr + frm->m_offset );
					glNormalPointer( GL_FLOAT, m_stride, ptr + frm->m_offset );
				break;
				case duColor:
					G_LOG_I( "color Vector4b binded to 0x%08X pointer", ptr + frm->m_offset );
					glColorPointer( 4, GL_UNSIGNED_BYTE, m_stride, ptr + frm->m_offset );
				break;
				case duTexCoord:
					G_LOG_I( "TEXTURE%d Vector2f binded to 0x%08X pointer", frm->m_usage_index, ptr + frm->m_offset );
					glClientActiveTexture( GL_TEXTURE0 + frm->m_usage_index );
					glTexCoordPointer( 2, GL_FLOAT, m_stride, ptr + frm->m_offset );
				break;
			};
		};

		m_last_selected = this;
	};
};
