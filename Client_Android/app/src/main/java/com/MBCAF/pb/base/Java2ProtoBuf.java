package com.MBCAF.pb.base;

import com.MBCAF.app.PreDefine;
import com.MBCAF.pb.Proto;

public class Java2ProtoBuf {
    /**----enum 转化接口--*/
    public static Proto.MessageType getProtoMsgType(int msgType){
        switch (msgType){
            case PreDefine.MT_GroupText:
                return Proto.MessageType.MT_GroupText;
            case PreDefine.MT_GroupAudio:
                return Proto.MessageType.MT_GroupAudio;
            case PreDefine.MT_Audio:
                return Proto.MessageType.MT_Audio;
            case PreDefine.MT_Text:
                return Proto.MessageType.MT_Text;
            default:
                throw new IllegalArgumentException("msgType is illegal,cause by #getProtoMsgType#" +msgType);
        }
    }


    public static Proto.SessionType getProtoSessionType(int sessionType){
        switch (sessionType){
            case PreDefine.ST_Single:
                return Proto.SessionType.ST_Single;
            case PreDefine.ST_Group:
                return Proto.SessionType.ST_Group;
            default:
                throw new IllegalArgumentException("sessionType is illegal,cause by #getProtoSessionType#" +sessionType);
        }
    }
}
