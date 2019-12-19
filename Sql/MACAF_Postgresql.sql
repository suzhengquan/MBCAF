
CREATE DATABASE "MACAF" WITH OWNER "postgres" ENCODING 'UTF8' LC_COLLATE = 'en_US.UTF-8' LC_CTYPE = 'en_US.UTF-8' TEMPLATE template0;
\c MACAF;

CREATE TABLE `MACAF_Admin` (
    `id` serial NOT NULL,
    `uname` varchar(40) NOT NULL,
    `pwd` char(32) NOT NULL,
	`salt` varchar(4) NOT NULL DEFAULT '',
    `state` smallint NOT NULL DEFAULT '0',
    `created` integer NOT NULL DEFAULT '0',
    `updated` integer NOT NULL DEFAULT '0',
    PRIMARY KEY (`id`)
)

comment on column MACAF_Admin.uname is 'user name';
comment on column MACAF_Admin.pwd is 'md5(md5(passwd)+salt)';
comment on column MACAF_Admin.salt is 'passwd salt';
comment on column MACAF_Admin.created is 'create time';
comment on column MACAF_Admin.updated is 'modify time';

CREATE TABLE `MACAF_User` (
    `id` serial NOT NULL,
    `sex` smallint NOT NULL DEFAULT '0',
    `name` varchar(32) NOT NULL DEFAULT '',
    `nick` varchar(32) NOT NULL DEFAULT '',
	`domain` varchar(32) NOT NULL DEFAULT '',
    `password` varchar(32) NOT NULL DEFAULT '',
    `salt` varchar(4) NOT NULL DEFAULT '',
    `phone` varchar(11) NOT NULL DEFAULT '',
    `email` varchar(64) NOT NULL DEFAULT '',
    `avatar` varchar(255) DEFAULT '',
    `departId` integer NOT NULL,
    `state` smallint DEFAULT '0',
	`sign_info` varchar(32) NOT NULL DEFAULT '',
    `created` integer NOT NULL,
    `updated` integer NOT NULL,
    PRIMARY KEY (`id`)
)

comment on column MACAF_User.id is 'user id';
comment on column MACAF_User.sex is '1: male 2: female';
comment on column MACAF_User.name is 'user name';
comment on column MACAF_User.nick is 'nick name';
comment on column MACAF_User.domain is 'domain name';
comment on column MACAF_User.password is 'md5(md5(passwd)+salt)';
comment on column MACAF_User.salt is 'passwd salt';
comment on column MACAF_User.phone is 'phone';
comment on column MACAF_User.email is 'email';
comment on column MACAF_User.avatar is 'custom avatar';
comment on column MACAF_User.departId is 'depart id';
comment on column MACAF_User.sign_info is 'signature';
comment on column MACAF_User.created is 'create time';
comment on column MACAF_User.updated is 'modify time';
create index idx_domain on MACAF_User (`domain`); 
create index idx_name on MACAF_User (`name`); 
create index idx_phone on MACAF_User (`phone`); 

CREATE TABLE `MACAF_Group` (
    `id` serial NOT NULL,
    `name` varchar(256) NOT NULL DEFAULT '',
    `avatar` varchar(256) NOT NULL DEFAULT '',
    `creator` integer NOT NULL DEFAULT '0',
    `type` smallint NOT NULL DEFAULT '1',
    `userCnt` integer NOT NULL DEFAULT '0',
    `state` smallint NOT NULL DEFAULT '1',
    `version` integer NOT NULL DEFAULT '1',
    `lastChated` integer NOT NULL DEFAULT '0',
    `created` integer NOT NULL DEFAULT '0',
    `updated` integer NOT NULL DEFAULT '0',
    PRIMARY KEY (`id`)
)

