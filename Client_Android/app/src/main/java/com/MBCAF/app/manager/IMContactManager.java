package com.MBCAF.app.manager;

import com.MBCAF.db.DBInterface;
import com.MBCAF.db.entity.DepartmentEntity;
import com.MBCAF.db.entity.UserEntity;
import com.MBCAF.app.event.CommonEvent;
import com.MBCAF.pb.base.ProtoBuf2JavaBean;
import com.MBCAF.pb.Proto;
import com.MBCAF.pb.MsgServer;
import com.MBCAF.common.IMUIHelper;
import com.MBCAF.common.Logger;
import com.MBCAF.common.pinyin.PinYin;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import de.greenrobot.event.EventBus;

public class IMContactManager extends IMManager {
    private Logger logger = Logger.getLogger(IMContactManager.class);

    // 单例
    private static IMContactManager inst = new IMContactManager();
    public static IMContactManager instance() {
            return inst;
    }
    private IMSocketManager imSocketManager = IMSocketManager.instance();
    private DBInterface dbInterface = DBInterface.instance();

    // 自身状态字段
    private boolean  userDataReady = false;
    private Map<Integer,UserEntity> userMap = new ConcurrentHashMap<>();
    private Map<Integer,DepartmentEntity> departmentMap = new ConcurrentHashMap<>();


    @Override
    public void onStart() {
    }

    public void onNormalLoginOk(){
        onLocalLoginOk();
        onLocalNetOk();
    }

    public void onLocalLoginOk(){
        logger.d("contact#loadAllUserInfo");

        List<DepartmentEntity> deptlist = dbInterface.loadAllDept();
        logger.d("contact#loadAllDept dbsuccess");

        List<UserEntity> userlist = dbInterface.loadAllUsers();
        logger.d("contact#loadAllUserInfo dbsuccess");

        for(UserEntity userInfo:userlist){
            // todo DB的状态不包含拼音的，这个样每次都要加载啊
            PinYin.getPinYin(userInfo.getMainName(), userInfo.getPinyinElement());
            userMap.put(userInfo.getPeerId(),userInfo);
        }

        for(DepartmentEntity deptInfo:deptlist){
            PinYin.getPinYin(deptInfo.getDepartName(), deptInfo.getPinyinElement());
            departmentMap.put(deptInfo.getDepartId(),deptInfo);
        }
        triggerEvent(CommonEvent.CE_User_InfoOK);
    }

    /**
     * 网络链接成功，登陆之后请求
     */
    public void onLocalNetOk(){
        // 部门信息
        int deptUpdateTime = dbInterface.getDeptLastTime();
        reqGetDepartment(deptUpdateTime);

        // 用户信息
        int updateTime = dbInterface.getUserInfoLastTime();
        logger.d("contact#loadAllUserInfo req-updateTime:%d", updateTime);
        reqGetAllUsers(updateTime);
    }

    @Override
    public void reset() {
        userDataReady = false;
        userMap.clear();
    }

    public void triggerEvent(CommonEvent event) {
        //先更新自身的状态
        switch (event){
            case CE_User_InfoOK:
                userDataReady = true;
                break;
        }
        EventBus.getDefault().postSticky(event);
    }

    private void reqGetAllUsers(int lastUpdateTime) {
		logger.i("contact#reqGetAllUsers");
		int userId = IMLoginManager.instance().getLoginId();

        MsgServer.VaryUserInfoListQ imAllUserReq  = MsgServer.VaryUserInfoListQ.newBuilder()
                .setUserId(userId)
                .setLatestUpdateTime(lastUpdateTime).build();
        int sid = Proto.MsgTypeID.MTID_MsgServer_VALUE;
        int cid = Proto.MsgServerMsgID.MSID_BuddyObjectListQ_VALUE;
        imSocketManager.sendRequest(imAllUserReq, sid, cid);
	}

	public void onRepAllUsers(MsgServer.VaryUserInfoListA imAllUserRsp) {
		logger.i("contact#onRepAllUsers");
        int userId = imAllUserRsp.getUserId();
        int lastTime = imAllUserRsp.getLatestUpdateTime();
        // lastTime 需要保存嘛? 不保存了

        int count =  imAllUserRsp.getUserListCount();
		logger.i("contact#user cnt:%d", count);
        if(count <=0){
            return;
        }

		int loginId = IMLoginManager.instance().getLoginId();
        if(userId != loginId){
            logger.e("[fatal error] userId not equels loginId ,cause by onRepAllUsers");
            return ;
        }

        List<Proto.UserInfo> changeList =  imAllUserRsp.getUserListList();
        ArrayList<UserEntity> needDb = new ArrayList<>();
        for(Proto.UserInfo userInfo:changeList){
            UserEntity entity =  ProtoBuf2JavaBean.getUserEntity(userInfo);
            userMap.put(entity.getPeerId(),entity);
            needDb.add(entity);
        }

        dbInterface.batchInsertOrUpdateUser(needDb);
        triggerEvent(CommonEvent.CE_User_InfoUpdate);
	}

