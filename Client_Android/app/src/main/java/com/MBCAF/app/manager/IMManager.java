package com.MBCAF.app.manager;

import android.content.Context;

public abstract class IMManager {
	protected Context ctx;
    protected void setContext(Context context) {
		if (context == null) {
			throw new RuntimeException("context is null");
		}
		ctx = context;
	}

    public void  onStartIMManager(Context ctx){
        setContext(ctx);
        doOnStart();
    }

    public abstract  void doOnStart();

	public abstract void reset();
}
