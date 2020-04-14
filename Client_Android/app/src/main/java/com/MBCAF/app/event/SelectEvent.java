package com.MBCAF.app.event;

import com.MBCAF.app.ui.adapter.album.ImageItem;

import java.util.List;

public class SelectEvent {
    private List<ImageItem> list;
    public SelectEvent(List<ImageItem> list){
        this.list = list;
    }

    public List<ImageItem> getList() {
        return list;
    }

    public void setList(List<ImageItem> list) {
        this.list = list;
    }
}
