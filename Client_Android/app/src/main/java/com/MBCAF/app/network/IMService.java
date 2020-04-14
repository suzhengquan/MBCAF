package com.MBCAF.app.network;

import android.app.Notification;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;

import com.MBCAF.db.DBInterface;
import com.MBCAF.db.entity.MessageEntity;
import com.MBCAF.db.sp.ConfigurationSp;
import com.MBCAF.db.sp.LoginSp;
import com.MBCAF.app.PreDefine;
import com.MBCAF.app.event.CommonEvent;
import com.MBCAF.app.event.PriorityEvent;
import com.MBCAF.app.manager.IMContactManager;
import com.MBCAF.app.manager.IMGroupManager;
import com.MBCAF.app.manager.IMHeartBeatManager;
import com.MBCAF.app.manager.IMLoginManager;
import com.MBCAF.app.manager.IMMessageManager;
import com.MBCAF.app.manager.IMNotificationManager;
import com.MBCAF.app.manager.IMReconnectManager;
import com.MBCAF.app.manager.IMSessionManager;
import com.MBCAF.app.manager.IMSocketManager;
import com.MBCAF.app.manager.IMUnreadMsgManager;
import com.MBCAF.common.ImageUtil;
import com.MBCAF.common.Logger;

import de.greenrobot.event.EventBus;

public class IMService extends Service {
	private Logger logger = Logger.getLogger(IMService.class);

    /**binder*/
	private IMServiceBinder binder = new IMServiceBinder();
    public class IMServiceBinder extends Binder {
        public IMService getService() {
            return IMService.this;
        }
    }

    @Override
    public IBinder onBind(Intent arg0) {
        logger.i("IMService onBind");
        return binder;
    }

	//所有的管理类
    private IMSocketManager socketMgr = IMSocketManager.instance();
	private IMLoginManager loginMgr = IMLoginManager.instance();
	private IMContactManager contactMgr = IMContactManager.instance();
	private IMGroupManager groupMgr = IMGroupManager.instance();
	private IMMessageManager messageMgr = IMMessageManager.instance();
	private IMSessionManager sessionMgr = IMSessionManager.instance();
	private IMReconnectManager reconnectMgr = IMReconnectManager.instance();
	private IMUnreadMsgManager unReadMsgMgr = IMUnreadMsgManager.instance();
	private IMNotificationManager notificationMgr = IMNotificationManager.instance();
    private IMHeartBeatManager heartBeatManager = IMHeartBeatManager.instance();

	private ConfigurationSp configSp;
    private LoginSp loginSp = LoginSp.instance();
    private DBInterface dbInterface = DBInterface.instance();

	@Override
	public void onCreate() {
		logger.i("IMService onCreate");
		super.onCreate();
        EventBus.getDefault().register(this, PreDefine.SERVICE_EVENTBUS_PRIORITY);
        IMNotificationManager.instance().startFG(this);
	}

	@Override
	public void onDestroy() {
		logger.i("IMService onDestroy");
        // todo 在onCreate中使用startForeground
        // 在这个地方是否执行 stopForeground呐
        EventBus.getDefault().unregister(this);
        handleLoginout();
        // DB的资源的释放
        dbInterface.close();
        IMNotificationManager.instance().stopFG(this);
        IMNotificationManager.instance().cancelAllNotifications();
		super.onDestroy();
	}

    /**收到消息需要上层的activity判断 {MessageActicity onEvent(PriorityEvent event)}，这个地方是特殊分支*/
    public void onEvent(PriorityEvent event){
        switch (event.event){
            case MSG_RECEIVED_MESSAGE:{
                MessageEntity entity = (MessageEntity) event.object;
                /**非当前的会话*/
                logger.d("messageactivity#not this session msg -> id:%s", entity.getFromId());
                messageMgr.ackReceiveMsg(entity);
                unReadMsgMgr.add(entity);
                }break;
        }
    }

    // EventBus 事件驱动
    public void onEvent(CommonEvent event){
       switch (event){
           case CE_Login_OK:
               onNormalLoginOk();break;
           case CE_Login_Success:
               onLocalLoginOk();
               break;
           case  CE_Login_MsgService:
               onLocalNetOk();
               break;
           case CE_Login_Out:
               handleLoginout();break;
       }
    }

    // 负责初始化 每个manager
	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		logger.i("IMService onStartCommand");

		Context ctx = getApplicationContext();
        loginSp.init(ctx);

