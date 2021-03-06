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
package com.MBCAF.db.entity;

// THIS CODE IS GENERATED BY greenDAO, EDIT ONLY INSIDE THE "KEEP"-SECTIONS

// KEEP INCLUDES - put your custom includes here
import com.MBCAF.app.entity.SearchElement;
import com.MBCAF.common.pinyin.PinYin;
// KEEP INCLUDES END
/**
 * Entity mapped to table Department.
 */
public class DepartmentEntity {

    private Long id;
    private int departId;
    /** Not-null value. */
    private String departName;
    private int priority;
    private int status;
    private int created;
    private int updated;

    // KEEP FIELDS - put your custom fields here
    private PinYin.PinYinElement pinyinElement = new PinYin.PinYinElement();
    private SearchElement searchElement = new SearchElement();
    // KEEP FIELDS END

    public DepartmentEntity() {
    }

    public DepartmentEntity(Long id) {
        this.id = id;
    }

    public DepartmentEntity(Long id, int departId, String departName, int priority, int status, int created, int updated) {
        this.id = id;
        this.departId = departId;
        this.departName = departName;
        this.priority = priority;
        this.status = status;
        this.created = created;
        this.updated = updated;
    }

    public Long getId() {
        return id;
    }

    public void setId(Long id) {
        this.id = id;
    }

    public int getDepartId() {
        return departId;
    }

    public void setDepartId(int departId) {
        this.departId = departId;
    }

    /** Not-null value. */
    public String getDepartName() {
        return departName;
    }

    /** Not-null value; ensure this value is available before it is saved to the database. */
    public void setDepartName(String departName) {
        this.departName = departName;
    }

    public int getPriority() {
        return priority;
    }

    public void setPriority(int priority) {
        this.priority = priority;
    }

    public int getStatus() {
        return status;
    }

    public void setStatus(int status) {
        this.status = status;
    }

    public int getCreated() {
        return created;
    }

    public void setCreated(int created) {
        this.created = created;
    }

    public int getUpdated() {
        return updated;
    }

    public void setUpdated(int updated) {
        this.updated = updated;
    }

    // KEEP METHODS - put your custom methods here

    public PinYin.PinYinElement getPinyinElement() {
        return pinyinElement;
    }

    public SearchElement getSearchElement() {
        return searchElement;
    }
    // KEEP METHODS END

}
