package com.MBCAF.app.ui.activity;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.text.TextUtils;
import android.view.ContextThemeWrapper;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;
import android.widget.Toast;

import com.MBCAF.db.sp.LoginSp;
import com.MBCAF.db.sp.SystemConfigSp;
import com.MBCAF.R;
import com.MBCAF.app.PreDefine;
import com.MBCAF.common.IMUIHelper;
import com.MBCAF.app.event.CommonEvent;
import com.MBCAF.app.event.CommonEvent;
import com.MBCAF.app.manager.IMLoginManager;
import com.MBCAF.app.network.IMService;
import com.MBCAF.app.ui.base.TTBaseActivity;
import com.MBCAF.app.network.IMServiceConnector;
import com.MBCAF.common.Logger;

import de.greenrobot.event.EventBus;

public class LoginActivity extends TTBaseActivity
{
    private Logger logger = Logger.getLogger(LoginActivity.class);
    private Handler uiHandler = new Handler();
    private EditText mNameView;
    private EditText mPasswordView;
    private View loginPage;
    private View splashPage;
    private View mLoginStatusView;
    private TextView mSwitchLoginServer;
    private InputMethodManager intputManager;
    private EditText mEditText;

    private IMService imService;
    private boolean autoLogin = true;
    private boolean loginSuccess = false;

    private IMServiceConnector imServiceConnector = new IMServiceConnector() {
        @Override
        public void onServiceDisconnected() {
        }

        @Override
        public void onIMServiceConnected() {
            logger.d("login#onIMServiceConnected");
            imService = imServiceConnector.getIMService();
            try {
                do {
                    if (imService == null) {
                        //后台服务启动链接失败
                        break;
                    }
                    IMLoginManager loginManager = imService.getLoginManager();
                    LoginSp loginSp = imService.getLoginSp();
                    if (loginManager == null || loginSp == null) {
                        // 无法获取登陆控制器
                        break;
                    }

                    LoginSp.SpLoginIdentity loginIdentity = loginSp.getLoginIdentity();
                    if (loginIdentity == null) {
                        // 之前没有保存任何登陆相关的，跳转到登陆页面
                        break;
                    }

                    mNameView.setText(loginIdentity.getLoginName());
                    if (TextUtils.isEmpty(loginIdentity.getPwd())) {
                        // 密码为空，可能是loginOut
                        break;
                    }
                    mPasswordView.setText(loginIdentity.getPwd());

                    if (!autoLogin) {
                        break;
                    }

                    handleGotLoginIdentity(loginIdentity);
                    return;
                } while (false);

                // 异常分支都会执行这个
                handleNoLoginIdentity();
            }
            catch (Exception e)
            {
                // 任何未知的异常
                logger.w("loadIdentity failed");
                handleNoLoginIdentity();
            }
        }
    };


