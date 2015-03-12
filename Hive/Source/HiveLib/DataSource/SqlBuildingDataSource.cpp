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
#include <boost/algorithm/string.hpp>
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
	static const string defaultObjectTable = "Object_DATA"; 

	if (conf != NULL)
	{
		_objTableName = getDB()->escape(conf->getString("Table", defaultObjectTable));
	}
	else
	{
		_objTableName = defaultObjectTable;
	}
}


void SqlBuildingDataSource::populateBuildings( int serverId, ServerBuildingsQueue& queue )
{

	auto worldBuildRes = getDB()->queryParams("SELECT building.class_name,instance_building.objectUID,character_data.CharacterID,instance_building.worldspace,instance_building.inventory, "
		"instance_building.hitpoints,instance_building.squadId,instance_building.combination FROM building INNER JOIN instance_building ON instance_building.buildingId = building.id "
		"LEFT JOIN character_data ON instance_building.characterId = character_data.PlayerUID WHERE instance_building.instanceId = '%d' AND character_data.Alive = 1", serverId);

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

		string classname = row[0].getString();
		string objectId = row[1].getString();
		bldParams.push_back(lexical_cast<string>(classname));
			try
			{
				
				bldParams.push_back(objectId); //objectId should be stringified 
				bldParams.push_back(lexical_cast<string>(row[2].getInt32())); //ownerId should be stringified
				Sqf::Value worldSpace = lexical_cast<Sqf::Value>(row[3].getString());

				_logger.information("Pushed BuildingID (" + objectId + ") class name (" + classname + ") with owner  (" + row[2].getString() + ")");
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
				string objectCombination = row[7].getString();
				bldParams.push_back(objectCombination);
			}
			
		catch (const bad_lexical_cast&)
		{
			_logger.error("Skipping BuildingID " + lexical_cast<string>(objectId) + " load because of invalid data in db");
			continue;
		}

		queue.push(bldParams);
	}
}



void SqlBuildingDataSource::populateGarages(int serverId, ServerGaragesQueue& queue)
{
	auto garageResource = getDB()->queryParams("SELECT instance_garagebuilding.objectUID, instance_garagebuilding.buildingClass, instance_garagebuilding.worldspace, "
		"instance_garagebuilding.playerId,instance_garagebuilding.id FROM instance_garagebuilding WHERE instance_garagebuilding.instanceId = %d", serverId );

	if (!garageResource)
	{
		_logger.error("Failed to fetch objects from database");
		return;
	}
	string objectId;
	
	while (garageResource->fetchRow())
	{
		
		auto row = garageResource->fields();
		Sqf::Parameters grgParams;
		string vehicleClasses = "[]";
		//grgParams.push_back(string("PASS"));
		try
		{
			objectId = row[0].getString();
			grgParams.push_back(lexical_cast<string>(objectId));
			string buildingClass = row[1].getString();
			grgParams.push_back(buildingClass);
			string wspace = row[2].getString();
			Sqf::Value worldSpace = lexical_cast<Sqf::Value>(wspace);
			grgParams.push_back(worldSpace);
			string charID = row[3].getString();
			grgParams.push_back(charID); //ownerId should be stringified
			string garageID = row[4].getString();
			grgParams.push_back(garageID); //ownerId should be stringified

			_logger.information("Pushed GarageID (" + lexical_cast<string>(objectId)+") class name (" + lexical_cast<string>(buildingClass)+") with owner  (" + lexical_cast<string>(charID)+")");
		}
			
		catch (const bad_lexical_cast&)
		{
			_logger.error("Skipping BuildingID " + lexical_cast<string>(objectId)+" load because of invalid data in db");
		}
		queue.push(grgParams);
		
	}
	
}