    public UserEntity findContact(int buddyId){
        if(buddyId > 0 && userMap.containsKey(buddyId)){
            return userMap.get(buddyId);
        }
        return null;
    }

    public void reqGetDetaillUsers(ArrayList<Integer> userIds){
        logger.i("contact#contact#reqGetDetaillUsers");
        if(null == userIds || userIds.size() <=0){
            logger.i("contact#contact#reqGetDetaillUsers return,cause by null or empty");
            return;
        }
        int loginId = IMLoginManager.instance().getLoginId();
        MsgServer.UserInfoListQ imUsersInfoReq  =  MsgServer.UserInfoListQ.newBuilder()
                .setUserId(loginId)
                .addAllUserIdList(userIds)
                .build();

        int sid = Proto.MsgTypeID.MTID_MsgServer_VALUE;
        int cid = Proto.MsgServerMsgID.MSID_BubbyObjectInfoQ_VALUE;
        imSocketManager.sendRequest(imUsersInfoReq, sid, cid);
    }

    public void  onRepDetailUsers(MsgServer.UserInfoListA imUsersInfoRsp){
        int loginId = imUsersInfoRsp.getUserId();
        boolean needEvent = false;
        List<Proto.UserInfo> userInfoList = imUsersInfoRsp.getUserInfoListList();

        ArrayList<UserEntity>  dbNeed = new ArrayList<>();
        for(Proto.UserInfo userInfo:userInfoList) {
            UserEntity userEntity = ProtoBuf2JavaBean.getUserEntity(userInfo);
            int userId = userEntity.getPeerId();
            if (userMap.containsKey(userId) && userMap.get(userId).equals(userEntity)) {
                //没有必要通知更新
            } else {
                needEvent = true;
                userMap.put(userEntity.getPeerId(), userEntity);
                dbNeed.add(userEntity);
                if (userInfo.getUserId() == loginId) {
                    IMLoginManager.instance().setLoginInfo(userEntity);
                }
            }
        }
        // 负责userMap
        dbInterface.batchInsertOrUpdateUser(dbNeed);

        // 判断有没有必要进行推送
        if(needEvent){
            triggerEvent(CommonEvent.CE_User_InfoUpdate);
        }
    }

    public DepartmentEntity findDepartment(int deptId){
         DepartmentEntity entity = departmentMap.get(deptId);
         return entity;
    }

    public  List<DepartmentEntity>  getDepartmentSortedList(){
        // todo eric efficiency
        List<DepartmentEntity> departmentList = new ArrayList<>(departmentMap.values());
        Collections.sort(departmentList, new Comparator<DepartmentEntity>() {
            @Override
            public int compare(DepartmentEntity entity1, DepartmentEntity entity2) {

                if (entity1.getPinyinElement().pinyin == null) {
                    PinYin.getPinYin(entity1.getDepartName(), entity1.getPinyinElement());
                }
                if (entity2.getPinyinElement().pinyin == null) {
                    PinYin.getPinYin(entity2.getDepartName(), entity2.getPinyinElement());
                }
                return entity1.getPinyinElement().pinyin.compareToIgnoreCase(entity2.getPinyinElement().pinyin);

            }
        });
        return departmentList;
    }

    public  List<UserEntity> getContactSortedList() {
        // todo eric efficiency
        List<UserEntity> contactList = new ArrayList<>(userMap.values());
        Collections.sort(contactList, new Comparator<UserEntity>(){
            @Override
            public int compare(UserEntity entity1, UserEntity entity2) {
                if (entity2.getPinyinElement().pinyin.startsWith("#")) {
                    return -1;
                } else if (entity1.getPinyinElement().pinyin.startsWith("#")) {
                    // todo eric guess: latter is > 0
                    return 1;
                } else {
                    if(entity1.getPinyinElement().pinyin==null)
                    {
                        PinYin.getPinYin(entity1.getMainName(),entity1.getPinyinElement());
                    }
                    if(entity2.getPinyinElement().pinyin==null)
                    {
                        PinYin.getPinYin(entity2.getMainName(),entity2.getPinyinElement());
                    }
                    return entity1.getPinyinElement().pinyin.compareToIgnoreCase(entity2.getPinyinElement().pinyin);
                }
            }
        });
        return contactList;
    }

