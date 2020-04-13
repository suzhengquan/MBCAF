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
package com.MBCAF.pb.base;

import com.MBCAF.app.manager.IMSeqNumManager;
import com.MBCAF.app.PreDefine;
import com.MBCAF.common.Logger;

public class Header 
{
    private Logger logger = Logger.getLogger(Header.class);

    private int length; // 数据包长度，包括包头
    private char serviceId;
    private char commandId;
    private short seqidx;

    public Header() {
        length = 0;
        serviceId = 0;
        commandId = 0;
        seqidx = 0;
    }

    public Header(int serviceId, int commandId) {
        setServiceId((char)serviceId);
        setMessageID((char)commandId);
        setSeqnum(IMSeqNumManager.getInstance().make());
    }

    public short getSeqnum() {
        return seqidx;
    }

    public void setSeqnum(short seq) {
        this.seqidx = seq;
    }

    public DataBuffer encode() {
        DataBuffer db = new DataBuffer(PreDefine.PROTOCOL_HEADER_LENGTH);
        db.writeInt(length);
        db.writeChar(serviceId);
        db.writeChar(commandId);
        db.writeShort(seqidx);
        return db;
    }

    public void decode(DataBuffer buffer) {
        if (null == buffer)
            return;
        try {
            length = buffer.readInt();
            serviceId = buffer.readChar();
            commandId = buffer.readChar();
            seqidx = buffer.readShort();
            logger.d(
                    "decode header, length:%d,  serviceId:%d, commandId:%d ,seq:%d",
                    length, serviceId, commandId, seqidx);
        } catch (Exception e) {
            logger.e(e.getMessage());
        }
    }

    @Override
    public String toString() {

        return "Header [length=" + length + ", serviceId=" + serviceId + ", commandId="

                + commandId + ", seq=" + seqidx + "]";
    }

    public char getMessageID() {
        return commandId;
    }

    public void setMessageID(char commandID) {
        this.commandId = commandID;
    }

    public char getServiceId() {
        return serviceId;
    }

    public void setServiceId(char serviceID) {
        this.serviceId = serviceID;
    }

    public int getLength() { return length; }

    public void setLength(int length) {
        this.length = length;
    }
}