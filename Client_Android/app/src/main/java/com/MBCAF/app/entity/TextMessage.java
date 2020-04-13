package com.MBCAF.app.entity;

import com.MBCAF.db.entity.PeerEntity;
import com.MBCAF.db.entity.UserEntity;
import com.MBCAF.app.PreDefine;
import com.MBCAF.db.entity.MessageEntity;
import com.MBCAF.app.manager.IMSeqNumManager;

import java.io.Serializable;
import java.io.UnsupportedEncodingException;

public class TextMessage extends MessageEntity implements Serializable {

     public TextMessage(){
         msgId = IMSeqNumManager.getInstance().makelocalUniqueMsgId();
     }

     private TextMessage(MessageEntity entity){
         /**父类的id*/
         id =  entity.getId();
         msgId  = entity.getMsgId();
         fromId = entity.getFromId();
         toId   = entity.getToId();
         sessionKey = entity.getSessionKey();
         content=entity.getContent();
         msgType=entity.getMsgType();
         displayType=entity.getDisplayType();
         status = entity.getStatus();
         created = entity.getCreated();
         updated = entity.getUpdated();
     }

     public static TextMessage parseFromNet(MessageEntity entity){
         TextMessage textMessage = new TextMessage(entity);
         textMessage.setStatus(PreDefine.MSG_SUCCESS);
         textMessage.setDisplayType(PreDefine.View_Text_Type);
         return textMessage;
     }

    public static TextMessage parseFromDB(MessageEntity entity){
        if(entity.getDisplayType()!=PreDefine.View_Text_Type){
            throw new RuntimeException("#TextMessage# parseFromDB,not View_Text_Type");
        }
        TextMessage textMessage = new TextMessage(entity);
        return textMessage;
    }

    public static TextMessage buildForSend(String content,UserEntity fromUser,PeerEntity peerEntity){
        TextMessage textMessage = new TextMessage();
        int nowTime = (int) (System.currentTimeMillis() / 1000);
        textMessage.setFromId(fromUser.getPeerId());
        textMessage.setToId(peerEntity.getPeerId());
        textMessage.setUpdated(nowTime);
        textMessage.setCreated(nowTime);
        textMessage.setDisplayType(PreDefine.View_Text_Type);
        textMessage.setGIfEmo(true);
        int peerType = peerEntity.getType();
        int msgType = peerType == PreDefine.ST_Group ? PreDefine.MT_GroupText
                : PreDefine.MT_Text;
        textMessage.setMsgType(msgType);
        textMessage.setStatus(PreDefine.MSG_SENDING);
        // 内容的设定
        textMessage.setContent(content);
        textMessage.buildSessionKey(true);
        return textMessage;
    }


    /**
     * Not-null value.
     * DB的时候需要
     */
    @Override
    public String getContent() {
        return content;
    }

    @Override
    public byte[] getSendContent() {
        try {
            /** 加密*/
            String sendContent =new String(com.MBCAF.app.network.Security.getInstance().EncryptMsg(content));
            return sendContent.getBytes("utf-8");
        } catch (UnsupportedEncodingException e) {
            e.printStackTrace();
        }
        return null;
    }
}
