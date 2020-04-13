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
package com.MBCAF;
import de.greenrobot.daogenerator.DaoGenerator;
import de.greenrobot.daogenerator.Entity;
import de.greenrobot.daogenerator.Index;
import de.greenrobot.daogenerator.Property;
import de.greenrobot.daogenerator.Schema;

/**
 */
public class BaseDaoGen
{
    private static String entityPath = "com.MBCAF.db.entity";

    public static void main(String[] args) throws Exception
    {
        int dbVersion = 91;
        Schema schema = new Schema(dbVersion, "com.MBCAF.db.dao");

        schema.enableKeepSectionsByDefault();
        addDepartment(schema);
        addUserInfo(schema);
        addGroupInfo(schema);
        addMessage(schema);
        addSessionInfo(schema);

        String path = "/Users/software/IM/gsgs/app/src/main/java";
        new DaoGenerator().generateAll(schema, path);
    }

    private static void addDepartment(Schema schema)
    {
        Entity info = schema.addEntity("DepartmentEntity");
        info.setTableName("Department");
        info.setClassNameDao("DepartmentDao");
        info.setJavaPackage(entityPath);

        info.addIdProperty().autoincrement();
        info.addIntProperty("departId").unique().notNull().index();
        info.addStringProperty("departName").unique().notNull().index();
        info.addIntProperty("priority").notNull();
        info.addIntProperty("status").notNull();

        info.addIntProperty("created").notNull();
        info.addIntProperty("updated").notNull();

        info.setHasKeepSections(true);
    }

    private static void addUserInfo(Schema schema)
    {
        Entity info = schema.addEntity("UserEntity");
        info.setTableName("UserInfo");
        info.setClassNameDao("UserDao");
        info.setJavaPackage(entityPath);

        info.addIdProperty().autoincrement();
        info.addIntProperty("peerId").unique().notNull().index();
        info.addIntProperty("gender").notNull();
        info.addStringProperty("mainName").notNull();
        // 这个可以自动生成pinyin
        info.addStringProperty("pinyinName").notNull();
        info.addStringProperty("realName").notNull();
        info.addStringProperty("avatar").notNull();
        info.addStringProperty("phone").notNull();
        info.addStringProperty("email").notNull();
        info.addIntProperty("departmentId").notNull();

        info.addIntProperty("status").notNull();
        info.addIntProperty("created").notNull();
        info.addIntProperty("updated").notNull();

        info.setHasKeepSections(true);

        // schema.addProtobufEntity();
    }

    private static void addGroupInfo(Schema schema)
    {
        Entity info = schema.addEntity("GroupEntity");
        info.setTableName("GroupInfo");
        info.setClassNameDao("GroupDao");
        info.setJavaPackage(entityPath);

        info.addIdProperty().autoincrement();
        info.addIntProperty("peerId").unique().notNull();
        info.addIntProperty("groupType").notNull();
        info.addStringProperty("mainName").notNull();
        info.addStringProperty("avatar").notNull();
        info.addIntProperty("creatorId").notNull();
        info.addIntProperty("userCnt").notNull();

        info.addStringProperty("userList").notNull();
        info.addIntProperty("version").notNull();
        info.addIntProperty("status").notNull();
        info.addIntProperty("created").notNull();
        info.addIntProperty("updated").notNull();

        info.setHasKeepSections(true);
    }

    private static void addMessage(Schema schema)
    {
        Entity info = schema.addEntity("MessageEntity");
        info.setTableName("Message");
        info.setClassNameDao("MessageDao");
        info.setJavaPackage(entityPath);

        info.implementsSerializable();
        info.addIdProperty().autoincrement();
        Property msgProId = info.addIntProperty("msgId").notNull().getProperty();
        info.addIntProperty("fromId").notNull();
        info.addIntProperty("toId").notNull();

        Property sessionPro  = info.addStringProperty("sessionKey").notNull().getProperty();
        info.addStringProperty("content").notNull();
        info.addIntProperty("msgType").notNull();
        info.addIntProperty("displayType").notNull();

        info.addIntProperty("status").notNull().index();
        info.addIntProperty("created").notNull().index();
        info.addIntProperty("updated").notNull();

        Index index = new Index();
        index.addProperty(msgProId);
        index.addProperty(sessionPro);
        index.makeUnique();
        info.addIndex(index);

        info.setHasKeepSections(true);
    }

    private static void addSessionInfo(Schema schema)
    {
        Entity info = schema.addEntity("SessionEntity");
        info.setTableName("Session");
        info.setClassNameDao("SessionDao");
        info.setJavaPackage(entityPath);

        //point to userId/groupId need sessionType 区分
        info.addIdProperty().autoincrement();
        info.addStringProperty("sessionKey").unique().notNull(); //.unique()
        info.addIntProperty("peerId").notNull();
        info.addIntProperty("peerType").notNull();

        info.addIntProperty("latestMsgType").notNull();
        info.addIntProperty("latestMsgId").notNull();
        info.addStringProperty("latestMsgData").notNull();

        info.addIntProperty("talkId").notNull();
        info.addIntProperty("created").notNull();
        info.addIntProperty("updated").notNull();

        info.setHasKeepSections(true);
    }
}
