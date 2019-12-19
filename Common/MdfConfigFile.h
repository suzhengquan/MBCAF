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

#ifndef _MDF_CONFIG_FILE_H_
#define _MDF_CONFIG_FILE_H_

#include "MdfPreInclude.h"
#include "MdfConnect.h"
#include "NetCore.h"

namespace Mdf
{
    /**
    @version 0.9.1
    */
    class MdfNetAPI ConfigFile
    {
    public:
        ConfigFile(const String & file, const String & section);
        ~ConfigFile();

        /**
        @version 0.9.1
        */
        bool isLoad() const;

        /**
        @version 0.9.1
        */
        bool getValue(const String & name, String & out);

        /**
        @version 0.9.1
        */
        bool getValue(const String & name, Mui32 & out);

        /**
        @version 0.9.1
        */
        bool setValue(const String & name, const String & value);

        /**
        @version 0.9.1
        */
        bool setValue(const String & name, Mui32 value);
    protected:
        ConfigFile();
        void loadFile();
        void closeFile();
        void saveFile();
    protected:
        typedef std::map<String, String> ConfigList;
    private:
        ACE_Configuration_Section_Key mSectionKey;
        ACE_Ini_ImpExp * mConfigFileImpl;
        ACE_Configuration_Heap mConfig;
        ConfigList mConfigList;
        String mConfigfile;
        String mSection;
        bool mLoad;
    };

	void M_ReadConfig(ConfigFile * in, ConnectInfoList & out, const char * ip, const char * port);
}
#endif
