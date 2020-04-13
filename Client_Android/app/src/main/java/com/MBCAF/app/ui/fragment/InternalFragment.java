package com.MBCAF.app.ui.fragment;

import android.content.Intent;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ListView;

import com.MBCAF.R;
import com.MBCAF.app.PreDefine;
import com.MBCAF.app.ui.activity.WebViewFragmentActivity;
import com.MBCAF.app.ui.adapter.InternalAdapter;
import com.MBCAF.app.ui.base.TTBaseFragment;

public class InternalFragment extends TTBaseFragment {
    private View curView = null;
    private ListView internalListView;
    private InternalAdapter mAdapter;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        if (null != curView) {
            ((ViewGroup) curView.getParent()).removeView(curView);
            return curView;
        }
        curView = inflater.inflate(R.layout.tt_fragment_internal,
                topContentView);

        initRes();
        mAdapter = new InternalAdapter(this.getActivity());
        internalListView.setAdapter(mAdapter);
        mAdapter.update();
        return curView;
    }

    private void initRes() {
        // 设置顶部标题栏
        setTopTitle(getActivity().getString(R.string.t004));
        internalListView = (ListView)curView.findViewById(R.id.internalListView);
        internalListView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> adapterView, View view, int i, long l) {
                String url = mAdapter.getItem(i).getItemUrl();
                Intent intent=new Intent(InternalFragment.this.getActivity(),WebViewFragmentActivity.class);
                intent.putExtra(PreDefine.WEBVIEW_URL, url);
                startActivity(intent);
            }
        });
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }

    @Override
    protected void initHandler() {
    }

}
