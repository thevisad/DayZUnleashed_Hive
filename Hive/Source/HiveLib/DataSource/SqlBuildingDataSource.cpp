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

#include "SqlBuildingDataSource.h"
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

SqlBuildingDataSource::SqlBuildingDataSource( Poco::Logger& logger, shared_ptr<Database> db, const Poco::Util::AbstractConfiguration* conf ) : SqlDataSource(logger,db)
{
	static const string defaultTable = "instance_buildings"; 
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


void SqlBuildingDataSource::populateBuildings( int serverId, ServerBuildingsQueue& queue )
{

	auto worldBuildRes = getDB()->queryParams("SELECT instance_building.objectUID, building.class_name, instance_building.characterId, instance_building.worldspace, instance_building.inventory, instance_building.hitpoints, instance_building.squadId, instance_building.combination FROM building INNER JOIN instance_building ON instance_building.buildingId = building.id WHERE instance_building.instanceId = '%d'", serverId);

	if (!worldBuildRes)
	{
		_logger.error("Failed to fetch objects from database");
		return;
	}
	while (worldBuildRes->fetchRow())
	{
		auto row = worldBuildRes->fields();

		Sqf::Parameters bldParams;
		//bldParams.push_back(string("OBJ"));

		int objectId = row[0].getInt32();
		bldParams.push_back(lexical_cast<string>(objectId));
			try
			{
				bldParams.push_back(row[1].getString()); //objectId should be stringified 
				bldParams.push_back(lexical_cast<string>(row[2].getInt32())); //ownerId should be stringified
				Sqf::Value worldSpace = lexical_cast<Sqf::Value>(row[3].getString());

				_logger.information("Pushed BuildingID (" + lexical_cast<string>(objectId) + ") class name (" + row[1].getString() + ") tp position  (" + row[2].getString() + ")");
				bldParams.push_back(worldSpace);
				//Inventory can be NULL
				{
					string invStr = "[]";
					if (!row[4].isNull())
						invStr = row[4].getString();

					bldParams.push_back(lexical_cast<Sqf::Value>(invStr));
				}	
				bldParams.push_back(lexical_cast<Sqf::Value>(row[5].getCStr()));
				bldParams.push_back(row[6].getInt32());
				bldParams.push_back(row[7].getInt32());
			}
			
		catch (const bad_lexical_cast&)
		{
			_logger.error("Skipping BuildingID " + lexical_cast<string>(objectId) + " load because of invalid data in db");
			continue;
		}

		queue.push(bldParams);
	}
}

/*
bool SqlBuildingDataSource::updateObjectInventory( int serverId, Int64 objectIdent, bool byUID, const Sqf::Value& inventory )
{
	unique_ptr<SqlStatement> stmt;
	if (byUID)
		stmt = getDB()->makeStatement(_stmtUpdateObjectbyUID, "UPDATE `"+_objTableName+"` SET `Inventory` = ? WHERE `ObjectUID` = ? AND `Instance` = ?");
	else
		stmt = getDB()->makeStatement(_stmtUpdateObjectByID, "UPDATE `"+_objTableName+"` SET `Inventory` = ? WHERE `ObjectID` = ? AND `Instance` = ?");

	stmt->addString(lexical_cast<string>(inventory));
	stmt->addInt64(objectIdent);
	stmt->addInt32(serverId);

	bool exRes = stmt->execute();
	poco_assert(exRes == true);

	return exRes;
}

bool SqlBuildingDataSource::deleteObject( int serverId, Int64 objectIdent, bool byUID )
{
	unique_ptr<SqlStatement> stmt;
	if (byUID)
		stmt = getDB()->makeStatement(_stmtDeleteObjectByUID, "DELETE FROM `"+_objTableName+"` WHERE `ObjectUID` = ? AND `Instance` = ?");
	else
		stmt = getDB()->makeStatement(_stmtDeleteObjectByID, "DELETE FROM `"+_objTableName+"` WHERE `ObjectID` = ? AND `Instance` = ?");

	stmt->addInt64(objectIdent);
	stmt->addInt32(serverId);

	bool exRes = stmt->execute();
	poco_assert(exRes == true);

	return exRes;
}*/

bool SqlBuildingDataSource::createBuilding( int serverId, const string& className, Int64 buildingUid, const Sqf::Value& worldSpace, const Sqf::Value& inventory, const Sqf::Value& hitPoints, int characterId, int squadId, int combinationId )
{
	auto stmt = getDB()->makeStatement(_stmtCreateBuilding, 
		"INSERT INTO `instance_building` ( `objectUID`, `instanceId`, `buildingId`, `worldspace`, `inventory`, `hitpoints`, `characterid`, `squadId`,`combination`, `created`) "
		"VALUES ('?', '?', (SELECT building.id FROM building where building.class_name = '?'),  '?', '?', '?', '?', '?', '?', CURRENT_TIMESTAMP)");

	//_key = format["CHILD:400:%1:%2:%3:%4:%5:%6:%7:%8:%9:",dayZ_instance,_uid,_class,_charID,_worldspace, [],[],_squad ,_combination];
	
	_logger.information("HIVE: Building Insert " + lexical_cast<string>(buildingUid) + ":"+lexical_cast<string>(serverId) + ":" +lexical_cast<string>(className) + ":" +lexical_cast<string>(worldSpace) + ":"+lexical_cast<string>(inventory) + ":"+lexical_cast<string>(hitPoints) + ":"+lexical_cast<string>(characterId) + ":"+lexical_cast<string>(squadId) + ":"+lexical_cast<string>(combinationId)+ ":");
	//_logger.error("HIVE: Statement " + lexical_cast<string>(stmt)+ ":");

	stmt->addInt32(buildingUid);
	stmt->addInt32(serverId);
	stmt->addString(className);
	stmt->addString(lexical_cast<string>(worldSpace));
	stmt->addString(lexical_cast<string>(inventory));
	stmt->addString(lexical_cast<string>(hitPoints));
	stmt->addInt32(characterId);
	stmt->addInt32(squadId);
	stmt->addInt32(combinationId);
	bool exRes = stmt->execute();
	poco_assert(exRes == true);

	return exRes;
}
/*

bool SqlBuildingDataSource::createSquad( int serverId, const string& squadName )
{
	auto stmt = getDB()->makeStatement(_stmtCreateSquad, 
		"INSERT INTO `squad` (`instance_id`, `squad_name`) "
		"VALUES ('?' '?',CURRENT_TIMESTAMP )");

	stmt->addInt32(serverId);
	stmt->addString(squadName);
	bool exRes = stmt->execute();
	poco_assert(exRes == true);

	return exRes;
}

bool SqlBuildingDataSource::createPlayerSquad( int squadId, int characterId )
{
	auto stmt = getDB()->makeStatement(_stmtCreatePlayerSquad, 
		"INSERT INTO `instance_squad` (`squadId`, `CharacterID`) "
		"VALUES (?, ?, CURRENT_TIMESTAMP)");

	stmt->addInt32(squadId);
	stmt->addInt32(characterId);
	bool exRes = stmt->execute();
	poco_assert(exRes == true);

	return exRes;
}

bool SqlBuildingDataSource::createInstance( int serverId, const Sqf::Value& currentState, const Sqf::Value& worldSpace, const Sqf::Value& quests )
{
	auto stmt = getDB()->makeStatement(_stmtCreateInstance, 
		"INSERT INTO `instance_variables` (`instanceID`, `currentState`, `worldSpace`, `quests`) "
		"VALUES (?, ?, ?, ?, CURRENT_TIMESTAMP)");

	stmt->addInt32(serverId);
	stmt->addString(lexical_cast<string>(currentState));
	stmt->addString(lexical_cast<string>(worldSpace));
	stmt->addString(lexical_cast<string>(quests));
	bool exRes = stmt->execute();
	poco_assert(exRes == true);

	return exRes;
} */





