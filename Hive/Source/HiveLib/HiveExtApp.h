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

#pragma once

#include "Shared/Common/Types.h"
#include "Shared/Server/AppServer.h"

#include "Sqf.h"
#include "DataSource/CharDataSource.h"
#include "DataSource/ObjDataSource.h"
#include "DataSource/CustomDataSource.h"
#include "DataSource/BuildingDataSource.h"
#include "DataSource/SquadDataSource.h"
#include "DataSource/QuestDataSource.h"
#include "DataSource/InstanceDataSource.h"
#include "DataSource/AntiHackDataSource.h"
#include "DataSource/MessagingDataSource.h"
#include "DataSource/LootDataSource.h"
#include "Database/SqlStatement.h"

#include <boost/function.hpp>
#include <boost/date_time.hpp>

class Database;
class HiveExtApp: public AppServer
{
public:
	HiveExtApp(string suffixDir);
	virtual ~HiveExtApp() {};

	struct ServerShutdownException : public std::exception
	{
		ServerShutdownException(string theKey, Sqf::Value theVal = false) 
			: _theKey(std::move(theKey)), _theVal(std::move(theVal)) {}
		bool keyMatches(const string& otherKey) const 
		{ 
			return ((_theKey.length() > 0) && (_theKey == otherKey)); 

		}
		const Sqf::Value& getReturnValue() const { return _theVal; }
	private:
		string _theKey;
		Sqf::Value _theVal;
	};
	void callExtension(const char* function, char* output, size_t outputSize);
protected:
	int main(const std::vector<std::string>& args);

	virtual bool initialiseService() = 0;
protected:
	void setServerId(int newId) { _serverId = newId; }
	int getServerId() const { return _serverId; }

	unique_ptr<CharDataSource> _charData;
	unique_ptr<AntiHackDataSource> _antihackData;
	unique_ptr<ObjDataSource> _objData;
	unique_ptr<CustomDataSource> _customData;
	//Unleashed
	unique_ptr<SquadDataSource> _plySqdData;
	unique_ptr<BuildingDataSource> _bldData;
	unique_ptr<QuestDataSource> _qstData;
	unique_ptr<QuestDataSource> _plyQstData;
	unique_ptr<SquadDataSource> _sqdData;
	unique_ptr<SquadDataSource> _psqdData;
	unique_ptr<InstanceDataSource> _instData;
	unique_ptr<MessagingDataSource> _msgData;
	unique_ptr<LootDataSource> _lootData;

	string _initKey;
private:
	int _serverId;
	boost::posix_time::time_duration _timeOffset;
	void setupClock();

	typedef boost::function<Sqf::Value (Sqf::Parameters)> HandlerFunc;
	map<int,HandlerFunc> handlers;

	Sqf::Value getDateTime(Sqf::Parameters params);

	ObjDataSource::ServerObjectsQueue _srvObjects;
	ObjDataSource::ServerObjectsQueue _srvGarageVehicleObjects;
	BuildingDataSource::ServerBuildingsQueue _srvBuildings;
	BuildingDataSource::ServerGaragesQueue _srvGarage;
	SquadDataSource::ServerSquadsQueue _srvSquads;
	SquadDataSource::ServerSquadsQueue _srvInstance;
	SquadDataSource::ServerSquadsQueue _srvPlayerSquads;
	QuestDataSource::ServerQuestsQueue _srvQuests;
	AntiHackDataSource::AntiHackQueue _srvAntiHacks;
	MessagingDataSource::ServerMessagingQueue _srvMessaging;
	LootDataSource::ServerLootsQueue _srvLoot;

	Sqf::Value streamObjects(Sqf::Parameters params);
	Sqf::Value streamVehicle(Sqf::Parameters params);
	Sqf::Value streamBuildings(Sqf::Parameters params);
	Sqf::Value removeFromGarage(Sqf::Parameters params);
	Sqf::Value insertIntoGarage(Sqf::Parameters params);
	Sqf::Value streamMessages(Sqf::Parameters params);
	Sqf::Value streamGarage(Sqf::Parameters params);
	Sqf::Value vehiclesInGarage(Sqf::Parameters params);
	Sqf::Value streamSquad( Sqf::Parameters params );
	Sqf::Value streamPlayerSquad( Sqf::Parameters params );
	Sqf::Value streamQuest( Sqf::Parameters params );
	Sqf::Value streamInstance( Sqf::Parameters params );

	Sqf::Value objectPublish(Sqf::Parameters params);
	Sqf::Value objectInventory(Sqf::Parameters params, bool byUID = false);
	Sqf::Value buildingInventory(Sqf::Parameters params);
	Sqf::Value objectDelete(Sqf::Parameters params, bool byUID = false);
	Sqf::Value buildingDelete(Sqf::Parameters params);
	Sqf::Value playerQuestDelete(Sqf::Parameters params, bool byUID = false);
	Sqf::Value squadDelete(Sqf::Parameters params, bool byUID = false);
	Sqf::Value playerSquadDelete(Sqf::Parameters params, bool byUID = false);

	Sqf::Value squadPublish( Sqf::Parameters params );
	Sqf::Value playerSquadPublish( Sqf::Parameters params );


	Sqf::Value vehicleMoved(Sqf::Parameters params);
	Sqf::Value vehicleDamaged(Sqf::Parameters params);

	Sqf::Value loadPlayer(Sqf::Parameters params);
	Sqf::Value loadPlayerMedical(Sqf::Parameters params);
	Sqf::Value loadCustomInventory(Sqf::Parameters params);
	Sqf::Value recordCharacterVariables(Sqf::Parameters params);
	Sqf::Value loadCharacterVariables(Sqf::Parameters params);
	Sqf::Value loadCharacterVariableArray(Sqf::Parameters params);
	Sqf::Value loadCharacterDetails(Sqf::Parameters params);
	Sqf::Value recordCharacterLogin(Sqf::Parameters params);
	Sqf::Value recordCharacterActivity(Sqf::Parameters params);
	
	Sqf::Value loadAHBans(Sqf::Parameters params);
	Sqf::Value loadAHAdmins(Sqf::Parameters params);
	Sqf::Value loadAHWhiteList(Sqf::Parameters params);
	
	Sqf::Value playerUpdate(Sqf::Parameters params);
	Sqf::Value playerInit(Sqf::Parameters params);
	Sqf::Value playerDeath(Sqf::Parameters params);

	Sqf::Value dataRequest(Sqf::Parameters params, bool async = false);
	Sqf::Value dataStatus(Sqf::Parameters params);
	Sqf::Value dataFetchRow(Sqf::Parameters params);
	Sqf::Value dataClose(Sqf::Parameters params);

	Sqf::Value changeTableAccess(Sqf::Parameters params);
	Sqf::Value serverShutdown(Sqf::Parameters params);

		//Unleashed
	Sqf::Value buildingPublish(Sqf::Parameters params);
	Sqf::Value garagePublish(Sqf::Parameters params);
	Sqf::Value instancePublish(Sqf::Parameters params);
	Sqf::Value loadLootPiles(Sqf::Parameters params);

	SqlStatementID _stmtCreateLoot;
};
