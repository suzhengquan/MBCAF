package com.MBCAF.app.manager;

import android.app.Activity;

public class IMStackManager {
    /**
     * Stack 中对应的Activity列表  （也可以写做 Stack<Activity>）
     */
    private static java.util.Stack<Activity> mActivityStack;
    private static IMStackManager mInstance;

    public static IMStackManager getStackManager() {
        if (mInstance == null) {
            mInstance = new IMStackManager();
        }
        return mInstance;
    }

    public void popActivity(Activity activity) {
        if (activity != null) {
            activity.finish();
            mActivityStack.remove(activity);
            activity = null;
        }
    }

    public Activity currentActivity() {
        //lastElement()获取最后个子元素，这里是栈顶的Activity
        if(mActivityStack == null || mActivityStack.size() ==0){
            return null;
        }
        Activity activity = (Activity) mActivityStack.lastElement();
        return activity;
    }

    public void pushActivity(Activity activity) {
        if (mActivityStack == null) {
            mActivityStack = new java.util.Stack();
        }
        mActivityStack.add(activity);
    }

    public void popTopActivitys(Class clsss) {
        while (true) {
            Activity activity = currentActivity();
            if (activity == null) {
                break;
            }
            if (activity.getClass().equals(clsss)) {
                break;
            }
            popActivity(activity);
        }
    }

    public void popAllActivitys() {
        while (true) {
            Activity activity = currentActivity();
            if (activity == null) {
                break;
            }
            popActivity(activity);
        }
    }
}
