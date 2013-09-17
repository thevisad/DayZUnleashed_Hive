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

#include "SqlDataSource.h"
#include "BuildingDataSource.h"
#include "Database/SqlStatement.h"

namespace Poco { namespace Util { class AbstractConfiguration; }; };
class SqlBuildingDataSource : public SqlDataSource, public BuildingDataSource
{
public:
	SqlBuildingDataSource(Poco::Logger& logger, shared_ptr<Database> db, const Poco::Util::AbstractConfiguration* conf);
	~SqlBuildingDataSource() {}

	void populateBuildings( int serverId, ServerBuildingsQueue& queue ) override;
	//void populateGarageVehicles( int serverId, Int64 buildingUID  ) override;
	bool updateBuildingInventory( int serverId, Int64 objectIdent, bool byUID, const Sqf::Value& inventory ) override;
	bool deleteBuilding( int serverId, Int64 buildingIdent, bool byUID ) override;
	bool createBuilding( int serverId, const string& className, Int64 buildingUid, const Sqf::Value& worldSpace, const Sqf::Value& inventory, const Sqf::Value& hitPoints, int characterId, int squadId, int combinationId ) override;
private:
	string _buildingTableName;

	SqlStatementID _stmtCreateBuilding;
	SqlStatementID _stmtUpdateBuildingbyUID;
	SqlStatementID _stmtUpdateBuildingByID;
	SqlStatementID _stmtDeleteBuildingByUID;
	SqlStatementID _stmtDeleteBuildingByID;
};