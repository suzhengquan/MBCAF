package com.MBCAF.app.entity;

import android.text.TextUtils;

import com.MBCAF.db.entity.MessageEntity;
import com.MBCAF.app.PreDefine;
import com.MBCAF.pb.base.ProtoBuf2JavaBean;
import com.MBCAF.pb.Proto;

import org.json.JSONException;

import java.util.ArrayList;
import java.util.List;

public class MsgAnalyzeEngine {
    public static String analyzeMessageDisplay(String content){
        String finalRes = content;
        String originContent = content;
        while (!originContent.isEmpty()) {
            int nStart = originContent.indexOf(PreDefine.Image_Magic_Begin);
            if (nStart < 0) {// 没有头
                break;
            } else {
                String subContentString = originContent.substring(nStart);
                int nEnd = subContentString.indexOf(PreDefine.Image_Magic_End);
                if (nEnd < 0) {// 没有尾
                    String strSplitString = originContent;
                    break;
                } else {// 匹配到
                    String pre = originContent.substring(0, nStart);

                    originContent = subContentString.substring(nEnd
                            + PreDefine.Image_Magic_End.length());

                    if(!TextUtils.isEmpty(pre) || !TextUtils.isEmpty(originContent)){
                        finalRes = PreDefine.DISPLAY_FOR_MIX;
                    }else{
                        finalRes = PreDefine.DISPLAY_FOR_IMAGE;
                    }
                }
            }
        }
        return finalRes;
    }

    // 抽离放在同一的地方
    public static MessageEntity analyzeMessage(Proto.MsgInfo msgInfo) {
       MessageEntity messageEntity = new MessageEntity();

       messageEntity.setCreated(msgInfo.getCreateTime());
       messageEntity.setUpdated(msgInfo.getCreateTime());
       messageEntity.setFromId(msgInfo.getFromSessionId());
       messageEntity.setMsgId(msgInfo.getMsgId());
       messageEntity.setMsgType(ProtoBuf2JavaBean.getJavaMsgType(msgInfo.getMsgType()));
       messageEntity.setStatus(PreDefine.MSG_SUCCESS);
       messageEntity.setContent(msgInfo.getMsgData().toStringUtf8());
        /**
         * 解密文本信息
         */
       String desMessage = new String(com.MBCAF.app.network.Security.getInstance().DecryptMsg(msgInfo.getMsgData().toStringUtf8()));
       messageEntity.setContent(desMessage);

       // 文本信息不为空
       if(!TextUtils.isEmpty(desMessage)){
           List<MessageEntity> msgList =  textDecode(messageEntity);
           if(msgList.size()>1){
               // 混合消息
               MixMessage mixMessage = new MixMessage(msgList);
               return mixMessage;
           }else if(msgList.size() == 0){
              // 可能解析失败 默认返回文本消息
              return TextMessage.parseFromNet(messageEntity);
           }else{
               //简单消息，返回第一个
               return msgList.get(0);
           }
       }else{
           // 如果为空
           return TextMessage.parseFromNet(messageEntity);
       }
    }

    private static List<MessageEntity> textDecode(MessageEntity msg){
        List<MessageEntity> msgList = new ArrayList<>();

        String originContent = msg.getContent();
        while (!TextUtils.isEmpty(originContent)) {
            int nStart = originContent.indexOf(PreDefine.Image_Magic_Begin);
            if (nStart < 0) {// 没有头
                String strSplitString = originContent;

                MessageEntity entity = addMessage(msg, strSplitString);
                if(entity!=null){
                    msgList.add(entity);
                }

                originContent = "";
            } else {
                String subContentString = originContent.substring(nStart);
                int nEnd = subContentString.indexOf(PreDefine.Image_Magic_End);
                if (nEnd < 0) {// 没有尾
                    String strSplitString = originContent;


                    MessageEntity entity = addMessage(msg,strSplitString);
                    if(entity!=null){
                        msgList.add(entity);
                    }

                    originContent = "";
                } else {// 匹配到
                    String pre = originContent.substring(0, nStart);
                    MessageEntity entity1 = addMessage(msg,pre);
                    if(entity1!=null){
                        msgList.add(entity1);
                    }

                    String matchString = subContentString.substring(0, nEnd
                            + PreDefine.Image_Magic_End.length());

                    MessageEntity entity2 = addMessage(msg,matchString);
                    if(entity2!=null){
                        msgList.add(entity2);
                    }

                    originContent = subContentString.substring(nEnd
                            + PreDefine.Image_Magic_End.length());
                }
            }
        }

        return msgList;
    }

    public static MessageEntity addMessage(MessageEntity msg,String strContent) {
        if (TextUtils.isEmpty(strContent.trim())){
            return null;
        }
        msg.setContent(strContent);

        if (strContent.startsWith(PreDefine.Image_Magic_Begin)
                && strContent.endsWith(PreDefine.Image_Magic_End)) {
            try {
                ImageMessage imageMessage =  ImageMessage.parseFromNet(msg);
                return imageMessage;
            } catch (JSONException e) {
                // e.printStackTrace();
                return null;
            }
        } else {
           return TextMessage.parseFromNet(msg);
        }
    }

}
