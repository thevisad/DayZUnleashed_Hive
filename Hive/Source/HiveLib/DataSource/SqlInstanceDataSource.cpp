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

#include "SqlInstanceDataSource.h"
#include "Database/Database.h"

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
using boost::lexical_cast;
using boost::bad_lexical_cast;

namespace
{
	typedef boost::optional<Sqf::Value> PositionInfo;
	class PositionFixerVisitor : public boost::static_visitor<PositionInfo>
	{
	public:
		PositionInfo operator()(Sqf::Parameters& pos) const 
		{ 
			if (pos.size() != 3)
				return PositionInfo();

			try
			{
				double x = Sqf::GetDouble(pos[0]);
				double y = Sqf::GetDouble(pos[1]);
				double z = Sqf::GetDouble(pos[2]);

				if (x < 0 || y > 15360)
				{
					PositionInfo fixed(pos);
					pos.clear();
					return fixed;
				}
			}
			catch(const boost::bad_get&) {}

			return PositionInfo();
		}
		template<typename T> PositionInfo operator()(const T& other) const	{ return PositionInfo(); }
	};

	class WorldspaceFixerVisitor : public boost::static_visitor<PositionInfo>
	{
	public:
		PositionInfo operator()(Sqf::Parameters& ws) const 
		{ 
			if (ws.size() != 2)
				return PositionInfo();

			return boost::apply_visitor(PositionFixerVisitor(),ws[1]);
		}
		template<typename T> PositionInfo operator()(const T& other) const	{ return PositionInfo(); }
	};

	PositionInfo FixOOBWorldspace(Sqf::Value& v) { return boost::apply_visitor(WorldspaceFixerVisitor(),v); }
};

#include <Poco/Util/AbstractConfiguration.h>

SqlInstanceDataSource::SqlInstanceDataSource( Poco::Logger& logger, shared_ptr<Database> db, const Poco::Util::AbstractConfiguration* conf ) : SqlDataSource(logger,db)
{
	static const string defaultTable = "instance_Instances"; 
	/*
	if (conf != NULL)
	{
		_objTableName = getDB()->escape(conf->getString("Table",defaultTable));
		_cleanupPlacedDays = conf->getInt("CleanupPlacedAfterDays",6);
		_vehicleOOBReset = conf->getBool("ResetOOBVehicles",false);
	}
	else
	{
		_objTableName = defaultTable;
		_cleanupPlacedDays = -1;
		_vehicleOOBReset = false;
	}*/
}


void SqlInstanceDataSource::populateInstances( int serverId, ServerInstancesQueue& queue )
{

	auto worldBuildRes = getDB()->queryParams("SELECT instance_Instance.objectUID, Instance.class_name, instance_Instance.characterId, instance_Instance.worldspace, instance_Instance.inventory, instance_Instance.hitpoints, instance_Instance.InstanceId, instance_Instance.combination FROM Instance INNER JOIN instance_Instance ON instance_Instance.InstanceId = Instance.id WHERE instance_Instance.instanceId = '%d'", serverId);

	if (!worldBuildRes)
	{
		_logger.error("Failed to fetch objects from database");
		return;
	}
	while (worldBuildRes->fetchRow())
	{
		auto row = worldBuildRes->fields();

		Sqf::Parameters instParams;
		//instParams.push_back(string("OBJ"));

		int objectId = row[0].getInt32();
		instParams.push_back(lexical_cast<string>(objectId));
			try
			{
				instParams.push_back(row[1].getString()); //objectId should be stringified 
				instParams.push_back(lexical_cast<string>(row[2].getInt32())); //ownerId should be stringified
				Sqf::Value worldSpace = lexical_cast<Sqf::Value>(row[3].getString());

				_logger.information("Pushed InstanceID (" + lexical_cast<string>(objectId) + ") class name (" + row[1].getString() + ") tp position  (" + row[2].getString() + ")");
				instParams.push_back(worldSpace);
				//Inventory can be NULL
				{
					string invStr = "[]";
					if (!row[4].isNull())
						invStr = row[4].getString();

					instParams.push_back(lexical_cast<Sqf::Value>(invStr));
				}	
				instParams.push_back(lexical_cast<Sqf::Value>(row[5].getCStr()));
				instParams.push_back(row[6].getInt32());
				instParams.push_back(row[7].getInt32());
			}
			
		catch (const bad_lexical_cast&)
		{
			_logger.error("Skipping InstanceID " + lexical_cast<string>(objectId) + " load because of invalid data in db");
			continue;
		}

		queue.push(instParams);
	}
}

