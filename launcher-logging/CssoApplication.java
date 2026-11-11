package me.nillerusr;

import android.app.Application;

public class CssoApplication extends Application
{
	@Override
	public void onCreate()
	{
		super.onCreate();

		FileLog.init( this );
		FileLog.installUncaughtHandler();
		FileLog.i( "CssoApplication", "Application.onCreate" );
	}
}