void SqlBuildingDataSource::fetchVehicleArray(int serverId, string garageID, ServerGaragesQueue& queue)
{
	auto garageVehicleResource = getDB()->queryParams("SELECT ObjectUID, Classname FROM instance_garage WHERE buildingUID = %s AND Instance = %d", getDB()->escape(garageID).c_str(), serverId);
	string vehicleClasses;
	string vehicleUID;

	if (!garageVehicleResource)
	{
		_logger.error("Failed to fetch objects from database");
		return;
	}
	while (garageVehicleResource->fetchRow())
	{
		auto vehiclerow = garageVehicleResource->fields();
		Sqf::Parameters grgVehParams;
		try {
			vehicleUID = vehiclerow[0].getString();
			vehicleClasses = vehiclerow[1].getString();
			grgVehParams.push_back(vehicleUID);
			grgVehParams.push_back(vehicleClasses);
		}
		catch (const bad_lexical_cast&)
		{
			_logger.error("Skipping BuildingID " + lexical_cast<string>(garageID)+" load because of invalid data in db");
		}
		queue.push(grgVehParams);
	}

}

bool SqlBuildingDataSource::deleteGarageVehicle(int serverId, string vehicleUid, string garageID)
{
	unique_ptr<SqlStatement> createbuilding;
	createbuilding = getDB()->makeStatement(_stmtDeleteGarageVehicle,
		"insert into object_data ( object_data.ObjectID, object_data.ObjectUID, object_data.Instance, "
		"object_data.Classname, object_data.Datestamp, object_data.CharacterID, object_data.Worldspace, "
		"object_data.Inventory, object_data.Hitpoints, object_data.Fuel, object_data.Damage, object_data.last_updated) "
		"SELECT instance_garage.ObjectID, instance_garage.ObjectUID, instance_garage.Instance, instance_garage.Classname, instance_garage.Datestamp, instance_garage.CharacterID, instance_garage.Worldspace, "
		"instance_garage.Inventory, instance_garage.Hitpoints, instance_garage.Fuel, instance_garage.Damage, instance_garage.last_updated FROM instance_garage "
		"where  instance_garage.ObjectUID = ? and instance_garage.Instance = ? and instance_garage.buildingUID = ?;");

	createbuilding->addString(vehicleUid);
	createbuilding->addInt32(serverId);
	createbuilding->addString(garageID);
	createbuilding->execute();

	unique_ptr<SqlStatement> stmt;
	stmt = getDB()->makeStatement(_stmtInsertVehicleByUID, "DELETE FROM instance_garage WHERE ObjectUID = ? AND Instance = ?");
	stmt->addString(vehicleUid);
	stmt->addInt32(serverId);

	bool exRes = stmt->execute();
	poco_assert(exRes == true);

	return exRes;
}



bool SqlBuildingDataSource::updateBuildingInventory( int serverId, Int64 objectIdent, const Sqf::Value& inventory )
{
	unique_ptr<SqlStatement> stmt;
	stmt = getDB()->makeStatement(_stmtUpdateBuildingbyUID, "UPDATE `instance_building` SET `Inventory` = ? WHERE `ObjectUID` = ? AND `instanceID` = ?");
	stmt->addString(lexical_cast<string>(inventory));
	stmt->addInt64(objectIdent);
	stmt->addInt32(serverId);

	bool exRes = stmt->execute();
	poco_assert(exRes == true);

	return exRes;
}


bool SqlBuildingDataSource::deleteBuilding( int serverId, Int64 objectIdent)
{
	unique_ptr<SqlStatement> stmt;
	stmt = getDB()->makeStatement(_stmtDeleteBuildingByUID, "DELETE FROM `instance_building` WHERE `ObjectUID` = ? AND `instanceID` = ?");
	stmt->addInt64(objectIdent);
	stmt->addInt32(serverId);

	bool exRes = stmt->execute();
	poco_assert(exRes == true);

	return exRes;
}