        socketMgr.setup(ctx);
        loginMgr.setup(ctx);
        contactMgr.setup(ctx);
        messageMgr.setup(ctx);
        groupMgr.setup(ctx);
        sessionMgr.setup(ctx);
        unReadMsgMgr.setup(ctx);
        notificationMgr.setup(ctx);
        reconnectMgr.setup(ctx);
        heartBeatManager.setup(ctx);

        ImageUtil.initImageLoaderConfig(ctx);
		return START_STICKY;
	}


    /**
     * 用户输入登陆流程
     * userName/pwd -> reqMessage ->connect -> loginMessage ->loginSuccess
     */
    private void onNormalLoginOk() {
        logger.d("imservice#onLogin Successful");

        Context ctx = getApplicationContext();
        int loginId =  loginMgr.getLoginId();
        configSp = ConfigurationSp.instance(ctx,loginId);
        dbInterface.initDbHelp(ctx,loginId);

        contactMgr.onNormalLoginOk();
        sessionMgr.onNormalLoginOk();
        groupMgr.onNormalLoginOk();
        unReadMsgMgr.onNormalLoginOk();

        reconnectMgr.onNormalLoginOk();
        //依赖的状态比较特殊
        messageMgr.onLoginSuccess();
        notificationMgr.onLoginSuccess();
        heartBeatManager.onloginNetSuccess();
        // 这个时候loginManage中的localLogin 被置为true
    }


    /**
     * 自动登陆/离线登陆成功
     * autoLogin -> DB(loginInfo,loginId...) -> loginSucsess
     */
    private void onLocalLoginOk(){
        Context ctx = getApplicationContext();
        int loginId =  loginMgr.getLoginId();
        configSp = ConfigurationSp.instance(ctx,loginId);
        dbInterface.initDbHelp(ctx,loginId);

        contactMgr.onLocalLoginOk();
        groupMgr.onLocalLoginOk();
        sessionMgr.onLocalLoginOk();
        reconnectMgr.onLocalLoginOk();
        notificationMgr.onLoginSuccess();
        messageMgr.onLoginSuccess();
    }

    /**
     * 1.从本机加载成功之后，请求MessageService建立链接成功(loginMessageSuccess)
     * 2. 重练成功之后
     */
    private void onLocalNetOk(){
        /**为了防止逗比直接把loginId与userName的对应直接改了,重刷一遍*/
        Context ctx = getApplicationContext();
        int loginId =  loginMgr.getLoginId();
        configSp = ConfigurationSp.instance(ctx,loginId);
        dbInterface.initDbHelp(ctx,loginId);

        contactMgr.onLocalNetOk();
        groupMgr.onLocalNetOk();
        sessionMgr.onLocalNetOk();
        unReadMsgMgr.onLocalNetOk();
        reconnectMgr.onLocalNetOk();
        heartBeatManager.onloginNetSuccess();
    }

	private void handleLoginout() {
		logger.d("imservice#handleLoginout");

        // login需要监听socket的变化,在这个地方不能释放，设计上的不合理?
        socketMgr.reset();
        loginMgr.reset();
        contactMgr.reset();
        messageMgr.reset();
        groupMgr.reset();
        sessionMgr.reset();
        unReadMsgMgr.reset();
        notificationMgr.reset();
        reconnectMgr.reset();
        heartBeatManager.reset();
        configSp = null;
        EventBus.getDefault().removeAllStickyEvents();
	}

    @Override
    public void onTaskRemoved(Intent rootIntent) {
        logger.d("imservice#onTaskRemoved");
        // super.onTaskRemoved(rootIntent);
        this.stopSelf();
    }

    /**-----------------get/set 的实体定义---------------------*/
    public IMLoginManager getLoginManager() {
        return loginMgr;
    }

    public IMContactManager getContactManager() {
        return contactMgr;
    }

    public IMMessageManager getMessageManager() {
        return messageMgr;
    }

    public IMGroupManager getGroupManager() {
        return groupMgr;
    }

    public IMSessionManager getSessionManager() {
        return sessionMgr;
    }

    public IMReconnectManager getReconnectManager() {
        return reconnectMgr;
    }

    public IMUnreadMsgManager getUnReadMsgManager() {
        return unReadMsgMgr;
    }

    public IMNotificationManager getNotificationManager() {
        return notificationMgr;
    }

    public DBInterface getDbInterface() {
        return dbInterface;
    }

    public ConfigurationSp getConfigSp() {
        return configSp;
    }

    public LoginSp getLoginSp() {
        return loginSp;
    }

}
