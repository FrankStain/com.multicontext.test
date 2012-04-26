#include <vector>
#include <string>
#include <pthread.h>

using namespace std;

#include "main.h"
#include "cVertexBuffer.h"
#include "cIndexBuffer.h"

const char LOG_TAG[] = "nt-core";

void render_with_select_ibo();
void render_with_index_pointer();

#ifdef USE_SELECT_IBO
#define render	render_with_select_ibo
#else
#define render	render_with_index_pointer
#endif

cVertexBuffer* vb;
cIndexBuffer* ib;
bool first_load = true;

const struct cVertex {
	float	m_pos[3];
	int32_t	m_color;
} vertices[] = {
	{ { 0.25f, 0.25f, 0.0f }, 0xFFFF0000 },
	{ { 0.75f, 0.25f, 0.0f }, 0xFF00FF00 },
	{ { 0.25f, 0.75f, 0.0f }, 0xFF0000FF },
	{ { 0.75f, 0.75f, 0.0f }, 0xFF888888 }
};

const uint16_t indices[] = {
	0, 0, 0, 0, 0, 0, 0, 1, 2, 1, 3, 2
};

const int32_t vert_size = sizeof( vertices );
const int32_t vert_count = vert_size / sizeof( cVertex );
const int32_t ind_size = sizeof( indices );
const int32_t ind_count = ind_size / sizeof( uint16_t );

void android_main( cNativeApplication& app )
{
	NT_LOG_DBG( "START" );
	cMainCommandProcessor proc;

	app.set_command_processor( &proc );
	while( !app.state().m_terminated )
	{
		while( app.process_messages() ) {};
		if( cGraphics::is_ready() ){
			glClearColor( 0.5f, 0.8f, 0.6f, 0.0f );
			glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
			
			render();
						
			cGraphics::instance()->swap();
		};
	};
	app.set_command_processor( NULL );

	NT_LOG_DBG( "STOP" );
};

void* loader_entry( void* param ){
	NT_LOG_INFO( "ENTER [TID:%d]", pthread_self() );

	vb = new cVertexBuffer();
	ib = new cIndexBuffer();
	
	NT_LOG_INFO( "prepare FVF declaration" );
	vector<cVertexFormat> fvf;
	fvf.push_back( cVertexFormat( duPosition, 0 ) );
	fvf.push_back( cVertexFormat( duColor, 12 ) );
	
	NT_LOG_INFO( "set VBO format" );
	vb->set_format( fvf );
	NT_LOG_INFO( "set VBO size to %d", vert_count );
	vb->resize( vert_count );
	NT_LOG_INFO( "lock VBO" );
	uint8_t* dst_vertices = (uint8_t*)vb->lock();
	memcpy( dst_vertices, vertices, vert_size );
	vb->unlock();
	NT_LOG_INFO( "unlock VBO" );

	NT_LOG_INFO( "set IBO count to %d", ind_count );
	ib->resize( ind_count );
	NT_LOG_INFO( "lock IBO" );
	uint16_t* dst_indices = ib->lock();
	memcpy( dst_indices, indices, ind_size );
	ib->unlock();
	NT_LOG_INFO( "unlock IBO" );
		
	NT_LOG_INFO( "EXIT [TID:%d]", pthread_self() );	
	return NULL;
};

void create_scene(){
	NT_LOG_DBG( "ENTER [TID:%d]", pthread_self() );
	if( first_load ){
#ifdef USE_MULTICONTEXT
		pthread_t	tid = 0;
		pthread_create( &tid, NULL, loader_entry, NULL );
		pthread_join( tid, NULL );
#else
		loader_entry( NULL );
#endif
		first_load = false;
	};
	NT_LOG_DBG( "EXIT [TID:%d]", pthread_self() );
};

const bool cMainCommandProcessor::process_command( const sAppMessage& msg, cNativeApplication& app ){
	switch( msg.m_command ){
		case AM_WINDOW_CHANGE:
			if( cGraphics::is_ready() ){
				create_scene();
			};
		break;
		case AM_APP_DESTROY:
			delete ib;
			delete vb;
			ib = NULL;
			vb = NULL;
		break;
	};
};

void render_with_select_ibo(){
	if( !( vb && ib ) ){
		return;
	};

	ib->select();
	vb->select();
	uint8_t* idx = 0;
	glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, idx + 0x0CL );
};

void render_with_index_pointer(){
	if( !( vb && ib ) ){
		return;
	};

	vb->select();
	uint16_t* idx = ib->lock();

	glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, &idx[6] );
	ib->unlock();
};
