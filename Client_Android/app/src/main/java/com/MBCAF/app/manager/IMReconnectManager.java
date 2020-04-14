package com.MBCAF.app.manager;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;

import com.MBCAF.app.event.CommonEvent;
import com.MBCAF.app.event.CommonEvent;
import com.MBCAF.app.event.CommonEvent;
import com.MBCAF.common.Logger;
import com.MBCAF.common.SysUtil;

import de.greenrobot.event.EventBus;

public class IMReconnectManager extends IMManager {
    private Logger logger = Logger.getLogger(IMReconnectManager.class);

	private static IMReconnectManager inst = new IMReconnectManager();
    public static IMReconnectManager instance() {
       return inst;
    }

    /**重连所处的状态*/
    private volatile CommonEvent status = CommonEvent.CE_Reconnect_None;

    private final int INIT_RECONNECT_INTERVAL_SECONDS = 3;
    private int reconnectInterval = INIT_RECONNECT_INTERVAL_SECONDS;
    private final int MAX_RECONNECT_INTERVAL_SECONDS = 60;

    private final int HANDLER_CHECK_NETWORK = 1;
    private volatile boolean isAlarmTrigger = false;

    /**wakeLock锁*/
    private PowerManager.WakeLock wakeLock;

    /**
     * imService 服务建立的时候
     * 初始化所有的manager 调用onStartIMManager会调用下面的方法
     * eventBus 注册可以放在这里
     */
    @Override
    public void onStart() {
    }

    public void onNormalLoginOk(){
        onLocalLoginOk();
        status = CommonEvent.CE_Reconnect_Success;
    }

    public void onLocalLoginOk(){
       logger.d("reconnect#CommonEvent onLocalLoginOk");

       if(!EventBus.getDefault().isRegistered(inst)){
           EventBus.getDefault().register(inst);
       }

       IntentFilter intentFilter = new IntentFilter();
       intentFilter.addAction(ACTION_RECONNECT);
       intentFilter.addAction(ConnectivityManager.CONNECTIVITY_ACTION);
       logger.d("reconnect#register actions");
       ctx.registerReceiver(imReceiver, intentFilter);
    }

    /** 网络链接成功*/
    public void onLocalNetOk(){
      logger.d("reconnect#onLoginSuccess 网络链接上来,无需其他操作");
      status = CommonEvent.CE_Reconnect_Success;
    }


    @Override
    public void reset() {
        logger.d("reconnect#reset begin");
        try {
            EventBus.getDefault().unregister(inst);
            ctx.unregisterReceiver(imReceiver);
            status = CommonEvent.CE_Reconnect_None;
            isAlarmTrigger = false;
            logger.d("reconnect#reset stop");
        }catch (Exception e){
            logger.e("reconnect#reset error:%s",e.getCause());
        }finally {
            releaseWakeLock();
        }
    }

    public void onEventMainThread(CommonEvent event){
        logger.d("reconnect#CommonEvent event:%s",event.name());
        switch (event){
            case CE_Connect_MsgServerDisConnect:
            case CE_Connect_MsgServerAddrAFail:
            case CE_Connect_MsgServerConnectFail:{
                tryReconnect();
            }break;
            case CE_Login_Fail: {
                tryReconnect();
            }break;
            case CE_Login_MsgService: {
                resetReconnectTime();
                releaseWakeLock();
            }break;
        }
    }