comment on column MACAF_Group.id is 'group id';
comment on column MACAF_Group.name is 'group name';
comment on column MACAF_Group.avatar is 'group avatar';
comment on column MACAF_Group.creator is 'creator (user id)';
comment on column MACAF_Group.type is '0 : permanent n: temp (days)';
comment on column MACAF_Group.userCnt is 'user count';
comment on column MACAF_Group.state is '0 : nomarl 1: other';
comment on column MACAF_Group.version is 'group version';
comment on column MACAF_Group.lastChated is 'last message time';
comment on column MACAF_Group.created is 'create time';
comment on column MACAF_Group.updated is 'modify time';
create index idx_name on MACAF_Group (`name`); 
create index idx_creator on MACAF_Group (`creator`); 

CREATE TABLE `MACAF_Organization` (
    `id` serial NOT NULL,
    `name` varchar(64) NOT NULL DEFAULT '',
    `pid` integer NOT NULL,
	`kind` integer NOT NULL DEFAULT '0',
    `state` integer NOT NULL DEFAULT '0',
    `created` integer NOT NULL,
    `updated` integer NOT NULL,
    PRIMARY KEY (`id`)
)

comment on column MACAF_Organization.id is 'compay id';
comment on column MACAF_Organization.name is 'compay name';
comment on column MACAF_Organization.pid is 'parent compay id';
comment on column MACAF_Organization.kind is 'kind';
comment on column MACAF_Organization.state is 'state';
comment on column MACAF_Organization.created is 'create time';
comment on column MACAF_Organization.updated is 'modify time';
create index idx_name on MACAF_Organization (`name`); 
create index idx_priority_state on MACAF_Organization (`priority`,`state`); 

CREATE TABLE `MACAF_Depart` (
    `id` serial NOT NULL,
    `oid` integer NOT NULL references MACAF_Organization(id),
    `name` varchar(64) NOT NULL DEFAULT '',
    `priority` integer NOT NULL DEFAULT '0',
    `pid` integer NOT NULL,
    `state` integer NOT NULL DEFAULT '0',
    `created` integer NOT NULL,
    `updated` integer NOT NULL,
    PRIMARY KEY (`id`)
)

comment on column MACAF_Depart.oid is 'compay id';
comment on column MACAF_Depart.name is 'depart name';
comment on column MACAF_Depart.priority is 'priority';
comment on column MACAF_Depart.pid is 'parent depart id';
comment on column MACAF_Depart.state is 'state';
comment on column MACAF_Depart.created is 'create time';
comment on column MACAF_Depart.updated is 'modify time';
create index idx_name on MACAF_Depart (`name`); 
create index idx_oid on MACAF_Depart (oid);
create index idx_priority_state on MACAF_Depart (`priority`,`state`); 

CREATE TABLE `MACAF_Product`(
    `id` serial NOT NULL,
    `name` varchar(64) NOT NULL DEFAULT '',
    `oid` integer NOT NULL references MACAF_Organization(id),
    `verbose` varchar(256) NOT NULL DEFAULT '',
    PRIMARY KEY (`id`)
)

comment on column MACAF_Product.name is 'product name';
comment on column MACAF_Product.oid is 'compay id';
comment on column MACAF_Product.verbose is 'product verbose';
create index idx_oid on MACAF_Product (oid);

CREATE TABLE `MACAF_IOT`(
    `id` serial NOT NULL,
    `name` varchar(64) NOT NULL DEFAULT '',
    `verbose` varchar(256) NOT NULL DEFAULT '',
    PRIMARY KEY (`id`)
)

comment on column MACAF_IOT.name is 'service name';
comment on column MACAF_IOT.verbose is 'service verbose';

CREATE TABLE `MACAF_Audio` (
    `id` serial NOT NULL,
    `fromId` integer NOT NULL,
    `toId` integer NOT NULL,
    `path` varchar(255) DEFAULT '',
    `size` integer NOT NULL DEFAULT '0',
    `duration` integer NOT NULL DEFAULT '0',
    `created` integer NOT NULL,
    PRIMARY KEY (`id`)
)

