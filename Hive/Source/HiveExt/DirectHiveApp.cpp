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

#include "DirectHiveApp.h"

DirectHiveApp::DirectHiveApp(string suffixDir) : HiveExtApp(suffixDir) {}

#include "Shared/Library/Database/DatabaseLoader.h"
#include "HiveLib/DataSource/SqlCharDataSource.h"
#include "HiveLib/DataSource/SqlObjDataSource.h"
#include "HiveLib/DataSource/SqlBuildingDataSource.h"
#include "HiveLib/DataSource/SqlQuestDataSource.h"
#include "HiveLib/DataSource/SqlSquadDataSource.h"
#include "HiveLib/DataSource/SqlInstanceDataSource.h"
#include "HiveLib/DataSource/SqlAntiHackDataSource.h"
#include "HiveLib/DataSource/SqlMessagingDataSource.h"

bool DirectHiveApp::initialiseService()
{
	//Load up databases
	{
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> globalDBConf(config().createView("Database"));
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> objDBConf(config().createView("ObjectDB"));
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> bldDBConf(config().createView("BuildingDB"));
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> instDBConf(config().createView("InstanceDB"));
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> sqdDBConf(config().createView("SquadDB"));
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> qstDBConf(config().createView("QuestDB"));
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> pqstDBConf(config().createView("PlayerQuestDB"));
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> psqdDBConf(config().createView("PlayerSquadDB"));
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> antiHackDBConf(config().createView("AntiHackDB"));
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> messagingDBConf(config().createView("MessagingDB"));



		
		try
		{
			Poco::Logger& dbLogger = Poco::Logger::get("Database");
			_charDb = DatabaseLoader::Create(globalDBConf);
			if (!_charDb->initialise(dbLogger,DatabaseLoader::MakeConnParams(globalDBConf)))
				return false;

			_objDb = _charDb;
			if (objDBConf->getBool("Use",false))
			{
				Poco::Logger& objDBLogger = Poco::Logger::get("ObjectDB");
				_objDb = DatabaseLoader::Create(objDBConf);
				if (!_objDb->initialise(objDBLogger,DatabaseLoader::MakeConnParams(objDBConf)))
					return false;
			}
			_bldDb = _charDb;
			if (bldDBConf->getBool("Use",false))
			{
				Poco::Logger& bldDBLogger = Poco::Logger::get("BuildingDB");
				_bldDb = DatabaseLoader::Create(bldDBConf);
				if (!_bldDb->initialise(bldDBLogger,DatabaseLoader::MakeConnParams(bldDBConf)))
					return false;
			}

			//_instData,_sqdData,_psqdData,_plyQstData,_qstData
			_instDb = _charDb;
			if (instDBConf->getBool("Use",false))
			{
				Poco::Logger& instDBLogger = Poco::Logger::get("InstanceDB");
				_instDb = DatabaseLoader::Create(instDBConf);
				if (!_instDb->initialise(instDBLogger,DatabaseLoader::MakeConnParams(instDBConf)))
					return false;
			}

			_sqdDb = _charDb;
			if (sqdDBConf->getBool("Use",false))
			{
				Poco::Logger& sqdDBLogger = Poco::Logger::get("SquadDB");
				_sqdDb = DatabaseLoader::Create(sqdDBConf);
				if (!_sqdDb->initialise(sqdDBLogger,DatabaseLoader::MakeConnParams(sqdDBConf)))
					return false;
			}

			_psqdDb = _charDb;
			if (psqdDBConf->getBool("Use",false))
			{
				Poco::Logger& psqdDBLogger = Poco::Logger::get("PlayerSquadDB");
				_psqdDb = DatabaseLoader::Create(psqdDBConf);
				if (!_psqdDb->initialise(psqdDBLogger,DatabaseLoader::MakeConnParams(psqdDBConf)))
					return false;
			}
			_qstDb = _charDb;
			if (qstDBConf->getBool("Use",false))
			{
				Poco::Logger& qstDBLogger = Poco::Logger::get("QuestDB");
				_qstDb = DatabaseLoader::Create(qstDBConf);
				if (!_qstDb->initialise(qstDBLogger,DatabaseLoader::MakeConnParams(qstDBConf)))
					return false;
			}
			_pqstDb = _charDb;
			if (pqstDBConf->getBool("Use",false))
			{
				Poco::Logger& pqstDBLogger = Poco::Logger::get("PlayerQuestDB");
				_pqstDb = DatabaseLoader::Create(pqstDBConf);
				if (!_pqstDb->initialise(pqstDBLogger,DatabaseLoader::MakeConnParams(pqstDBConf)))
					return false;
			}

			_antiHackDb = _charDb;
			if (antiHackDBConf->getBool("Use",false))
			{
				Poco::Logger& antiHackDBLogger = Poco::Logger::get("AntiHackDB");
				_antiHackDb = DatabaseLoader::Create(antiHackDBConf);
				if (!_antiHackDb->initialise(antiHackDBLogger,DatabaseLoader::MakeConnParams(antiHackDBConf)))
					return false;
			}

			_messagingDb = _charDb;
			if (messagingDBConf->getBool("Use",false))
			{
				Poco::Logger& messagingDBLogger = Poco::Logger::get("MessagingDB");
				_messagingDb = DatabaseLoader::Create(messagingDBConf);
				if (!_messagingDb->initialise(messagingDBLogger,DatabaseLoader::MakeConnParams(messagingDBConf)))
					return false;
			}

		}
		catch (const DatabaseLoader::CreationError& e) 
		{
			logger().critical(e.displayText());
			return false;
		}
	}

	//Create character datasource
	{
		static const string defaultID = "PlayerUID";
		static const string defaultWS = "Worldspace";

		Poco::AutoPtr<Poco::Util::AbstractConfiguration> charDBConf(config().createView("Characters"));
		_charData.reset(new SqlCharDataSource(logger(),_charDb,charDBConf->getString("IDField",defaultID),charDBConf->getString("WSField",defaultWS)));	
	}

	//Create object datasource
	{
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> objConf(config().createView("Objects"));
		_objData.reset(new SqlObjDataSource(logger(),_objDb,objConf.get()));
	}

	//Create antihack datasource
	{
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> antiHackConf(config().createView("AntiHack"));
		_antihackData.reset(new SqlAntiHackDataSource(logger(),_antiHackDb,antiHackConf.get()));
	}

	//Building Datasource
	//Create object datasource
	{
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> bldConf(config().createView("Buildings"));
		_bldData.reset(new SqlBuildingDataSource(logger(),_bldDb,bldConf.get()));
	}
	
	//Instance Datasource
	{
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> instConf(config().createView("Instance"));
		_instData.reset(new SqlInstanceDataSource(logger(),_instDb,instConf.get()));
	}
	
	//Squad Datasource
	{
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> sqdConf(config().createView("Squad"));
		_sqdData.reset(new SqlSquadDataSource(logger(),_sqdDb,sqdConf.get()));
	}

	//PlayerSquad Datasource
	{
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> psqdConf(config().createView("PlayerSquad"));
		_psqdData.reset(new SqlSquadDataSource(logger(),_psqdDb,psqdConf.get()));
	}

	//PlayerQuest Datasource
	{
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> qstConf(config().createView("PlayerSquad"));
		_plyQstData.reset(new SqlQuestDataSource(logger(),_qstDb,qstConf.get()));
	}

	//Quest Datasource
	{
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> qstConf(config().createView("Quest"));
		_qstData.reset(new SqlQuestDataSource(logger(),_pqstDb,qstConf.get()));
	}

	//Messaging Datasource
	{
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> messagingConf(config().createView("Messaging"));
		_msgData.reset(new SqlMessagingDataSource(logger(),_messagingDb,messagingConf.get()));
	}
	//Create custom datasource
	_customData.reset(new CustomDataSource(logger(),_charDb,_objDb));

	_charDb->allowAsyncOperations();	
	if (_objDb != _charDb)
		_objDb->allowAsyncOperations();
	
	return true;
}
