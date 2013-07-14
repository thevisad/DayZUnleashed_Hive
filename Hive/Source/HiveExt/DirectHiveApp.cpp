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

bool DirectHiveApp::initialiseService()
{
	//Load up databases
	{
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> globalDBConf(config().createView("Database"));
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> objDBConf(config().createView("ObjectDB"));

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

	//Create custom datasource
	_customData.reset(new CustomDataSource(logger(),_charDb,_objDb));

	_charDb->allowAsyncOperations();	
	if (_objDb != _charDb)
		_objDb->allowAsyncOperations();
	
	return true;
}
