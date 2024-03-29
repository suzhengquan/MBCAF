package com.MBCAF.app.manager;

import android.app.NotificationManager;
import android.content.Context;
import android.text.TextUtils;

import com.MBCAF.app.PreDefine;
import com.MBCAF.db.entity.GroupEntity;
import com.MBCAF.db.entity.MessageEntity;
import com.MBCAF.app.entity.UnreadEntity;
import com.MBCAF.app.event.UnreadEvent;
import com.MBCAF.pb.base.EntityChangeEngine;
import com.MBCAF.pb.base.Java2ProtoBuf;
import com.MBCAF.pb.base.ProtoBuf2JavaBean;
import com.MBCAF.pb.Proto;
import com.MBCAF.pb.MsgServer;
import com.MBCAF.common.Logger;

import java.util.List;
import java.util.concurrent.ConcurrentHashMap;

import de.greenrobot.event.EventBus;

public class IMUnreadMsgManager extends IMManager {
    private Logger logger = Logger.getLogger(IMUnreadMsgManager.class);
	private static IMUnreadMsgManager inst = new IMUnreadMsgManager();
	public static IMUnreadMsgManager instance() {
			return inst;
	}

    private IMSocketManager imSocketManager = IMSocketManager.instance();
    private IMLoginManager loginManager =IMLoginManager.instance();

    /**key=> sessionKey*/
    private ConcurrentHashMap<String,UnreadEntity> unreadMsgMap = new ConcurrentHashMap<>();
    private int totalUnreadCount = 0;

    private boolean unreadListReady = false;

    @Override
    public void onStart() {}


    // 未读消息控制器，本地是不存状态的
    public void onNormalLoginOk(){
        unreadMsgMap.clear();
        reqUnreadMsgContactList();
    }

    public void onLocalNetOk(){
        unreadMsgMap.clear();
        reqUnreadMsgContactList();
    }

    @Override
    public void reset() {
        unreadListReady = false;
        unreadMsgMap.clear();
    }

    /**
     * 继承该方法实现自身的事件驱动
     * @param event
     */
    public synchronized void triggerEvent(UnreadEvent event) {
        switch (event.event){
            case UNREAD_MSG_LIST_OK:
                unreadListReady = true;
                break;
        }

        EventBus.getDefault().post(event);
    }

    /**-------------------------------分割线----------------------------------*/
    /**
     * 请求未读消息列表
     */
    private void reqUnreadMsgContactList() {
        logger.i("unread#1reqUnreadMsgContactList");
        int loginId = IMLoginManager.instance().getLoginId();
        MsgServer.MessageUnreadCntQ  unreadMsgCntReq  = MsgServer.MessageUnreadCntQ
                .newBuilder()
                .setUserId(loginId)
                .build();
        int sid = Proto.MsgTypeID.MTID_MsgServer_VALUE;
        int cid = Proto.MsgServerMsgID.MSID_UnReadCountQ_VALUE;
        imSocketManager.sendRequest(unreadMsgCntReq,sid,cid);
    }

    public void onRepUnreadMsgContactList(MsgServer.MessageUnreadCntA unreadMsgCntRsp) {
        logger.i("unread#2onRepUnreadMsgContactList");
        totalUnreadCount = unreadMsgCntRsp.getTotalCnt();
        List<Proto.UnreadInfo> unreadInfoList =  unreadMsgCntRsp.getUnreadinfoListList();
        logger.i("unread#unreadMsgCnt:%d, unreadMsgInfoCnt:%d",unreadInfoList.size(),totalUnreadCount);

        for(Proto.UnreadInfo unreadInfo:unreadInfoList){
            UnreadEntity unreadEntity = ProtoBuf2JavaBean.getUnreadEntity(unreadInfo);
            //屏蔽的设定
            addIsForbidden(unreadEntity);
            unreadMsgMap.put(unreadEntity.getSessionKey(), unreadEntity);
        }
        triggerEvent(new UnreadEvent(UnreadEvent.Event.UNREAD_MSG_LIST_OK));
    }

    /**
     * 回话是否已经被设定为屏蔽
     * @param unreadEntity
     */
    private void addIsForbidden(UnreadEntity unreadEntity){
        if(unreadEntity.getSessionType() == PreDefine.ST_Group){
            GroupEntity groupEntity= IMGroupManager.instance().findGroup(unreadEntity.getPeerId());
            if(groupEntity !=null && groupEntity.getStatus() == PreDefine.Group_State_Shield){
                unreadEntity.setForbidden(true);
            }
        }
    }

    /**设定未读回话为屏蔽回话 仅限于群组 todo*/
    public void setForbidden(String sessionKey,boolean isFor){
       UnreadEntity unreadEntity =  unreadMsgMap.get(sessionKey);
       if(unreadEntity !=null){
           unreadEntity.setForbidden(isFor);
       }
    }

