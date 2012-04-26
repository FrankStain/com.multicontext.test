#include <string>

#include "main.h"
#include "cNativeApplication.h"

const char LOG_TAG[] = "nt-core";

void android_main( cNativeApplication& app )
{
	NT_LOG_DBG( "START" );

	while( !app.state().m_terminated )
	{
		while( app.process_messages() ) {};
	};

	NT_LOG_DBG( "STOP" );
};