void SqlInstanceDataSource::populatePlayerInstances( int serverId, ServerInstancesQueue& queue )
{

	auto worldBuildRes = getDB()->queryParams("SELECT instance_Instance.objectUID, Instance.class_name, instance_Instance.characterId, instance_Instance.worldspace, instance_Instance.inventory, instance_Instance.hitpoints, instance_Instance.InstanceId, instance_Instance.combination FROM Instance INNER JOIN instance_Instance ON instance_Instance.InstanceId = Instance.id WHERE instance_Instance.instanceId = '%d'", serverId);

	if (!worldBuildRes)
	{
		_logger.error("Failed to fetch objects from database");
		return;
	}
	while (worldBuildRes->fetchRow())
	{
		auto row = worldBuildRes->fields();

		Sqf::Parameters instParams;
		//instParams.push_back(string("OBJ"));

		int objectId = row[0].getInt32();
		instParams.push_back(lexical_cast<string>(objectId));
			try
			{
				instParams.push_back(row[1].getString()); //objectId should be stringified 
				instParams.push_back(lexical_cast<string>(row[2].getInt32())); //ownerId should be stringified
				Sqf::Value worldSpace = lexical_cast<Sqf::Value>(row[3].getString());

				_logger.information("Pushed InstanceID (" + lexical_cast<string>(objectId) + ") class name (" + row[1].getString() + ") tp position  (" + row[2].getString() + ")");
				instParams.push_back(worldSpace);
				//Inventory can be NULL
				{
					string invStr = "[]";
					if (!row[4].isNull())
						invStr = row[4].getString();

					instParams.push_back(lexical_cast<Sqf::Value>(invStr));
				}	
				instParams.push_back(lexical_cast<Sqf::Value>(row[5].getCStr()));
				instParams.push_back(row[6].getInt32());
				instParams.push_back(row[7].getInt32());
			}
			
		catch (const bad_lexical_cast&)
		{
			_logger.error("Skipping InstanceID " + lexical_cast<string>(objectId) + " load because of invalid data in db");
			continue;
		}

		queue.push(instParams);
	}
}



bool SqlInstanceDataSource::deleteInstance( int serverId, Int64 InstanceIdent, bool byUID )
{
	unique_ptr<SqlStatement> stmt;
	if (byUID)
		stmt = getDB()->makeStatement(_stmtDeleteInstanceByUID, "DELETE FROM `instance_Instance` WHERE `ObjectUID` = ? AND `Instance` = ?");
	else
		stmt = getDB()->makeStatement(_stmtDeleteInstanceByID, "DELETE FROM `instance_Instance` WHERE `ObjectID` = ? AND `Instance` = ?");

	stmt->addInt64(InstanceIdent);
	stmt->addInt32(serverId);

	bool exRes = stmt->execute();
	poco_assert(exRes == true);

	return exRes;
}

bool SqlInstanceDataSource::deletePlayerInstance( int serverId, Int64 playerInstanceIdent, bool byUID )
{
	unique_ptr<SqlStatement> stmt;
	if (byUID)
		stmt = getDB()->makeStatement(_stmtDeletePlayerInstanceByUID, "DELETE FROM `instance_Instance` WHERE `ObjectUID` = ? AND `Instance` = ?");
	else
		stmt = getDB()->makeStatement(_stmtDeletePlayerInstanceByID, "DELETE FROM `instance_Instance` WHERE `ObjectID` = ? AND `Instance` = ?");

	stmt->addInt64(playerInstanceIdent);
	stmt->addInt32(serverId);

	bool exRes = stmt->execute();
	poco_assert(exRes == true);

	return exRes;
}



bool SqlInstanceDataSource::createInstance( int serverId, const string& InstanceName )
{
	auto stmt = getDB()->makeStatement(_stmtCreateInstance, 
		"INSERT INTO `Instance` (`instance_id`, `Instance_name`) "
		"VALUES ('?' '?',CURRENT_TIMESTAMP )");

	stmt->addInt32(serverId);
	stmt->addString(InstanceName);
	bool exRes = stmt->execute();
	poco_assert(exRes == true);

	return exRes;
}

bool SqlInstanceDataSource::createPlayerInstance( int InstanceId, int characterId )
{
	auto stmt = getDB()->makeStatement(_stmtCreatePlayerInstance, 
		"INSERT INTO `instance_Instance` (`InstanceId`, `CharacterID`) "
		"VALUES (?, ?, CURRENT_TIMESTAMP)");

	stmt->addInt32(InstanceId);
	stmt->addInt32(characterId);
	bool exRes = stmt->execute();
	poco_assert(exRes == true);

	return exRes;
}