comment on column MACAF_Audio.fromId is 'sender (user id)';
comment on column MACAF_Audio.toId is 'recevier (user id)';
comment on column MACAF_Audio.path is 'dir path';
comment on column MACAF_Audio.size is 'file size';
comment on column MACAF_Audio.duration is 'file duration';
comment on column MACAF_Audio.created is 'create time';
create index idx_fromId_toId on MACAF_Audio (`fromId`,`toId`); 

CREATE TABLE `MACAF_GroupMember` (
    `id` serial NOT NULL,
    `gid` integer NOT NULL,
    `userId` integer NOT NULL,
    `state` tinyint(4) NOT NULL DEFAULT '1',
    `created` integer NOT NULL DEFAULT '0',
    `updated` integer NOT NULL DEFAULT '0',
    PRIMARY KEY (`id`)
)

comment on column MACAF_GroupMember.gid is 'group id';
comment on column MACAF_GroupMember.userId is 'user id';
comment on column MACAF_GroupMember.state is '0:normal 1:other';
comment on column MACAF_GroupMember.created is 'create time';
comment on column MACAF_GroupMember.updated is 'modify time';
create index idx_gid_userId_state on MACAF_GroupMember (`gid`,`userId`,`state`); 
create index idx_userId_state_updated on MACAF_GroupMember (`userId`,`state`,`updated`); 
create index idx_gid_updated on MACAF_GroupMember (`gid`,`updated`); 

CREATE TABLE `MACAF_Session` (
    `id` serial NOT NULL,
    `userId` integer NOT NULL,
    `tid` integer NOT NULL,
    `type` smallint DEFAULT '0',
    `state` smallint DEFAULT '0',
    `created` integer NOT NULL DEFAULT '0',
    `updated` integer NOT NULL DEFAULT '0',
    PRIMARY KEY (`id`)
)

comment on column MACAF_Session.userId is 'user id';
comment on column MACAF_Session.tid is 'target id';
comment on column MACAF_Session.type is '1:user 2:group';
comment on column MACAF_Session.state is '0:normal  1:delete';
comment on column MACAF_Session.created is 'create time';
comment on column MACAF_Session.updated is 'modify time';
create index idx_userId_tid_state_updated on MACAF_Session (`userId`,`tid`,`state`,`updated`); 
create index idx_userId_tid_type on MACAF_Session (`userId`,`tid`,`type`); 

CREATE TABLE `MACAF_SessionRelation` (
    `id` serial NOT NULL,
    `uid1` integer NOT NULL,
    `uid2` integer NOT NULL,
    `state` smallint DEFAULT '0',
    `created` integer NOT NULL DEFAULT '0',
    `updated` integer NOT NULL DEFAULT '0',
    PRIMARY KEY (`id`)  
)

comment on column MACAF_SessionRelation.uid1 is 'user id 1';
comment on column MACAF_SessionRelation.uid2 is 'user id 2';
comment on column MACAF_SessionRelation.state is '0:normal 1:other';
comment on column MACAF_SessionRelation.created is 'create time';
comment on column MACAF_SessionRelation.updated is 'modify time';
create index idx_uid1_uid2_state_updated on MACAF_SessionRelation (`uid1`,`uid2`,`state`,`updated`); 

CREATE TABLE `MACAF_GroupMessageX` (
    `id` serial NOT NULL,
    `gid` integer NOT NULL,
    `userId` integer NOT NULL,
    `msgId` integer NOT NULL,
    `content` varchar(4096) NOT NULL DEFAULT '',
    `type` smallint NOT NULL DEFAULT '2',
    `state` integer NOT NULL DEFAULT '0',
    `created` integer NOT NULL DEFAULT '0',
    `updated` integer NOT NULL DEFAULT '0',
    PRIMARY KEY (`id`)
)

comment on column MACAF_GroupMessageX.gid is 'group id';
comment on column MACAF_GroupMessageX.userId is 'user id';
comment on column MACAF_GroupMessageX.msgId is 'message id';
comment on column MACAF_GroupMessageX.created is 'create time';
comment on column MACAF_GroupMessageX.updated is 'modify time';
create index idx_gid_state_created on MACAF_GroupMessageX (`gid`,`state`,`created`); 
create index idx_gid_msgId_state_created on MACAF_GroupMessageX (`gid`,`msgId`,`state`,`created`); 