	public void add(MessageEntity msg) {
        //更新session list中的msg信息
        //更新未读消息计数
        if(msg == null){
            logger.d("unread#unreadMgr#add msg is null!");
            return;
        }
        // isFirst场景:出现一条未读消息，出现小红点，需要触发 [免打扰的情况下]
        boolean isFirst = false;
		logger.d("unread#unreadMgr#add unread msg:%s", msg);
        UnreadEntity unreadEntity;
        int loginId = IMLoginManager.instance().getLoginId();
        String sessionKey = msg.getSessionKey();
        boolean isSend = msg.isSend(loginId);
        if(isSend){
            IMNotificationManager.instance().cancelSessionNotifications(sessionKey);
            return;
        }

        if(unreadMsgMap.containsKey(sessionKey)){
            unreadEntity = unreadMsgMap.get(sessionKey);
            // 判断最后一条msgId是否相同
            if(unreadEntity.getLaststMsgId() == msg.getMsgId()){
                return;
            }
            unreadEntity.setUnReadCnt(unreadEntity.getUnReadCnt()+1);
        }else{
            isFirst = true;
            unreadEntity = new UnreadEntity();
            unreadEntity.setUnReadCnt(1);
            unreadEntity.setPeerId(msg.getPeerId(isSend));
            unreadEntity.setSessionType(msg.getSessionType());
            unreadEntity.buildSessionKey();
        }

        unreadEntity.setLatestMsgData(msg.getMessageDisplay());
        unreadEntity.setLaststMsgId(msg.getMsgId());
        addIsForbidden(unreadEntity);

        /**放入manager 状态中*/
        unreadMsgMap.put(unreadEntity.getSessionKey(),unreadEntity);

        /**没有被屏蔽才会发送广播*/
        if(!unreadEntity.isForbidden() || isFirst) {
            UnreadEvent unreadEvent = new UnreadEvent();
            unreadEvent.event = UnreadEvent.Event.UNREAD_MSG_RECEIVED;
            unreadEvent.entity = unreadEntity;
            triggerEvent(unreadEvent);
        }
	}

    public void ackReadMsg(MessageEntity entity){
        logger.d("chat#ackReadMsg -> msg:%s", entity);
        int loginId = loginManager.getLoginId();
        Proto.SessionType sessionType = Java2ProtoBuf.getProtoSessionType(entity.getSessionType());
        MsgServer.MessageReadA readAck = MsgServer.MessageReadA.newBuilder()
                .setMsgId(entity.getMsgId())
                .setSessionId(entity.getPeerId(false))
                .setSessionType(sessionType)
                .setUserId(loginId)
                .build();
        int sid = Proto.MsgTypeID.MTID_MsgServer_VALUE;
        int cid = Proto.MsgServerMsgID.MSID_ReadACK_VALUE;
        imSocketManager.sendRequest(readAck,sid,cid);
    }

    public void ackReadMsg(UnreadEntity unreadEntity){
        logger.d("chat#ackReadMsg -> msg:%s", unreadEntity);
        int loginId = loginManager.getLoginId();
        Proto.SessionType sessionType = Java2ProtoBuf.getProtoSessionType(unreadEntity.getSessionType());
        MsgServer.MessageReadA readAck = MsgServer.MessageReadA.newBuilder()
                .setMsgId(unreadEntity.getLaststMsgId())
                .setSessionId(unreadEntity.getPeerId())
                .setSessionType(sessionType)
                .setUserId(loginId)
                .build();
        int sid = Proto.MsgTypeID.MTID_MsgServer_VALUE;
        int cid = Proto.MsgServerMsgID.MSID_ReadACK_VALUE;
        imSocketManager.sendRequest(readAck,sid,cid);
    }


    /**
     * 服务端主动发送已读通知
     * @param readNotify
     */
    public void onNotifyRead(MsgServer.MessageRead readNotify){
        logger.d("chat#onNotifyRead");
        //发送此信令的用户id
        int trigerId = readNotify.getUserId();
        int loginId = IMLoginManager.instance().getLoginId();
        if(trigerId != loginId){
            logger.i("onNotifyRead# trigerId:%s,loginId:%s not Equal",trigerId,loginId);
            return ;
        }
        //现在的逻辑是msgId之后的 全部都是已读的
        // 不做复杂判断了，简单处理
        int msgId = readNotify.getMsgId();
        int peerId = readNotify.getSessionId();
        int sessionType = ProtoBuf2JavaBean.getJavaSessionType(readNotify.getSessionType());
        String sessionKey = EntityChangeEngine.getSessionKey(peerId,sessionType);

        // 通知栏也要去除掉
        NotificationManager notifyMgr = (NotificationManager) ctx.getSystemService(Context.NOTIFICATION_SERVICE);
        if (notifyMgr == null) {
            return;
        }

        notifyMgr.cancel(IMNotificationManager.instance().getSessionNotificationId(sessionKey));

        UnreadEntity unreadSession =  findUnread(sessionKey);
        if(unreadSession!=null && unreadSession.getLaststMsgId() <= msgId){
            // 清空会话session
            logger.d("chat#onNotifyRead# unreadSession onLoginOut");
            readUnreadSession(sessionKey);
        }
    }

    /**
     * 备注: 先获取最后一条消息
     * 1. 清除回话内的未读计数
     * 2. 发送最后一条msgId的已读确认
     * @param sessionKey
     */
    public void  readUnreadSession(String sessionKey){
        logger.d("unread#readUnreadSession# sessionKey:%s", sessionKey);
         if(unreadMsgMap.containsKey(sessionKey)){
             UnreadEntity entity = unreadMsgMap.remove(sessionKey);
             ackReadMsg(entity);
             triggerEvent(new UnreadEvent(UnreadEvent.Event.SESSION_READED_UNREAD_MSG));
         }
    }


    public UnreadEntity findUnread(String sessionKey){
        logger.d("unread#findUnread# buddyId:%s", sessionKey);
        if(TextUtils.isEmpty(sessionKey) || unreadMsgMap.size()<=0){
            logger.i("unread#findUnread# no unread info");
            return null;
        }
        if(unreadMsgMap.containsKey(sessionKey)){
            return unreadMsgMap.get(sessionKey);
        }
        return null;
    }

    public ConcurrentHashMap<String, UnreadEntity> getUnreadMsgMap() {
        return unreadMsgMap;
    }

    public int getTotalUnreadCount() {
        int count = 0;
        for(UnreadEntity entity:unreadMsgMap.values()){
            if(!entity.isForbidden()){
                count  = count +  entity.getUnReadCnt();
            }
        }
        return count;
    }

    public boolean isUnreadListReady() {
        return unreadListReady;
    }
}