bool SqlBuildingDataSource::createBuilding(int serverId, const string& className, Int64 buildingUid, const Sqf::Value& worldSpace, const Sqf::Value& inventory, const Sqf::Value& hitPoints, int characterId, int squadId, int combinationId)
{
	auto createbuilding = getDB()->makeStatement(_stmtCreateBuilding, 
		"INSERT INTO `instance_building` ( `objectUID`, `instanceId`, `buildingId`, `worldspace`, `inventory`, `hitpoints`, `characterid`, `squadId`,`combination`, `created`) "
		"VALUES (?, ?, (SELECT building.id FROM building where building.class_name = ?), ?, ?, ?, (SELECT character_data.PlayerUID FROM character_data WHERE character_data.CharacterID = ?), ?, ?, CURRENT_TIMESTAMP)");

	
	_logger.information("HIVE: Building Insert " + lexical_cast<string>(buildingUid) + ":"+lexical_cast<string>(serverId) + ":" +lexical_cast<string>(className) + ":" +lexical_cast<string>(worldSpace) + ":"+lexical_cast<string>(inventory) + ":"+lexical_cast<string>(hitPoints) + ":"+lexical_cast<string>(characterId) + ":"+lexical_cast<string>(squadId) + ":"+lexical_cast<string>(combinationId)+ ":");
	//_logger.error("HIVE: Statement " + lexical_cast<string>(stmt)+ ":");

	createbuilding->addInt64(buildingUid);
	createbuilding->addInt32(serverId);
	createbuilding->addString(className);
	createbuilding->addString(lexical_cast<string>(worldSpace));
	createbuilding->addString(lexical_cast<string>(inventory));
	createbuilding->addString(lexical_cast<string>(hitPoints));
	createbuilding->addInt32(characterId);
	createbuilding->addInt32(squadId);
	createbuilding->addInt32(combinationId);
	bool exRes = createbuilding->execute();
	poco_assert(exRes == true);

	return exRes;
}

bool SqlBuildingDataSource::createGarage(int serverId, const string& className, string buildingUid, const Sqf::Value& worldSpace, string characterId)
{
	auto createbuilding = getDB()->makeStatement(_stmtCreateBuilding,
		"INSERT INTO `instance_garagebuilding` ( `objectUID`, `instanceId`, `buildingClass`, `worldspace`, `playerId`, `created`) "
		"VALUES (?, ?, ?, ?, ?, CURRENT_TIMESTAMP)");


	_logger.information("HIVE: Building Insert " + lexical_cast<string>(buildingUid));
	//_logger.error("HIVE: Statement " + lexical_cast<string>(stmt)+ ":");

	createbuilding->addString(buildingUid);
	createbuilding->addInt32(serverId);
	createbuilding->addString(className);
	createbuilding->addString(lexical_cast<string>(worldSpace));
	createbuilding->addString(characterId);
	bool exRes = createbuilding->execute();
	poco_assert(exRes == true);

	return exRes;
}


bool SqlBuildingDataSource::garageInsertion(int serverId, string vehicleUid, string garageid)
{
	auto createbuilding = getDB()->makeStatement(_stmtInsertGarageVehicle,
		"insert into instance_garage (instance_garage.buildingUID, instance_garage.ObjectID, instance_garage.ObjectUID, instance_garage.Instance, "
		"instance_garage.Classname, instance_garage.Datestamp, instance_garage.CharacterID, instance_garage.Worldspace, "
		"instance_garage.Inventory, instance_garage.Hitpoints, instance_garage.Fuel, instance_garage.Damage, instance_garage.last_updated) "
		"SELECT ?, object_data.ObjectID, object_data.ObjectUID, object_data.Instance, object_data.Classname, object_data.Datestamp, object_data.CharacterID, object_data.Worldspace, "
		"object_data.Inventory, object_data.Hitpoints, object_data.Fuel, object_data.Damage, object_data.last_updated FROM object_data "
		"where object_data.ObjectUID = ? and object_data.Instance = ?;");

	createbuilding->addString(garageid);
	createbuilding->addString(vehicleUid);
	createbuilding->addInt32(serverId);
	bool exRes = createbuilding->execute();
	poco_assert(exRes == true);


	unique_ptr<SqlStatement> delstmt;
	delstmt = getDB()->makeStatement(_stmtDeleteWorldVehicle, "DELETE FROM Object_data WHERE ObjectUID = ? AND Instance = ?");
	delstmt->addString(vehicleUid);
	delstmt->addInt32(serverId);
	delstmt->execute();

	return exRes;
}

