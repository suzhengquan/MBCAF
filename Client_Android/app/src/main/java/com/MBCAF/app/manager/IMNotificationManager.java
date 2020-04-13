package com.MBCAF.app.manager;

import android.annotation.SuppressLint;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.support.v4.app.NotificationCompat;
import android.support.v4.app.NotificationCompat.Builder;
import android.app.NotificationChannel;
import android.view.View;
import android.os.Build;

import com.MBCAF.app.ui.activity.MainActivity;
import com.MBCAF.db.entity.GroupEntity;
import com.MBCAF.db.entity.UserEntity;
import com.MBCAF.db.sp.ConfigurationSp;
import com.MBCAF.R;
import com.MBCAF.app.PreDefine;
import com.MBCAF.app.entity.UnreadEntity;
import com.MBCAF.app.event.GroupEvent;
import com.MBCAF.app.event.UnreadEvent;
import com.MBCAF.app.ui.activity.MessageActivity;
import com.MBCAF.common.IMUIHelper;
import com.MBCAF.common.Logger;
import com.nostra13.universalimageloader.core.ImageLoader;
import com.nostra13.universalimageloader.core.assist.FailReason;
import com.nostra13.universalimageloader.core.assist.ImageSize;
import com.nostra13.universalimageloader.core.listener.SimpleImageLoadingListener;

import de.greenrobot.event.EventBus;

public class IMNotificationManager extends IMManager{

    private Logger logger = Logger.getLogger(IMNotificationManager.class);
	private static IMNotificationManager inst = new IMNotificationManager();
	public static IMNotificationManager instance() {
			return inst;
	}
    private ConfigurationSp configurationSp;

	private IMNotificationManager() {
	}

    @SuppressLint("WrongConstant")
    public void startFG(Service s) {
        NotificationManager notifyMgr = (NotificationManager) s.getSystemService(Context.NOTIFICATION_SERVICE);
        if (notifyMgr == null) {
            return;
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O ) {
            int notifyid = (int)System.currentTimeMillis();
            String silentchid = "com.MBCAF.gsgs";
            String silentchname = "GSGSch0";
            NotificationChannel silentch = new NotificationChannel(silentchid, silentchname, NotificationManager.IMPORTANCE_NONE);
            silentch.setLockscreenVisibility(Notification.VISIBILITY_SECRET);
            silentch.enableLights(false);
            silentch.setDescription("gsgs");
            silentch.setName("gsgs");
            silentch.setSound(null, null);
            silentch.enableVibration(false);
            notifyMgr.createNotificationChannel(silentch);

            //Intent nfIntent = new Intent(s, MainActivity.class);
            //PendingIntent pendingIntent = PendingIntent.getActivity(s, notifyid, nfIntent, 0);

            Builder builder = new NotificationCompat.Builder(s, silentchid);
            builder.setContentTitle("gsgs");
            builder.setContentText("");
            builder.setDefaults(Notification.DEFAULT_ALL);
            builder.setAutoCancel(true);
            builder.setShowWhen(true);
            builder.setSmallIcon(R.drawable.tt_small_icon);
            //builder.setContentIntent(pendingIntent);
            s.startForeground((int) System.currentTimeMillis(), builder.build());
        }
    }

    public void stopFG(Service s){
	    s.stopForeground(false);
    }
    @Override
    public void doOnStart() {
        cancelAllNotifications();
    }

    public void onLoginSuccess(){
        int loginId = IMLoginManager.instance().getLoginId();
        configurationSp = ConfigurationSp.instance(ctx,loginId);
        if(!EventBus.getDefault().isRegistered(inst)){
            EventBus.getDefault().register(inst);
        }
    }

    public void reset() {
        EventBus.getDefault().unregister(this);
        cancelAllNotifications();
    }


    public void onEventMainThread(UnreadEvent event){
        switch (event.event){
            case UNREAD_MSG_RECEIVED:
                UnreadEntity unreadEntity = event.entity;
                handleMsgRecv(unreadEntity);
                break;
        }
    }

    // 屏蔽群，相关的通知全部删除
    public void onEventMainThread(GroupEvent event){
        GroupEntity gEntity = event.getGroupEntity();
        if(event.getEvent()== GroupEvent.Event.SHIELD_GROUP_OK){
            if(gEntity == null){
                return;
            }
            cancelSessionNotifications(gEntity.getSessionKey());
        }
    }