    /**
     * 跳转到登陆的页面
     */
    private void handleNoLoginIdentity() {
        logger.i("login#handleNoLoginIdentity");
        uiHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                showLoginPage();
            }
        }, 1500);
    }

    /**
     * 自动登陆
     */
    private void handleGotLoginIdentity(final LoginSp.SpLoginIdentity loginIdentity) {
        logger.i("login#handleGotLoginIdentity");

        uiHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                logger.d("login#start auto login");
                if (imService == null || imService.getLoginManager() == null) {
                    Toast.makeText(LoginActivity.this, getString(R.string.t017), Toast.LENGTH_SHORT).show();
                    showLoginPage();
                }
                imService.getLoginManager().login(loginIdentity);
            }
        }, 500);
    }


    private void showLoginPage() {
        splashPage.setVisibility(View.GONE);
        loginPage.setVisibility(View.VISIBLE);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);
        intputManager = (InputMethodManager) getSystemService(this.INPUT_METHOD_SERVICE);
        logger.d("login#onCreate");

        SystemConfigSp.instance().init(getApplicationContext());
        if (TextUtils.isEmpty(SystemConfigSp.instance().getStrConfig(SystemConfigSp.SysCfgDimension.LOGINSERVER))) {
            SystemConfigSp.instance().setStrConfig(SystemConfigSp.SysCfgDimension.LOGINSERVER, PreDefine.ACCESS_MSG_ADDRESS);
        }

        imServiceConnector.connect(LoginActivity.this);
        EventBus.getDefault().register(this);

        setContentView(R.layout.tt_activity_login);
        mSwitchLoginServer = (TextView)findViewById(R.id.sign_switch_login_server);
        mSwitchLoginServer.setOnClickListener(new View.OnClickListener(){
            @Override
            public void onClick(View view) {
                AlertDialog.Builder builder = new AlertDialog.Builder(new ContextThemeWrapper(LoginActivity.this, android.R.style.Theme_Holo_Dialog));
                LayoutInflater inflater = (LayoutInflater)getSystemService(Context.LAYOUT_INFLATER_SERVICE);
                View dialog_view = inflater.inflate(R.layout.tt_custom_dialog, null);

                final EditText editText = (EditText)dialog_view.findViewById(R.id.dialog_edit_content);
                mEditText = editText;
                editText.setText(SystemConfigSp.instance().getStrConfig(SystemConfigSp.SysCfgDimension.LOGINSERVER));
                TextView textText = (TextView)dialog_view.findViewById(R.id.dialog_title);
                textText.setText(R.string.t063);

                ((RadioButton)dialog_view.findViewById(R.id.radioButton2)).setChecked(true);
                mEditText.setFocusable(false);
                mEditText.setFocusableInTouchMode(false);
                mEditText.setVisibility(View.INVISIBLE);

                RadioGroup rg = (RadioGroup)dialog_view.findViewById(R.id.radioGroup1);
                rg.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
                    @Override
                    public void onCheckedChanged(RadioGroup rg, int checkedId)
                    {
                        //for(int i=0; i<rg.getChildCount(); i++)
                        //{
                        // RadioButton btn = (RadioButton) rg.getChildAt(i);
                        //   if(btn.getId()==R.id.radioButton) {
                        if(checkedId != R.id.radioButton)
                        {
                            //final EditText editText1 = (EditText)dialog_view.findViewById(R.id.dialog_edit_content);
                            mEditText.setFocusable(false);
                            mEditText.setFocusableInTouchMode(false);
                            mEditText.setVisibility(View.INVISIBLE);
                        }
                        else
                        {
                            //final EditText editText1 = (EditText)dialog_view.findViewById(R.id.dialog_edit_content);
                            mEditText.setFocusable(true);
                            mEditText.setFocusableInTouchMode(true);
                            mEditText.setVisibility(View.VISIBLE);
                            mEditText.setSelection(7);
                            mEditText.requestFocus();
                        }
                       // }
                    }
                });

                builder.setView(dialog_view);
                builder.setNegativeButton(getString(R.string.t046), new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialogInterface, int i) {
                        dialogInterface.dismiss();
                    }
                });
                builder.setPositiveButton(getString(R.string.t047), new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {

                        if(!TextUtils.isEmpty(editText.getText().toString().trim()))
                        {
                            SystemConfigSp.instance().setStrConfig(SystemConfigSp.SysCfgDimension.LOGINSERVER,editText.getText().toString().trim());
                            dialog.dismiss();
                        }
                    }
                });
                builder.show();
            }
        });

        mNameView = (EditText) findViewById(R.id.name);
        mPasswordView = (EditText) findViewById(R.id.password);
        mNameView.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override
            public void onFocusChange(View v, boolean hasFocus) {
                EditText editText=(EditText)v;
                if (!hasFocus)
                {// 失去焦点
                    editText.setHint(editText.getTag().toString());
                }
                else {
                    editText.setTag(editText.getHint().toString());
                    editText.setHint("");
                }
            }
        });
        mPasswordView.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView textView, int id, KeyEvent keyEvent) {

                if (id == R.id.login || id == EditorInfo.IME_NULL) {
                    attemptLogin();
                    return true;
                }
                return false;
            }
        });

        mPasswordView.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override
            public void onFocusChange(View v, boolean hasFocus)
            {
                EditText editText=(EditText)v;
                if (!hasFocus)
                {// 失去焦点
                    editText.setHint(editText.getTag().toString());
                }
                else
                    {
                    editText.setTag(editText.getHint().toString());
                    editText.setHint("");
                }
            }
        });
        mLoginStatusView = findViewById(R.id.login_status);
        findViewById(R.id.sign_in_button).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                intputManager.hideSoftInputFromWindow(mPasswordView.getWindowToken(), 0);
                attemptLogin();
            }
        });
        initAutoLogin();
    }

    private void initAutoLogin()
    {
        logger.i("login#initAutoLogin");

        splashPage = findViewById(R.id.splash_page);
        loginPage = findViewById(R.id.login_page);
        autoLogin = shouldAutoLogin();

        splashPage.setVisibility(autoLogin ? View.VISIBLE : View.GONE);
        loginPage.setVisibility(autoLogin ? View.GONE : View.VISIBLE);

        loginPage.setOnTouchListener(new OnTouchListener() {

            @Override
            public boolean onTouch(View v, MotionEvent event) {

                if (mPasswordView != null)
                {
                    intputManager.hideSoftInputFromWindow(mPasswordView.getWindowToken(), 0);
                }

                if (mNameView != null)
                {
                    intputManager.hideSoftInputFromWindow(mNameView.getWindowToken(), 0);
                }

                return false;
            }
        });

        if (autoLogin)
        {
            Animation splashAnimation = AnimationUtils.loadAnimation(this, R.anim.login_splash);
            if (splashAnimation == null)
            {
                logger.e("login#loadAnimation login_splash failed");
                return;
            }

            splashPage.startAnimation(splashAnimation);
        }
    }

    // 主动退出的时候， 这个地方会有值,更具pwd来判断
    private boolean shouldAutoLogin()
    {
        Intent intent = getIntent();
        if (intent != null)
        {
            boolean notAutoLogin = intent.getBooleanExtra(PreDefine.KEY_LOGIN_NOT_AUTO, false);
            logger.d("login#notAutoLogin:%s", notAutoLogin);
            if (notAutoLogin)
            {
                return false;
            }
        }
        return true;
    }

    @Override
    protected void onDestroy()
    {
        super.onDestroy();

        imServiceConnector.disconnect(LoginActivity.this);
        EventBus.getDefault().unregister(this);
        splashPage = null;
        loginPage = null;
    }

    public void attemptLogin()
    {
        String loginName = mNameView.getText().toString();
        String mPassword = mPasswordView.getText().toString();
        boolean cancel = false;
        View focusView = null;

        if (TextUtils.isEmpty(loginName))
        {
            Toast.makeText(this, getString(R.string.t015), Toast.LENGTH_SHORT).show();
            mPasswordView.setText("");
            focusView = mNameView;
            cancel = true;
        }
        else if (TextUtils.isEmpty(mPassword))
        {
            Toast.makeText(this, getString(R.string.t016), Toast.LENGTH_SHORT).show();
            focusView = mPasswordView;
            cancel = true;
        }

        if (cancel)
        {
            focusView.requestFocus();
        }
        else {
            showProgress(true);
            if (imService != null)
            {
                loginName = loginName.trim();
                mPassword = mPassword.trim();
                imService.getLoginManager().login(loginName, mPassword);
            }
        }
    }

    private void showProgress(final boolean show)
    {
        if (show)
        {
            mLoginStatusView.setVisibility(View.VISIBLE);
        }
        else
            {
            mLoginStatusView.setVisibility(View.GONE);
        }
    }

    @Override
    public void onBackPressed()
    {
        logger.d("login#onBackPressed");
        //imLoginMgr.cancel();
        // TODO Auto-generated method stub
        super.onBackPressed();
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event)
    {
//        if (keyCode == KeyEvent.KEYCODE_BACK && event.getRepeatCount() == 0) {
//            LoginActivity.this.finish();
//            return true;
//        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    protected void onStop() {
        super.onStop();
    }

    /**
     * ----------------------------event 事件驱动----------------------------
     */
    public void onEventMainThread(CommonEvent event)
    {
        switch (event) {
            case CE_Login_Success:
            case CE_Login_OK:
                onLoginSuccess();
                break;
            case CE_Login_FailAuth:
            case CE_Login_Fail: {
                if (!loginSuccess)
                    onLoginFailure(event);
            }
                break;
            case CE_Connect_MsgServerConnectFail:
            case CE_Connect_MsgServerAddrAFail: {
                if (!loginSuccess)
                    onSocketFailure(event);
            }
                break;
        }
    }

    private void onLoginSuccess()
    {
        logger.i("login#onLoginSuccess");
        loginSuccess = true;
        Intent intent = new Intent(LoginActivity.this, MainActivity.class);
        startActivity(intent);
        LoginActivity.this.finish();
    }

    private void onLoginFailure(CommonEvent event)
    {
        logger.e("login#onLoginError -> errorCode:%s", event.name());
        showLoginPage();
        String errorTip = getString(IMUIHelper.getLoginErrorTip(event));
        logger.d("login#errorTip:%s", errorTip);
        mLoginStatusView.setVisibility(View.GONE);
        Toast.makeText(this, errorTip, Toast.LENGTH_SHORT).show();
    }

    private void onSocketFailure(CommonEvent event)
    {
        logger.e("login#onLoginError -> errorCode:%s,", event.name());
        showLoginPage();
        String errorTip = getString(IMUIHelper.getSocketErrorTip(event));
        logger.d("login#errorTip:%s", errorTip);
        mLoginStatusView.setVisibility(View.GONE);
        Toast.makeText(this, errorTip, Toast.LENGTH_SHORT).show();
    }
}
