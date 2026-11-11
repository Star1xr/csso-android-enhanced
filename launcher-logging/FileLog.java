package me.nillerusr;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Build;
import android.util.Log;

import java.io.File;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Date;
import java.util.Locale;

/**
 * File based logging for the launcher and the SDL/engine activity.
 *
 * The APK is normally installed on devices where logcat is not reachable.
 * Everything important is therefore appended to launcher.log in a directory
 * the user can open with a file manager.
 */
public final class FileLog
{
	private static final String TAG = "FileLog";
	private static final long MAX_SIZE = 8L * 1024L * 1024L;
	private static final Object LOCK = new Object();

	private static File logFile;
	private static boolean initialized;
	private static boolean uncaughtInstalled;

	private FileLog()
	{
	}

	public static synchronized void init( Context context )
	{
		if( initialized )
			return;

		initialized = true;

		try
		{
			File dir = chooseDir( context );
			if( dir != null )
			{
				if( !dir.exists() )
					dir.mkdirs();

				logFile = new File( dir, "launcher.log" );

				if( logFile.length() > MAX_SIZE )
				{
					File old = new File( dir, "launcher.log.old" );
					if( old.exists() )
						old.delete();
					logFile.renameTo( old );
				}
			}
		}
		catch( Throwable t )
		{
			Log.e( TAG, "init failed", t );
		}

		i( TAG, "===== launcher log started =====" );
		i( TAG, "logFile=" + ( logFile != null ? logFile.getAbsolutePath() : "(none)" ) );
		i( TAG, "package=" + context.getPackageName() );
		i( TAG, "version=" + versionName( context ) );
		i( TAG, "device=" + Build.MANUFACTURER + " " + Build.MODEL + " (" + Build.DEVICE + ")" );
		i( TAG, "android=" + Build.VERSION.RELEASE + " sdk=" + Build.VERSION.SDK_INT );
		i( TAG, "abi=" + Arrays.toString( Build.SUPPORTED_ABIS ) );
		i( TAG, "nativeLibDir=" + context.getApplicationInfo().nativeLibraryDir );
		i( TAG, "filesDir=" + context.getFilesDir() );
		i( TAG, "freeSpace=" + context.getFilesDir().getFreeSpace() );

		File external = context.getExternalFilesDir( null );
		i( TAG, "externalFilesDir=" + ( external != null ? external.getAbsolutePath() : "(null)" ) );

		try
		{
			PackageManager pm = context.getPackageManager();
			i( TAG, "vulkanHardware=" + pm.hasSystemFeature( PackageManager.FEATURE_VULKAN_HARDWARE_VERSION ) );
			i( TAG, "vulkanLevel=" + pm.hasSystemFeature( PackageManager.FEATURE_VULKAN_HARDWARE_LEVEL ) );
		}
		catch( Throwable t )
		{
			i( TAG, "vulkan feature check failed: " + t );
		}
	}

	public static void installUncaughtHandler()
	{
		synchronized( LOCK )
		{
			if( uncaughtInstalled )
				return;
			uncaughtInstalled = true;
		}

		final Thread.UncaughtExceptionHandler previous = Thread.getDefaultUncaughtExceptionHandler();
		Thread.setDefaultUncaughtExceptionHandler( ( thread, throwable ) ->
		{
			e( "Uncaught", "thread=" + thread.getName(), throwable );
			if( previous != null )
				previous.uncaughtException( thread, throwable );
		} );
	}

	public static String getDir()
	{
		synchronized( LOCK )
		{
			return logFile != null ? logFile.getParent() : null;
		}
	}

	public static String getPath()
	{
		synchronized( LOCK )
		{
			return logFile != null ? logFile.getAbsolutePath() : null;
		}
	}

	public static void i( String tag, String message )
	{
		write( "I", tag, message, null );
	}

	public static void w( String tag, String message )
	{
		write( "W", tag, message, null );
	}

	public static void e( String tag, String message, Throwable throwable )
	{
		write( "E", tag, message, throwable );
	}

	private static void write( String level, String tag, String message, Throwable throwable )
	{
		String text = message != null ? message : "(null)";
		if( throwable != null )
			text += "\n" + stackTrace( throwable );

		String line = timestamp() + " " + level + "/" + tag + " [" + Thread.currentThread().getName() + "] " + text;

		try
		{
			int priority = "E".equals( level ) ? Log.ERROR : ( "W".equals( level ) ? Log.WARN : Log.INFO );
			Log.println( priority, tag, text );
		}
		catch( Throwable ignored )
		{
		}

		synchronized( LOCK )
		{
			if( logFile == null )
				return;

			try( FileWriter writer = new FileWriter( logFile, true );
				 PrintWriter out = new PrintWriter( writer ) )
			{
				out.println( line );
				out.flush();
			}
			catch( Throwable ignored )
			{
			}
		}
	}

	private static String timestamp()
	{
		return new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss.SSS", Locale.US ).format( new Date() );
	}

	private static String stackTrace( Throwable throwable )
	{
		StringWriter writer = new StringWriter();
		throwable.printStackTrace( new PrintWriter( writer ) );
		return writer.toString();
	}

	private static String versionName( Context context )
	{
		try
		{
			return context.getPackageManager().getPackageInfo( context.getPackageName(), 0 ).versionName;
		}
		catch( Throwable t )
		{
			return "(unknown)";
		}
	}

	private static File chooseDir( Context context )
	{
		try
		{
			SharedPreferences pref = context.getSharedPreferences( "mod", 0 );
			String gamePath = pref.getString( "gamepath", null );
			if( gamePath != null && !gamePath.isEmpty() )
			{
				File dir = new File( gamePath, "csso_logs" );
				if( dir.exists() || dir.mkdirs() )
					return dir;
			}
		}
		catch( Throwable ignored )
		{
		}

		try
		{
			File external = context.getExternalFilesDir( null );
			if( external != null )
			{
				File dir = new File( external, "logs" );
				if( dir.exists() || dir.mkdirs() )
					return dir;
			}
		}
		catch( Throwable ignored )
		{
		}

		File dir = new File( context.getFilesDir(), "logs" );
		if( dir.exists() || dir.mkdirs() )
			return dir;

		return null;
	}
}
