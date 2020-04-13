package com.MBCAF.common;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.Environment;
import android.telephony.TelephonyManager;
import java.lang.reflect.Field;
import java.net.URLEncoder;

public class SysUtil{
	public static final int NONETWORK = 0;
	public static final int WIFI = 1;
	public static final int NOWIFI = 2;

	private String mDeviceId;
	private String mImie;
	
	@SuppressWarnings("deprecation")
	public static final String INFO = URLEncoder.encode(Build.MODEL);
	@SuppressWarnings("deprecation")
	public static String M_SYS = URLEncoder.encode(Build.VERSION.RELEASE);

	private static String DEFAULT_STRING = "mgj_2012";

	public static final int ONE_MINUTE = 1000 * 60;
	public static final int ONE_HOUR =  60 * ONE_MINUTE;
	public static final int ONE_DAY = 24 * ONE_HOUR;

	private Context mCtx;
	private static SysUtil mScreenTools;

	public static SysUtil instance(Context ctx){
		if(null == mScreenTools){
			mScreenTools = new SysUtil(ctx);
		}
		return mScreenTools;
	}

	private SysUtil(Context ctx){
		mCtx = ctx.getApplicationContext();
	}

	public int getScreenWidth(){
		return mCtx.getResources().getDisplayMetrics().widthPixels;
	}

	public int dip2px(int dip){
		float density = getDensity(mCtx);
		return (int)(dip * density + 0.5);
	}

	public int px2dip(int px){
		float density = getDensity(mCtx);
		return (int)((px - 0.5) / density);
	}

	private  float getDensity(Context ctx) { return ctx.getResources().getDisplayMetrics().density; }

	public int getScal(){
		return (int)(getScreenWidth() * 100 / 480);
	}

	public int get480Height(int height480){
		int width = getScreenWidth();
		return (height480 * width / 480);
	}

	public int getStatusBarHeight(){
		Class<?> c = null;
		Object obj = null;
		Field field = null;
		int x = 0, sbar = 0;
		try {
			c = Class.forName("com.android.internal.R$dimen");
			obj = c.newInstance();
			field = c.getField("status_bar_height");
			x = Integer.parseInt(field.get(obj).toString());
			sbar = mCtx.getResources().getDimensionPixelSize(x);
		} catch (Exception e1) {
			e1.printStackTrace();
		}
		return sbar;
	}

	public int getScreenHeight(){
		return mCtx.getResources().getDisplayMetrics().heightPixels;
	}

	public static  int getIntervalHour(long time){
		return (int) (time / ONE_HOUR);
	}

	public static int getIntervalMinute(long time){
		return (int) (time % ONE_HOUR / ONE_MINUTE);
	}

	public static int getIntervalSecond(long time){ return (int)(time % ONE_HOUR % ONE_MINUTE / 1000); }

	public static long getCurrentTime(long serverTimeDiff){ return (int)(System.currentTimeMillis() - serverTimeDiff); }

	public static int getNetWorkType(Context context) {
		if (!isNetWorkAvalible(context)) {
			return SysUtil.NONETWORK;
		}
		ConnectivityManager cm = (ConnectivityManager) context
				.getSystemService(Context.CONNECTIVITY_SERVICE);
		// cm.getNetworkInfo(ConnectivityManager.TYPE_MOBILE);
		if (cm.getNetworkInfo(ConnectivityManager.TYPE_WIFI).isConnectedOrConnecting())
			return SysUtil.WIFI;
		else
			return SysUtil.NOWIFI;
	}

	public static boolean isNetWorkAvalible(Context context) {
		ConnectivityManager cm = (ConnectivityManager) context
				.getSystemService(Context.CONNECTIVITY_SERVICE);
		if (cm == null) {
			return false;
		}
		NetworkInfo ni = cm.getActiveNetworkInfo();
		if (ni == null || !ni.isAvailable()) {
			return false;
		}
		return true;
	}

	@SuppressWarnings("deprecation")
	public String getDeviceId(Context ctx){
		if(null != mDeviceId && mDeviceId.length() > 0){
			return mDeviceId;
		}
		TelephonyManager tm = (TelephonyManager)ctx.getSystemService(Context.TELEPHONY_SERVICE);  
		String deviceId = tm.getDeviceId();
		if(null != deviceId && deviceId.length() > 0) {
			deviceId = URLEncoder.encode(deviceId);
		}else {
			//没取到-取mac地址-
			String mac = getMacAddress(ctx);
			if(null != mac && mac.length() > 0){
				deviceId = "mac" + mac;
			}else{
				//取不到-给个默认值-
				deviceId = DEFAULT_STRING;
			}
		}
		mDeviceId = deviceId;
		return deviceId;
	}
	
	private String getMacAddress(Context ctx){
		WifiManager wifiManager = (WifiManager) ctx.getSystemService(Context.WIFI_SERVICE);
		WifiInfo wifiInfo = wifiManager.getConnectionInfo();
		if(null == wifiInfo || null == wifiInfo.getMacAddress()){
			return "";
		}
		return wifiInfo.getMacAddress().replaceAll(":", "");
	}

	public String getImie(Context ctx){
		if(null != mImie && mImie.length() > 0){
			return mImie;
		}
		TelephonyManager tm = (TelephonyManager)ctx.getSystemService(Context.TELEPHONY_SERVICE);  
		String scriber = tm.getSubscriberId();
		if(null != scriber){
			scriber = URLEncoder.encode(scriber);
		}else {
			scriber = DEFAULT_STRING;
		}
		return scriber;
	}
	
	public static boolean isWifi(Context ctx){
		ConnectivityManager manager = (ConnectivityManager) ctx
				.getSystemService(Context.CONNECTIVITY_SERVICE);
		NetworkInfo networkinfo = manager.getActiveNetworkInfo();
		if(null == networkinfo){
			return true;
		}
		if(ConnectivityManager.TYPE_WIFI == networkinfo.getType()){
			return true;
		}
		return false;
	}

	public static boolean isSDCardExist(){
		return Environment.getExternalStorageState().equals(
			android.os.Environment.MEDIA_MOUNTED);
	}
}