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

#include "DataSource.h"

class BuildingDataSource
{
public:
	virtual ~BuildingDataSource() {}

	typedef std::queue<Sqf::Parameters> ServerBuildingsQueue;
	virtual void populateBuildings( int serverId, ServerBuildingsQueue& queue ) = 0;

	//virtual void populateGarageVehicles( int serverId, Int64 buildingUID  ) = 0;
	virtual bool deleteBuilding( int serverId, Int64 buildingIdent, bool byUID ) = 0;
	virtual bool updateBuildingInventory( int serverId, Int64 objectIdent, bool byUID, const Sqf::Value& inventory ) = 0;

	//Unleashed
	virtual bool createBuilding(int serverId, const string& className, Int64 buildingUid, const Sqf::Value& worldSpace, const Sqf::Value& inventory, const Sqf::Value& hitPoints, int characterId, int squadId, int combinationId) = 0;
};