create table MACAF_GroupMessage0 ( like MACAF_GroupMessageX including all );
create table MACAF_GroupMessage1 ( like MACAF_GroupMessageX including all );
create table MACAF_GroupMessage2 ( like MACAF_GroupMessageX including all );
create table MACAF_GroupMessage3 ( like MACAF_GroupMessageX including all );
create table MACAF_GroupMessage4 ( like MACAF_GroupMessageX including all );
create table MACAF_GroupMessage5 ( like MACAF_GroupMessageX including all );
create table MACAF_GroupMessage6 ( like MACAF_GroupMessageX including all );
create table MACAF_GroupMessage7 ( like MACAF_GroupMessageX including all );
create table MACAF_GroupMessage8 ( like MACAF_GroupMessageX including all );
create table MACAF_GroupMessage9 ( like MACAF_GroupMessageX including all );
create table MACAF_GroupMessage10 ( like MACAF_GroupMessageX including all );
create table MACAF_GroupMessage11 ( like MACAF_GroupMessageX including all );
create table MACAF_GroupMessage12 ( like MACAF_GroupMessageX including all );
create table MACAF_GroupMessage13 ( like MACAF_GroupMessageX including all );
create table MACAF_GroupMessage14 ( like MACAF_GroupMessageX including all );
create table MACAF_GroupMessage15 ( like MACAF_GroupMessageX including all );

CREATE TABLE `MACAF_MessageX` (
    `id` serial NOT NULL,
    `relateId` integer NOT NULL,
    `fromId` integer NOT NULL,
    `toId` integer NOT NULL,
    `msgId` integer NOT NULL,
    `content` varchar(4096) DEFAULT '',
    `type` smallint NOT NULL DEFAULT '1',
    `state` smallint NOT NULL DEFAULT '0',
    `created` integer NOT NULL, 
    `updated` integer NOT NULL,     
	PRIMARY KEY (`id`)
)

comment on column MACAF_MessageX.relateId is 'relate id';
comment on column MACAF_MessageX.fromId is 'sender (user id)';
comment on column MACAF_MessageX.toId is 'recevier (user id)';
comment on column MACAF_MessageX.state is '0:noraml 1:other';
comment on column MACAF_MessageX.created is 'create time';
comment on column MACAF_MessageX.updated is 'modify time';
create index idx_relateId_state_created on MACAF_MessageX (`relateId`,`state`,`created`); 
create index idx_relateId_state_msgId_created on MACAF_MessageX (`relateId`,`state`,`msgId`,`created`); 
create index idx_fromId_toId_created on MACAF_MessageX (`fromId`,`toId`,`created`); 

create table MACAF_Message0 ( like MACAF_MessageX including all );
create table MACAF_Message1 ( like MACAF_MessageX including all );
create table MACAF_Message2 ( like MACAF_MessageX including all );
create table MACAF_Message3 ( like MACAF_MessageX including all );
create table MACAF_Message4 ( like MACAF_MessageX including all );
create table MACAF_Message5 ( like MACAF_MessageX including all );
create table MACAF_Message6 ( like MACAF_MessageX including all );
create table MACAF_Message7 ( like MACAF_MessageX including all );
create table MACAF_Message8 ( like MACAF_MessageX including all );
create table MACAF_Message9 ( like MACAF_MessageX including all );
create table MACAF_Message10 ( like MACAF_MessageX including all );
create table MACAF_Message11 ( like MACAF_MessageX including all );
create table MACAF_Message12 ( like MACAF_MessageX including all );
create table MACAF_Message13 ( like MACAF_MessageX including all );
create table MACAF_Message14 ( like MACAF_MessageX including all );
create table MACAF_Message15 ( like MACAF_MessageX including all );