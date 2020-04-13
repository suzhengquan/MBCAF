
package com.MBCAF.app.manager;

public class IMSeqNumManager {

    private volatile short mSquence = 0;

    private volatile long preMsgId = 0;

    private static IMSeqNumManager maker = new IMSeqNumManager();

    private IMSeqNumManager() {
    }

    public static IMSeqNumManager getInstance() {
        return maker;
    }

    public short make() {
        synchronized (this) {
            mSquence++;
            if (mSquence >= Short.MAX_VALUE)
                mSquence = 1;
        }
        return mSquence;
    }

     public int makelocalUniqueMsgId(){
        synchronized(IMSeqNumManager.this) {
            int timeStamp = (int) (System.currentTimeMillis() % 10000000);
            int localId = timeStamp + 90000000;
            if (localId == preMsgId)
            {
                localId++;
                if (localId >= 100000000) {
                    localId = 90000000;
                }
            }
            preMsgId = localId;
            return localId;
        }
    }

    public boolean isFailure(int msgId){
        if(msgId>=90000000){
            return true;
        }
        return false;
    }
}