    // 通讯录中的部门显示 需要根据优先级
    public List<UserEntity> getDepartmentTabSortedList() {
        // todo eric efficiency
        List<UserEntity> contactList = new ArrayList<>(userMap.values());
        Collections.sort(contactList, new Comparator<UserEntity>() {
            @Override
            public int compare(UserEntity entity1, UserEntity entity2) {
                DepartmentEntity dept1 = departmentMap.get(entity1.getDepartmentId());
                DepartmentEntity dept2 = departmentMap.get(entity2.getDepartmentId());

                if (entity1.getDepartmentId() == entity2.getDepartmentId()) {
                    // start compare
                    if (entity2.getPinyinElement().pinyin.startsWith("#")) {
                        return -1;
                    } else if (entity1.getPinyinElement().pinyin.startsWith("#")) {
                        // todo eric guess: latter is > 0
                        return 1;
                    } else {
                        if(entity1.getPinyinElement().pinyin==null)
                        {
                            PinYin.getPinYin(entity1.getMainName(), entity1.getPinyinElement());
                        }
                        if(entity2.getPinyinElement().pinyin==null)
                        {
                            PinYin.getPinYin(entity2.getMainName(),entity2.getPinyinElement());
                        }
                        return entity1.getPinyinElement().pinyin.compareToIgnoreCase(entity2.getPinyinElement().pinyin);
                    }
                    // end compare
                } else {
                    if(dept1!=null && dept2!=null && dept1.getDepartName()!=null && dept2.getDepartName()!=null)
                    {
                        return dept1.getDepartName().compareToIgnoreCase(dept2.getDepartName());
                    }
                    else
                        return 1;

                }
            }
        });
        return contactList;
    }

    public  List<UserEntity> getSearchContactList(String key){
       List<UserEntity> searchList = new ArrayList<>();
       for(Map.Entry<Integer,UserEntity> entry:userMap.entrySet()){
           UserEntity user = entry.getValue();
           if (IMUIHelper.handleContactSearch(key, user)) {
               searchList.add(user);
           }
       }
       return searchList;
    }

    public List<DepartmentEntity> getSearchDepartList(String key) {
        List<DepartmentEntity> searchList = new ArrayList<>();
        for(Map.Entry<Integer,DepartmentEntity> entry:departmentMap.entrySet()){
            DepartmentEntity dept = entry.getValue();
            if (IMUIHelper.handleDepartmentSearch(key, dept)) {
                searchList.add(dept);
            }
        }
        return searchList;
    }

    // 更新的方式与userInfo一直，根据时间点
    public void reqGetDepartment(int lastUpdateTime)
    {
        logger.i("contact#reqGetDepartment");
        int userId = IMLoginManager.instance().getLoginId();

        MsgServer.VaryDepartListQ imDepartmentReq  = MsgServer.VaryDepartListQ.newBuilder()
                .setUserId(userId)
                .setLatestUpdateTime(lastUpdateTime).build();
        int sid = Proto.MsgTypeID.MTID_MsgServer_VALUE;
        int cid = Proto.MsgServerMsgID.MSID_BuddyOrganizationQ_VALUE;
        imSocketManager.sendRequest(imDepartmentReq,sid,cid);
    }

    public void onRepDepartment(MsgServer.VaryDepartListA imDepartmentRsp)
    {
        logger.i("contact#onRepDepartment");
        int userId = imDepartmentRsp.getUserId();
        int lastTime = imDepartmentRsp.getLatestUpdateTime();

        int count =  imDepartmentRsp.getDeptListCount();
        logger.i("contact#department cnt:%d", count);
        // 如果用户找不到depart 那么部门显示未知
        if(count <=0){
            return;
        }

        int loginId = IMLoginManager.instance().getLoginId();
        if(userId != loginId){
            logger.e("[fatal error] userId not equels loginId ,cause by onRepDepartment");
            return ;
        }
        List<Proto.DepartInfo> changeList =  imDepartmentRsp.getDeptListList();
        ArrayList<DepartmentEntity> needDb = new ArrayList<>();

        for(Proto.DepartInfo  departInfo:changeList){
            DepartmentEntity entity =  ProtoBuf2JavaBean.getDepartEntity(departInfo);
            departmentMap.put(entity.getDepartId(),entity);
            needDb.add(entity);
        }
        // 部门信息更新
        dbInterface.batchInsertOrUpdateDepart(needDb);
        triggerEvent(CommonEvent.CE_User_InfoUpdate);
    }

    public Map<Integer, UserEntity> getUserMap() {
        return userMap;
    }

    public Map<Integer, DepartmentEntity> getDepartmentMap() {
        return departmentMap;
    }

    public boolean isUserDataReady() {
        return userDataReady;
    }

}
