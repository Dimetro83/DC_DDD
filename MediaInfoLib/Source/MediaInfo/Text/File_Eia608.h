// File_Eia608 - Info for EIA-608 files
// Copyright (C) 2009-2011 MediaArea.net SARL, Info@MediaArea.net
//
// This library is free software: you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this library. If not, see <http://www.gnu.org/licenses/>.
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about PGS files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_Eia608H
#define MediaInfo_File_Eia608H
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include <vector>
#include <bitset>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Eia608
//***************************************************************************

static const int8u Eia608_Rows=15; //Screen size, specified by the standard
static const int8u Eia608_Columns=32; //Screen size, specified by the standard
class File_Eia608 : public File__Analyze
{
public :
    //In
    int8u   cc_type;

    //Constructor/Destructor
    File_Eia608();
    ~File_Eia608();

private :
    //Streams management
    void Streams_Fill();
    void Streams_Finish();

    //Synchro
    void Read_Buffer_Unsynched();

    //Buffer - Global
    void Read_Buffer_Init();
    void Read_Buffer_Continue();
    void Read_Buffer_AfterParsing();

    //Function
    void Standard (int8u Character);

    std::vector<std::vector<int8u> > XDS_Data;
    size_t XDS_Level;
    bool TextMode; //CC or T
    bool DataChannelMode; //if true, CC2/CC4/T2/T4

    void XDS(int8u cc_data_1, int8u cc_data_2);
    void XDS();
    void XDS_Current();
    void XDS_Current_ProgramName();
    void XDS_Current_ContentAdvisory();
    void XDS_Current_CopyAndRedistributionControlPacket();
    void XDS_PublicService();
    void XDS_PublicService_NationalWeatherService();
    void XDS_Channel();
    void XDS_Channel_NetworkName();
    void Special(int8u cc_data_1, int8u cc_data_2);
    void Special_10(int8u cc_data_2);
    void Special_11(int8u cc_data_2);
    void Special_12(int8u cc_data_2);
    void Special_13(int8u cc_data_2);
    void Special_14(int8u cc_data_2);
    void Special_17(int8u cc_data_2);
    void PreambleAddressCode(int8u cc_data_1, int8u cc_data_2);

    //An attribute consists of Attribute_Color_*, optionally OR'd with Attribute_Underline and/or Attribute_Italic
    enum attributes
    {
        Attribute_Color_White   =0x00,
        Attribute_Color_Green   =0x01,
        Attribute_Color_Blue    =0x02,
        Attribute_Color_Cyan    =0x03,
        Attribute_Color_Red     =0x04,
        Attribute_Color_Yellow  =0x05,
        Attribute_Color_Magenta =0x06,
        Attribute_Underline     =0x10,
        Attribute_Italic        =0x20,
    };

    struct character
    {
        wchar_t Value;
        int8u   Attribute;

        character()
        {
            Value=L' ';
        }
    };
    void Character_Fill(wchar_t Character);
    void HasChanged();
    void Illegal(int8u cc_data_1, int8u cc_data_2);
    struct stream
    {
        vector<vector<character> > CC_Displayed;
        vector<vector<character> > CC_NonDisplayed;
        bool    InBack; //The back buffer is written
        size_t  x;
        size_t  y;
        int8u   Attribute_Current;
        size_t  RollUpLines;
        bool    Synched;

        stream()
        {
            InBack=false;
            x=0;
            y=Eia608_Rows-1;
            Attribute_Current=0;
            RollUpLines=0;
            Synched=false;
        }
    };
    std::vector<stream*> Streams;

    int8u cc_data_1_Old;
    int8u cc_data_2_Old;
    bool   HasContent;
};

} //NameSpace

#endif
