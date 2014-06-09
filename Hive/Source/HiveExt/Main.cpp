/*
* Copyright (C) 2009-2012 Rajko Stojadinovic <http://github.com/rajkosto/hive>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include "HiveLib/ExtStartup.h"
#include "DirectHiveApp.h"

BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		ExtStartup::InitModule([](string profileFolder){ return new DirectHiveApp(profileFolder); });
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		ExtStartup::ProcessShutdown();
		break;
	}
	return TRUE;
}

int main()
{
	Sqf::runTest();

#define DEBUG_SPLIT_TESTS
#ifdef DEBUG_SPLIT_TESTS
	using boost::lexical_cast;

	DllMain(NULL,DLL_PROCESS_ATTACH,NULL);
	char testOutBuf[4096];
	//RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:307:");
//#define CUSTOMDATA_TESTS
#ifdef CUSTOMDATA_TESTS
	RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:302:1337:");
	Sqf::Parameters objStreamStart = boost::get<Sqf::Parameters>(lexical_cast<Sqf::Value>(string(testOutBuf)));
	poco_assert(boost::get<string>(objStreamStart.at(0)) == "ObjectStreamStart");
	string magicKey = boost::get<string>(objStreamStart.at(2));
	for (size_t i=0; i<lexical_cast<int>(objStreamStart.at(1)); i++)
	{
		RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:302:1337:");
		Sqf::Parameters objInfo = boost::get<Sqf::Parameters>(lexical_cast<Sqf::Value>(string(testOutBuf)));
		string objDump = lexical_cast<string>(Sqf::Value(objInfo));
	}
	RVExtension(testOutBuf,sizeof(testOutBuf),string("CHILD:500:"+magicKey+":Character.characters:").c_str());

	vector<string> dbTests;
	dbTests.push_back("CHILD:502:Character.characters:[\"charId\",\"status\",\"handle\"]:[\"( (\",[\"charId\",\"!=\",\"7\"],\"))\"]:[0,10]:");
	dbTests.push_back("CHILD:501:Character.characters:[\"charId\",\"status\",\"handle\"]:[]:");
	dbTests.push_back("CHILD:503:");
	dbTests.push_back("CHILD:504:353253:");
	dbTests.push_back("CHILD:505:sdmf:");
	dbTests.push_back("CHILD:503:132:");
	dbTests.push_back("CHILD:504:aaaaaaaa:");
	dbTests.push_back("CHILD:505:353253:");
	for (auto it=dbTests.begin(); it!=dbTests.end(); ++it)
	{
		RVExtension(testOutBuf,sizeof(testOutBuf),it->c_str());
		string token;
		{
			auto firstResponse = boost::get<Sqf::Parameters>(lexical_cast<Sqf::Value>(string(testOutBuf)));
			string firstMsg = boost::get<string>(firstResponse.at(0));
			if (firstMsg == "PASS")
				token = boost::get<string>(firstResponse.at(1));
			else
				continue;
		}

		int numRows = 0;
		for (;;)
		{
			string reqStr = "CHILD:503:" + token + ":";
			RVExtension(testOutBuf,sizeof(testOutBuf),reqStr.c_str());
			auto detailsResp = boost::get<Sqf::Parameters>(lexical_cast<Sqf::Value>(string(testOutBuf)));
			string detailsMsg = boost::get<string>(detailsResp.at(0));

			if (detailsMsg == "WAIT")
			{
				Sleep(10);
				continue;
			}

			if (detailsMsg != "PASS")
				break;
			else
			{
				numRows = Sqf::GetIntAny(detailsResp.at(1));
				break;
			}
		}
		
		for (int i=0; i< numRows+2; i++)
		{
			string reqStr = "CHILD:504:" + token + ":";
			RVExtension(testOutBuf,sizeof(testOutBuf),reqStr.c_str());

			auto rowResp = boost::get<Sqf::Parameters>(lexical_cast<Sqf::Value>(string(testOutBuf)));
			string rowMsg = boost::get<string>(rowResp.at(0));

			if (rowMsg != "PASS")
			{
				if (rowMsg != "NOMORE")
					break;
			}
		}

		string reqStr = "CHILD:505:" + token + ":";
		RVExtension(testOutBuf,sizeof(testOutBuf),reqStr.c_str());
		auto closeResp = boost::get<Sqf::Parameters>(lexical_cast<Sqf::Value>(string(testOutBuf)));
		string closeMsg = boost::get<string>(closeResp.at(0));

		if (closeMsg != "PASS")
			continue;
	}
	
#else
	//RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:302:2:");
	//|CHILD:201:11:[]:[]:[]:[]:false:false:0:0:0:0:[]:0:0::0:2:|
	//RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:501:dayzunleashed.character_data:[""Medical""]:[[""CharacterID"",""="",""2""]]:[0,1]:");
	//RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:550:1:1:");
	//RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:551:");
	//RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:421:2:HNGamers:22773510:");
	//RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:422:1:22773510:");
	//RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:220:1:");
	//RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:221:22773510:");
	//RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:552:");
	//RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:302:2:");
	//RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:150:1:22773510:");
	//RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:152:30873:hunter:2342:");
	//RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:152:30873:hunter:fdghfdghdf:");
	//RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:152:30873:hunter:34:");
	//RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:230:Land_HouseV_3I1::::30:");
	RVExtension(testOutBuf, sizeof(testOutBuf), "CHILD:400 : 1 : Land_DZE_LargeWoodDoorLocked : 5810611027514268 : [268.059, [5810.57, 11027.5, -0.143317]] : [] : [] : 39445 : 0 : Blue85 :");
	//RVExtension(testOutBuf, sizeof(testOutBuf), "CHILD:643:58237110386217:1:");
	//RVExtension(testOutBuf, sizeof(testOutBuf), "CHILD:641:5824311036415269:[[],[]]:1:");
	//RVExtension(testOutBuf, sizeof(testOutBuf), "CHILD:230:Land_aif_hlaska:Land_afbarabizna:Land_cihlovej_dum_in:Land_HouseV_3I1:30:");
	//RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:152:14276:testing:4564:");
	//RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:152:14276:fuckface:5645:");
	//RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:152:14276:tightcunt:767:");
	//RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:152:30872:Pin:54363:");
	//RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:151:14276:hunter1:");
	//RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:151:14276:hunter:");
	//RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:151:14276:medical:");
	//RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:151:14276:testing:");
	//RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:151:14276:fuckface:");
	//RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:151:14276:tightcunt:");
	//RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:151:14276:dicklick:");
	//RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:153:30872:");
	//RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:201:21375:[299,[12617.9,15447.4,0.002]]:[]:[]:[false,false,false,false,false,false,false,12000,[],[0,0],0,[0.05,50.684]]:false:false:0:0:3267:1:[,aidlpercmstpsnonwnondnon_player_idlesteady04,40]:0:0:TheVisad_DZU:0:");
	//RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:400:2:WoodGate_DZ:32432432322342:[40.1727,[13040.8,15735,0.014276253]]:[]:[]:143:0:907:");
	//RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:201:11:[]:[]:[]:[]:false:false:0:0:0:0:[]:0:0::0:2:");
	//RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:201:5700692:[80,[2588.59,10073.7,0.001]]:");
	
	//RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:101:23572678:1311:Audris:");
	//RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:308:1:Wire_cat1:0:6255222:[329.449,[10554.4,3054.12,0]]:[]:[]:0:1.055e14:356:");
	//										  |CHILD:400:1:Fort_RazorWire:12790215031911209:[208.744,[12790.2,15031.9,1.14432]]:[]:[]:929:0:514:|
	//RVExtension(testOutBuf,sizeof(testOutBuf),"CHILD:400:1:Fort_RazorWire:12790215031911209:[208.744,[12790.2,15031.9,1.14432]]:[]:[]:929:0:514:");
	//["CHILD:400:%1:%2:%3:%4:%5:%6:%7:%8:%9:",dayZ_instance,_uid,_class,_charID,_worldspace, [],[],_squad ,_combination];
#endif

	DllMain(NULL,DLL_PROCESS_DETACH,NULL);
#endif

	return 0;
}