    private void handleMsgRecv(UnreadEntity entity) {
        logger.d("notification#recv unhandled message");
        int peerId = entity.getPeerId();
        int sessionType =  entity.getSessionType();
        logger.d("notification#msg no one handled, peerId:%d, sessionType:%d", peerId, sessionType);

        //判断是否设定了免打扰
        if(entity.isForbidden()){
               logger.d("notification#Group_State_Shield");
               return;
        }

        //PC端是否登陆 取消 【暂时先关闭】
//        if(IMLoginManager.instance().isPcOnline()){
//            logger.d("notification#isPcOnline");
//            return;
//        }

        // 全局开关
        boolean  globallyOnOff = configurationSp.getCfg(PreDefine.SETTING_GLOBAL,ConfigurationSp.CfgDimension.NOTIFICATION);
        if (globallyOnOff) {
            logger.d("notification#shouldGloballyShowNotification is false, return");
            return;
        }

        // 单独的设置
        boolean singleOnOff = configurationSp.getCfg(entity.getSessionKey(),ConfigurationSp.CfgDimension.NOTIFICATION);
        if (singleOnOff) {
            logger.d("notification#shouldShowNotificationBySession is false, return");
            return;
        }

        //if the message is a multi login message which send from another terminal,not need notificate to status bar
        // 判断是否是自己的消息
        if(IMLoginManager.instance().getLoginId() != peerId){
             showNotification(entity);
        }
    }


	public void cancelAllNotifications() {
		logger.d("notification#cancelAllNotifications");
        if(null == ctx){
            return;
        }
		NotificationManager notifyMgr = (NotificationManager) ctx.getSystemService(Context.NOTIFICATION_SERVICE);
		if (notifyMgr == null) {
			return;
		}
		notifyMgr.cancelAll();
	}


    /**
     * 在通知栏中删除特定回话的状态
     * @param sessionKey
     */
    public void cancelSessionNotifications(String sessionKey) {
        logger.d("notification#cancelSessionNotifications");
        NotificationManager notifyMgr = (NotificationManager) ctx.getSystemService(Context.NOTIFICATION_SERVICE);
        if (null == notifyMgr) {
            return;
        }
        int notificationId = getSessionNotificationId(sessionKey);
        notifyMgr.cancel(notificationId);
    }


	private void showNotification(final UnreadEntity unreadEntity) {
		// todo eric need to set the exact size of the big icon
        // 服务端有些特定的支持 尺寸是不是要调整一下 todo 100*100  下面的就可以不要了
		ImageSize targetSize = new ImageSize(80, 80);
        int peerId = unreadEntity.getPeerId();
        int sessionType = unreadEntity.getSessionType();
        String avatarUrl = "";
        String title = "";
        String content = unreadEntity.getLatestMsgData();
        String unit = ctx.getString(R.string.t087);
        int totalUnread = unreadEntity.getUnReadCnt();

        if(unreadEntity.getSessionType() == PreDefine.ST_Single){
            UserEntity contact = IMContactManager.instance().findContact(peerId);
            if(contact !=null){
                title = contact.getMainName();
                avatarUrl = contact.getAvatar();
            }else{
                title = "User_"+peerId;
                avatarUrl = "";
            }

        }else{
            GroupEntity group = IMGroupManager.instance().findGroup(peerId);
            if(group !=null){
                title = group.getMainName();
                avatarUrl = group.getAvatar();
            }else{
                title = "Group_"+peerId;
                avatarUrl = "";
            }
        }
        //获取头像
		avatarUrl = IMUIHelper.getRealAvatarUrl(avatarUrl);
        final String ticker = String.format("[%d%s]%s: %s", totalUnread, unit, title, content);
        final int notificationId = getSessionNotificationId(unreadEntity.getSessionKey());
        final Intent intent = new Intent(ctx, MessageActivity.class);
        intent.putExtra(PreDefine.KEY_SESSION_KEY, unreadEntity.getSessionKey());

        logger.d("notification#notification avatarUrl:%s", avatarUrl);
        final String finalTitle = title;
        ImageLoader.getInstance().loadImage(avatarUrl, targetSize, null, new SimpleImageLoadingListener() {

            @Override
            public void onLoadingComplete(String imageUri, View view,
                                          Bitmap loadedImage) {
                logger.d("notification#icon onLoadComplete");
                // holder.image.setImageBitmap(loadedImage);
                showInNotificationBar(finalTitle,ticker,loadedImage,notificationId,intent);
            }

            @Override
            public void onLoadingFailed(String imageUri, View view,
                                        FailReason failReason) {
                logger.d("notification#icon onLoadFailed");
                // 服务器支持的格式有哪些
                // todo eric default avatar is too small, need big size(128 * 128)
                Bitmap defaultBitmap = BitmapFactory.decodeResource(ctx.getResources(), IMUIHelper.getDefaultAvatarResId(unreadEntity.getSessionType()));
                showInNotificationBar(finalTitle,ticker,defaultBitmap,notificationId,intent);
            }
        });
	}

