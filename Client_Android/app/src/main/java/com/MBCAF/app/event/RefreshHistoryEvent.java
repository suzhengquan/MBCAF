package com.MBCAF.app.event;

import com.MBCAF.db.entity.MessageEntity;

import java.util.List;

public class RefreshHistoryEvent {
   public int pullTimes;
   public int lastMsgId;
   public int count;
   public List<MessageEntity> listMsg;
   public int peerId;
   public int peerType;
   public String sessionKey;

   public RefreshHistoryEvent(){}

}