    Handler handler = new Handler(){
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what){
                case HANDLER_CHECK_NETWORK:{
                      if(!SysUtil.isNetWorkAvalible(ctx)){
                          logger.w("reconnect#handleMessage#网络依旧不可用");
                          releaseWakeLock();
                          EventBus.getDefault().post(CommonEvent.CE_Reconnect_Disable);
                      }
                }break;
            }
        }
    };

    private boolean isReconnecting(){
        CommonEvent socketEvent =   IMSocketManager.instance().getSocketStatus();
        CommonEvent loginEvent = IMLoginManager.instance().getLoginStatus();

        if(socketEvent.equals(CommonEvent.CE_Connect_MsgServerConnect)
                || socketEvent.equals(CommonEvent.CE_Connect_MsgServerAddrQ)
                || loginEvent.equals(CommonEvent.CE_Login_In)){
            return true;
        }
        return false;
    }

    /**
     * 可能是网络环境切换
     * 可能是数据包异常
     * 启动快速重连机制
     */
    private  void tryReconnect(){
        /**检测网络状态*/
        ConnectivityManager nw = (ConnectivityManager) ctx.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo netinfo = nw.getActiveNetworkInfo();

        if(netinfo == null){
            // 延迟检测
            logger.w("reconnect#netinfo 为空延迟检测");
            status = CommonEvent.CE_Reconnect_Disable;
            handler.sendEmptyMessageDelayed(HANDLER_CHECK_NETWORK, 2000);
            return ;
        }

        synchronized(IMReconnectManager.this) {
            // 网络状态可用 当前状态处于重连的过程中 可能会死循环嘛
            if (netinfo.isAvailable()) {
                /**网络显示可用*/
                /**判断是否有必要重练*/
                if (status == CommonEvent.CE_Reconnect_None
                        || !IMLoginManager.instance().isEverLogined()
                        || IMLoginManager.instance().isKickout()
                        || IMSocketManager.instance().isSocketConnect()
                        ) {
                    logger.i("reconnect#无需启动重连程序");
                    return;
                }
                if (isReconnecting()) {
                    /**升级时间，部署下一次的重练*/
                    logger.d("reconnect#正在重连中..");
                    incrementReconnectInterval();
                    scheduleReconnect(reconnectInterval);
                    logger.d("reconnect#tryReconnect#下次重练时间间隔:%d", reconnectInterval);
                    return;
                }

                /**确保之前的链接已经关闭*/
                handleReconnectServer();
            } else {
                //通知上层UI修改
                logger.d("reconnect#网络不可用!! 通知上层");
                status = CommonEvent.CE_Reconnect_Disable;
                EventBus.getDefault().post(CommonEvent.CE_Reconnect_Disable);
            }
        }
    }

    /**
     * 部署下次请求的时间
     * 为什么用这种方式，而不是handler?
     * @param seconds
     */
	private void scheduleReconnect(int seconds) {
		logger.d("reconnect#scheduleReconnect after %d seconds", seconds);
		Intent intent = new Intent(ACTION_RECONNECT);
		PendingIntent pi = PendingIntent.getBroadcast(ctx, 0, intent,PendingIntent.FLAG_CANCEL_CURRENT);
		if (pi == null) {
			logger.e("reconnect#pi is null");
			return;
		}
		AlarmManager am = (AlarmManager) ctx.getSystemService(Context.ALARM_SERVICE);
		am.set(AlarmManager.RTC_WAKEUP, System.currentTimeMillis() + seconds
				* 1000, pi);
	}

    private void incrementReconnectInterval() {
		if (reconnectInterval >= MAX_RECONNECT_INTERVAL_SECONDS) {
			reconnectInterval = MAX_RECONNECT_INTERVAL_SECONDS;
		} else {
			reconnectInterval = reconnectInterval * 2;
		}
	}

    private void resetReconnectTime() {
        logger.d("reconnect#resetReconnectTime");
        reconnectInterval = INIT_RECONNECT_INTERVAL_SECONDS;
    }

    /**--------------------boradcast-广播相关-----------------------------*/
    private final String  ACTION_RECONNECT = "com.MBCAF.imlib.action.reconnect";
    private BroadcastReceiver imReceiver = new BroadcastReceiver(){
        @Override
        public void onReceive(Context context, Intent intent) {
                String action = intent.getAction();
                logger.d("reconnect#im#receive action:%s", action);
                onAction(action, intent);
        }
    };

    public void onAction(String action, Intent intent) {
        logger.d("reconnect#onAction action:%s", action);
        if (action.equals(ConnectivityManager.CONNECTIVITY_ACTION)) {
            logger.d("reconnect#onAction#网络状态发生变化!!");
            tryReconnect();
        } else if (action.equals(ACTION_RECONNECT)) {
            isAlarmTrigger = true;
            tryReconnect();
        }
    }

    private void handleReconnectServer() {
        logger.d("reconnect#handleReconnectServer#定时任务触发");
        acquireWakeLock();
        IMSocketManager.instance().disconnectMsgServer();
        if (isAlarmTrigger) {
            isAlarmTrigger = false;
            logger.d("reconnect#定时器触发重连。。。");
            if(reconnectInterval > 24){
                // 重新请求msg地址
                IMLoginManager.instance().relogin();
            }else{
                IMSocketManager.instance().reconnectMsg();
            }
        } else {
            logger.d("reconnect#正常重连，非定时器");
            IMSocketManager.instance().reconnectMsg();
        }
    }

    /**
     * 获取电源锁，保持该服务在屏幕熄灭时仍然获取CPU时，保持运行
     * 由于重连流程中充满了异步操作,按照函数执行流判断加锁代码侵入性较大，而且还需要traceId跟踪（因为可能会并发）
     * 所以这个锁定义为时间锁，15s的重连容忍时间
     */
    private void acquireWakeLock() {
        try {
            if (null == wakeLock) {
                PowerManager pm = (PowerManager) ctx.getSystemService(Context.POWER_SERVICE);
                wakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "teamtalk_reconnecting_wakelock");
                logger.i("acquireWakeLock#call acquireWakeLock");
                wakeLock.acquire(15000);
            }
        }catch (Exception e){
            e.printStackTrace();
        }
    }

    // 释放设备电源锁
    private void releaseWakeLock() {
        try {
            if (null != wakeLock && wakeLock.isHeld()) {
                logger.i("releaseWakeLock##call releaseWakeLock");
                wakeLock.release();
                wakeLock = null;
            }
        }catch (Exception e){
            e.printStackTrace();
        }
    }
}


