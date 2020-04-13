/*
Copyright (c) "2018-2019", Shenzhen Mindeng Technology Co., Ltd(www.niiengine.com),
		Mindeng Base Communication Application Framework
All rights reserved.
	Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
	Redistributions of source code must retain the above copyright notice, this list of
conditions and the following disclaimer.
	Redistributions in binary form must reproduce the above copyright notice, this list
of conditions and the following disclaimer in the documentation and/or other materials
provided with the distribution.
	Neither the name of the "ORGANIZATION" nor the names of its contributors may be used
to endorse or promote products derived from this software without specific prior written
permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
package com.MBCAF.app.manager;

import android.os.Handler;

import com.MBCAF.common.Logger;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

public class IMPacketManager
{
    public abstract static class PacketListener
    {
        private long createTime;
        private long timeOut;
        public PacketListener(long timeOut)
        {
            this.timeOut = timeOut;
            long now = System.currentTimeMillis();
            createTime = now;
        }

        public PacketListener()
        {
            this.timeOut = 8*1000;
            long now = System.currentTimeMillis();
            createTime = now;
        }

        public long getCreateTime() {
            return createTime;
        }

        public void setCreateTime(long createTime) {
            this.createTime = createTime;
        }

        public long getTimeOut() {
            return timeOut;
        }

        public void setTimeOut(long timeOut) {
            this.timeOut = timeOut;
        }

        public abstract void onSuccess(Object response);

        public abstract void onFaild();

        public abstract void onTimeout();
    }

    private static IMPacketManager listenerQueue = new IMPacketManager();
    private Logger logger = Logger.getLogger(IMPacketManager.class);
    public static IMPacketManager instance(){
        return listenerQueue;
    }

    private volatile  boolean stopFlag = false;
    private volatile  boolean hasTask = false;

    //callback 队列
    private Map<Integer, PacketListener> callBackQueue = new ConcurrentHashMap<>();
    private Handler timerHandler = new Handler();

    public void onStart()
    {
        logger.d("IMPacketManager#onStart run");
        stopFlag = false;
        startTimer();
    }
    public void onDestory()
    {
        logger.d("IMPacketManager#onDestory ");
        callBackQueue.clear();
        stopTimer();
    }

    //以前是TimerTask处理方式
    private void startTimer()
    {
        if(!stopFlag && hasTask == false)
        {
            hasTask = true;
            timerHandler.postDelayed(new Runnable()
            {
                @Override
                public void run() {
                    timerImpl();
                    hasTask = false;
                    startTimer();
                }
            }, 5 * 1000);
        }
    }

    private void stopTimer(){
        stopFlag = true;
    }

    private void timerImpl()
    {
        long currentRealtime =   System.currentTimeMillis();//SystemClock.elapsedRealtime();

        for (java.util.Map.Entry<Integer, PacketListener> entry : callBackQueue.entrySet()) {

            PacketListener packetlistener = entry.getValue();
            Integer seqNo = entry.getKey();
            long timeRange = currentRealtime - packetlistener.getCreateTime();

            try
            {
                if (timeRange >= packetlistener.getTimeOut()) {
                    logger.d("IMPacketManager#find timeout msg");
                    PacketListener listener = pop(seqNo);
                    if (listener != null) {
                        listener.onTimeout();
                    }
                }
            }
            catch (Exception e)
            {
                logger.d("IMPacketManager#timerImpl onTimeout is Error,exception is %s", e.getCause());
            }
        }
    }

    public void push(int seqNo,PacketListener packetlistener){
        if(seqNo <=0 || null==packetlistener){
            logger.d("IMPacketManager#push error, cause by Illegal params");
            return;
        }
        callBackQueue.put(seqNo,packetlistener);
    }


    public PacketListener pop(int seqNo)
    {
        synchronized (IMPacketManager.this)
        {
            if (callBackQueue.containsKey(seqNo))
            {
                PacketListener packetlistener = callBackQueue.remove(seqNo);
                return packetlistener;
            }
            return null;
        }
    }
}
