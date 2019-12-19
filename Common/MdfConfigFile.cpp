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

#include "MdfConfigFile.h"
#include <sstream>

namespace Mdf
{
    //-----------------------------------------------------------------------
    ConfigFile::ConfigFile():
        mConfigFileImpl(0),
        mLoad(false)
    {
    }
    //-----------------------------------------------------------------------
    ConfigFile::ConfigFile(const String & file, const String & section):
        mConfigfile(file),
        mConfigFileImpl(0),
        mSection(section),
        mLoad(false)
    {
		loadFile();
    }
    //-----------------------------------------------------------------------
    ConfigFile::~ConfigFile()
    {
		closeFile();
    }
    //-----------------------------------------------------------------------
    bool ConfigFile::isLoad() const
    {
        return mLoad;
    }
    //-----------------------------------------------------------------------
    void ConfigFile::loadFile()
    {
        if (mConfig.open() == -1)
        {
            mLoad = false;
            return;
        }
        else
        {
            mLoad = true;
        }

        mConfigFileImpl = new ACE_Ini_ImpExp(mConfig);

        mConfigFileImpl->import_config(mConfigfile.c_str());

        if (mConfig.open_section(mConfig.root_section(), mSection.c_str(), 0, mSectionKey) == -1)
        {
            mLoad = false;
            delete mConfigFileImpl;
            mConfigFileImpl = 0;
        }
        else
        {
            mLoad = true;
        }
    }
	//-----------------------------------------------------------------------
	void ConfigFile::saveFile()
	{
		if (mConfigFileImpl->export_config(mConfigfile.c_str()) == -1)
		{
			printf("error %d, line %d/n", ACE_OS::last_error(), __LINE__);
		}
	}
    //-----------------------------------------------------------------------
    void ConfigFile::closeFile()
    {
        if (mConfigFileImpl)
        {
            delete mConfigFileImpl;
            mConfigFileImpl = 0;
        }
    }
    //-----------------------------------------------------------------------
    bool ConfigFile::getValue(const String & key, String & out)
    {
        if (!mLoad)
            return false;

        ConfigList::iterator it = mConfigList.find(key);
        if (it == mConfigList.end())
        {
            ACE_TString tempvalue;
            if (mConfig.get_string_value(mSectionKey, key.c_str(), tempvalue) == -1)
                return false;

            out = tempvalue.c_str();
            mConfigList.insert(make_pair(key, out));
        }
        else
        {
            out = it->second;
        }
        return true;
    }
    //-----------------------------------------------------------------------
    bool ConfigFile::getValue(const String & key, Mui32 & out)
    {
        if (!mLoad)
            return false;

        ConfigList::iterator it = mConfigList.find(key);
        if (it == mConfigList.end())
        {
            if (mConfig.get_integer_value(mSectionKey, key.c_str(), out) == -1)
                return false;

			StringStream ss;
            ss << out;
            mConfigList.insert(make_pair(key, ss.str()));
        }
        else
        {
            out = _ttoi(it->second.c_str());
        }
        return true;
    }
    //-----------------------------------------------------------------------
    bool ConfigFile::setValue(const String & name, const String & value)
    {
        if (!mLoad)
            return false;

        ConfigList::iterator it = mConfigList.find(name);
        if (it != mConfigList.end())
        {
            it->second = value;
            saveFile();
        }
        else
        {
            ACE_TString tempvalue(value.c_str());
            if (mConfig.set_string_value(mSectionKey, name.c_str(), tempvalue) != -1)
            {
                mConfigList.insert(make_pair(name, value));
                saveFile();
            }
            else
                return false;
        }
        return true;
    }
    //-----------------------------------------------------------------------
    bool ConfigFile::setValue(const String & name, Mui32 value)
    {
        if (!mLoad)
            return false;

		StringStream tstr;
		tstr << value;

        ConfigList::iterator it = mConfigList.find(name);
        if (it != mConfigList.end())
        {
            it->second = tstr.str();
            saveFile();
        }
        else
        {
            ACE_TString tempvalue(tstr.str().c_str());
            if (mConfig.set_string_value(mSectionKey, name.c_str(), tempvalue) != -1)
            {
                mConfigList.insert(make_pair(name, tstr.str()));
                saveFile();
            }
            else
                return false;
        }
        return true;
    }
    //-----------------------------------------------------------------------
	void M_ReadConfig(ConfigFile * config, ConnectInfoList & out, const char * ip, const char * port)
	{
		String::value_type serverIP[64];
		String::value_type serverPort[64];

		String ipstr;
		String portstr;
		Mui32 cnt = 0;
		while (true)
		{
			_stprintf(serverIP, _T("%s%d"), ip, cnt + 1);
			_stprintf(serverPort, _T("%s%d"), port, cnt + 1);
			config->getValue(serverIP, ipstr);
			config->getValue(serverPort, portstr);
			if (config->getValue(serverIP, ipstr) && config->getValue(serverPort, portstr))
			{
				ConnectInfo * list = new ConnectInfo(cnt);
				list->mServerIP = ipstr;
				list->mServerPort = _ttoi(portstr.c_str());
				out.push_back(list);
			}
			else
			{
				break;
			}
		}
	}
	//-----------------------------------------------------------------------
}