    @SuppressLint("WrongConstant")
    private void showInNotificationBar(String title, String ticker, Bitmap iconBitmap, int notificationId, Intent intent) {
		logger.d("notification#showInNotificationBar title:%s ticker:%s",title,ticker);

		NotificationManager notifyMgr = (NotificationManager) ctx.getSystemService(Context.NOTIFICATION_SERVICE);
		if (notifyMgr == null) {
			return;
		}

        String CHANNEL_ID = "com.MBCAF.gsgs";
        String CHANNEL_NAME = "GSGSch1";
        NotificationChannel nCh = null;
        if(Build.VERSION.SDK_INT>=Build.VERSION_CODES.O){
            nCh = new NotificationChannel(CHANNEL_ID, CHANNEL_NAME, NotificationManager.IMPORTANCE_HIGH);
            nCh.enableLights(true);//如果使用中的设备支持通知灯，则说明此通知通道是否应显示灯
            nCh.setShowBadge(false);//是否显示角标
            nCh.setLockscreenVisibility(Notification.VISIBILITY_SECRET);
            notifyMgr.createNotificationChannel(nCh);
        }

        // if MessageActivity is in the background, the system would bring it to
        // the front
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        PendingIntent pendingIntent = PendingIntent.getActivity(ctx, notificationId, intent, PendingIntent.FLAG_UPDATE_CURRENT);

        Builder builder = new NotificationCompat.Builder(ctx, CHANNEL_ID).
                setContentTitle(title).
                setContentText(ticker).
                setTicker(ticker).
                setWhen(System.currentTimeMillis()).
                setSmallIcon(R.drawable.tt_small_icon).
                setAutoCancel(true).
                setContentIntent(pendingIntent);

		// this is the content near the right bottom side
		// builder.setContentInfo("content info");

		if (configurationSp.getCfg(PreDefine.SETTING_GLOBAL,ConfigurationSp.CfgDimension.VIBRATION)) {
			// delay 0ms, vibrate 200ms, delay 250ms, vibrate 200ms
			long[] vibrate = {0, 200, 250, 200};
			builder.setVibrate(vibrate);
		} else {
			logger.d("notification#setting is not using vibration");
		}

		// sound
		if (configurationSp.getCfg(PreDefine.SETTING_GLOBAL,ConfigurationSp.CfgDimension.SOUND)) {
			builder.setDefaults(Notification.DEFAULT_SOUND);
		} else {
			logger.d("notification#setting is not using sound");
		}
		if (iconBitmap != null) {
			logger.d("notification#fetch icon from network ok");
			builder.setLargeIcon(iconBitmap);
		}

		Notification notification = builder.build();
		notifyMgr.notify(notificationId, notification);
	}

	// come from
	// http://www.partow.net/programming/hashfunctions/index.html#BKDRHashFunction
	private long hashBKDR(String str) {
		long seed = 131; // 31 131 1313 13131 131313 etc..
		long hash = 0;

		for (int i = 0; i < str.length(); i++) {
			hash = (hash * seed) + str.charAt(i);
		}
		return hash;
	}

	/* End Of BKDR Hash Function */
	public int getSessionNotificationId(String sessionKey) {
		logger.d("notification#getSessionNotificationId sessionTag:%s", sessionKey);
		int hashedNotificationId = (int) hashBKDR(sessionKey);
		logger.d("notification#hashedNotificationId:%d", hashedNotificationId);
		return hashedNotificationId;
	}
}
