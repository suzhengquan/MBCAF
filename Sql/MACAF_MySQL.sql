
CREATE DATABASE MACAF DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci;
USE MACAF;

CREATE TABLE `MACAF_Admin` (
    `id` mediumint(6) unsigned NOT NULL AUTO_INCREMENT,
    `uname` varchar(40) NOT NULL COMMENT 'user name',
    `pwd` char(32) NOT NULL COMMENT 'md5(md5(passwd)+salt)',
    `state` tinyint(2) unsigned NOT NULL DEFAULT '0' COMMENT '',
    `created` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'create time',
    `updated` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'modify time',
    PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4

CREATE TABLE `MACAF_User` (
    `id` int(11) unsigned NOT NULL AUTO_INCREMENT COMMENT 'user id',
    `sex` tinyint(1) unsigned NOT NULL DEFAULT '0' COMMENT '1: male 2: female',
    `name` varchar(32) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT 'user name',
    `nick` varchar(32) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT 'nick name',
	`domain` varchar(32) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT 'domain name',
    `password` varchar(32) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT 'md5(md5(passwd)+salt)',
    `salt` varchar(4) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT 'passwd salt',
    `phone` varchar(11) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT 'phone',
    `email` varchar(64) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT 'email',
    `avatar` varchar(255) COLLATE utf8mb4_bin DEFAULT '' COMMENT 'custom avatar',
    `departId` int(11) unsigned NOT NULL COMMENT 'depart id',
    `state` tinyint(2) unsigned DEFAULT '0' COMMENT '',
	`sign_info` varchar(32) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT 'signature',
    `created` int(11) unsigned NOT NULL COMMENT 'create time',
    `updated` int(11) unsigned NOT NULL COMMENT 'modify time',
    PRIMARY KEY (`id`),
    KEY `idx_domain` (`domain`),
    KEY `idx_name` (`name`),
    KEY `idx_phone` (`phone`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin


CREATE TABLE `MACAF_Group` (
    `id` int(11) NOT NULL AUTO_INCREMENT COMMENT 'group id',
    `name` varchar(256) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT 'group name',
    `avatar` varchar(256) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT 'group avatar',
    `creator` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'creator (user id)',
    `type` tinyint(3) unsigned NOT NULL DEFAULT '1' COMMENT '0 : permanent n: temp (days)',
    `userCnt` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'user count',
    `state` tinyint(3) unsigned NOT NULL DEFAULT '1' COMMENT '0 : nomarl 1: other',
    `version` int(11) unsigned NOT NULL DEFAULT '1' COMMENT 'group version',
    `lastChated` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'last message time',
    `created` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'create time',
    `updated` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'modify time',
    PRIMARY KEY (`id`),
    KEY `idx_name` (`name`),
    KEY `idx_creator` (`creator`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin


CREATE TABLE `MACAF_Organization` (
    `id` int(11) unsigned NOT NULL AUTO_INCREMENT COMMENT 'compay id',
    `name` varchar(64) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT 'compay name',
    `pid` int(11) unsigned NOT NULL COMMENT 'parent compay id',
	`kind` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'kind',
    `state` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'state',
    `created` int(11) unsigned NOT NULL COMMENT 'create time',
    `updated` int(11) unsigned NOT NULL COMMENT 'modify time',
    PRIMARY KEY (`id`),
    KEY `idx_name` (`name`),
    KEY `idx_priority_state` (`priority`,`state`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin


CREATE TABLE `MACAF_Depart` (
    `id` int(11) unsigned NOT NULL AUTO_INCREMENT COMMENT 'depart id',
    `oid` int(11) unsigned NOT NULL COMMENT 'depart id',
    `name` varchar(64) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT 'depart name',
    `priority` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'priority',
    `pid` int(11) unsigned NOT NULL COMMENT 'parent depart id',
    `state` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'state',
    `created` int(11) unsigned NOT NULL COMMENT 'create time',
    `updated` int(11) unsigned NOT NULL COMMENT 'modify time',
    PRIMARY KEY (`id`),
    foreign key(`oid`) references MACAF_Organization(id),
    KEY `idx_name` (`name`),
    KEY `idx_priority_state` (`priority`,`state`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin

CREATE TABLE `MACAF_Product`(
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `name` varchar(64) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT 'product name',
    `oid` int(11) unsigned NOT NULL COMMENT 'compay id',
    `verbose` varchar(256) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT 'product verbose',
    PRIMARY KEY (`id`),
    foreign key(`oid`) references MACAF_Organization(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin

CREATE TABLE `MACAF_IOT`(
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `name` varchar(64) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT 'service name',
    `verbose` varchar(256) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT 'service verbose',
    PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin

CREATE TABLE `MACAF_Audio` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `fromId` int(11) unsigned NOT NULL COMMENT 'sender (user id)',
    `toId` int(11) unsigned NOT NULL COMMENT 'recevier (user id)',
    `path` varchar(255) COLLATE utf8mb4_bin DEFAULT '' COMMENT 'dir path',
    `size` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'file size',
    `duration` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'file duration',
    `created` int(11) unsigned NOT NULL COMMENT 'create time',
    PRIMARY KEY (`id`),
    KEY `idx_fromId_toId` (`fromId`,`toId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin

CREATE TABLE `MACAF_GroupMember` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `gid` int(11) unsigned NOT NULL COMMENT 'group id',
    `userId` int(11) unsigned NOT NULL COMMENT 'user id',
    `state` tinyint(4) unsigned NOT NULL DEFAULT '1' COMMENT '0:normal 1:other',
    `created` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'create time',
    `updated` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'modify time',
    PRIMARY KEY (`id`),
    KEY `idx_gid_userId_state` (`gid`,`userId`,`state`),
    KEY `idx_userId_state_updated` (`userId`,`state`,`updated`),
    KEY `idx_gid_updated` (`gid`,`updated`)
) ENGINE=InnoDB AUTO_INCREMENT=68 DEFAULT CHARSET=utf8mb4


CREATE TABLE `MACAF_Session` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `userId` int(11) unsigned NOT NULL COMMENT 'user id',
    `tid` int(11) unsigned NOT NULL COMMENT 'target id',
    `type` tinyint(1) unsigned DEFAULT '0' COMMENT '1:user 2:group',
    `state` tinyint(1) unsigned DEFAULT '0' COMMENT '0:normal  1:delete',
    `created` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'create time',
    `updated` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'modify time',
    PRIMARY KEY (`id`),
    KEY `idx_userId_tid_state_updated` (`userId`,`tid`,`state`,`updated`),
    KEY `idx_userId_tid_type` (`userId`,`tid`,`type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4


CREATE TABLE `MACAF_SessionRelation` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `uid1` int(11) unsigned NOT NULL COMMENT 'user id 1',
    `uid2` int(11) unsigned NOT NULL COMMENT 'user id 2',
    `state` tinyint(1) unsigned DEFAULT '0' COMMENT '0:normal 1:other',
    `created` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'create time',
    `updated` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'modify time',
    PRIMARY KEY (`id`),
    KEY `idx_uid1_uid2_state_updated` (`uid1`,`uid2`,`state`,`updated`)   
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4

CREATE TABLE `MACAF_GroupMessageX` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `gid` int(11) unsigned NOT NULL COMMENT 'group id',
    `userId` int(11) unsigned NOT NULL COMMENT 'user id',
    `msgId` int(11) unsigned NOT NULL COMMENT 'message id',
    `content` varchar(4096) COLLATE utf8mb4_bin NOT NULL DEFAULT '',
    `type` tinyint(3) unsigned NOT NULL DEFAULT '2',
    `state` int(11) unsigned NOT NULL DEFAULT '0',
    `created` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'create time',
    `updated` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'modify time',
    PRIMARY KEY (`id`),
    KEY `idx_gid_state_created` (`gid`,`state`,`created`),
    KEY `idx_gid_msgId_state_created` (`gid`,`msgId`,`state`,`created`)   
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin

CREATE TABLE MACAF_GroupMessage0 LIKE MACAF_GroupMessageX;
CREATE TABLE MACAF_GroupMessage1 LIKE MACAF_GroupMessageX;
CREATE TABLE MACAF_GroupMessage2 LIKE MACAF_GroupMessageX;
CREATE TABLE MACAF_GroupMessage3 LIKE MACAF_GroupMessageX;
CREATE TABLE MACAF_GroupMessage4 LIKE MACAF_GroupMessageX;
CREATE TABLE MACAF_GroupMessage5 LIKE MACAF_GroupMessageX;
CREATE TABLE MACAF_GroupMessage6 LIKE MACAF_GroupMessageX;
CREATE TABLE MACAF_GroupMessage7 LIKE MACAF_GroupMessageX;
CREATE TABLE MACAF_GroupMessage8 LIKE MACAF_GroupMessageX;
CREATE TABLE MACAF_GroupMessage9 LIKE MACAF_GroupMessageX;
CREATE TABLE MACAF_GroupMessage10 LIKE MACAF_GroupMessageX;
CREATE TABLE MACAF_GroupMessage11 LIKE MACAF_GroupMessageX;
CREATE TABLE MACAF_GroupMessage12 LIKE MACAF_GroupMessageX;
CREATE TABLE MACAF_GroupMessage13 LIKE MACAF_GroupMessageX;
CREATE TABLE MACAF_GroupMessage14 LIKE MACAF_GroupMessageX;
CREATE TABLE MACAF_GroupMessage15 LIKE MACAF_GroupMessageX;

CREATE TABLE `MACAF_MessageX` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `relateId` int(11) unsigned NOT NULL COMMENT 'relate id',
    `fromId` int(11) unsigned NOT NULL COMMENT 'sender (user id)',
    `toId` int(11) unsigned NOT NULL COMMENT 'recevier (user id)',
    `msgId` int(11) unsigned NOT NULL COMMENT 'message id',
    `content` varchar(4096) COLLATE utf8mb4_bin DEFAULT '',
    `type` tinyint(2) unsigned NOT NULL DEFAULT '1',
    `state` tinyint(1) unsigned NOT NULL DEFAULT '0' COMMENT '0:noraml 1:other',
    `created` int(11) unsigned NOT NULL COMMENT 'create time', 
    `updated` int(11) unsigned NOT NULL COMMENT 'modify time',     
	PRIMARY KEY (`id`),
    KEY `idx_relateId_state_created` (`relateId`,`state`,`created`),
    KEY `idx_relateId_state_msgId_created` (`relateId`,`state`,`msgId`,`created`),
    KEY `idx_fromId_toId_created` (`fromId`,`toId`,`created`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin

CREATE TABLE MACAF_Message0 LIKE MACAF_MessageX;
CREATE TABLE MACAF_Message1 LIKE MACAF_MessageX;
CREATE TABLE MACAF_Message2 LIKE MACAF_MessageX;
CREATE TABLE MACAF_Message3 LIKE MACAF_MessageX;
CREATE TABLE MACAF_Message4 LIKE MACAF_MessageX;
CREATE TABLE MACAF_Message5 LIKE MACAF_MessageX;
CREATE TABLE MACAF_Message6 LIKE MACAF_MessageX;
CREATE TABLE MACAF_Message7 LIKE MACAF_MessageX;
CREATE TABLE MACAF_Message8 LIKE MACAF_MessageX;
CREATE TABLE MACAF_Message9 LIKE MACAF_MessageX;
CREATE TABLE MACAF_Message10 LIKE MACAF_MessageX;
CREATE TABLE MACAF_Message11 LIKE MACAF_MessageX;
CREATE TABLE MACAF_Message12 LIKE MACAF_MessageX;
CREATE TABLE MACAF_Message13 LIKE MACAF_MessageX;
CREATE TABLE MACAF_Message14 LIKE MACAF_MessageX;
CREATE TABLE MACAF_Message15 LIKE MACAF_MessageX;