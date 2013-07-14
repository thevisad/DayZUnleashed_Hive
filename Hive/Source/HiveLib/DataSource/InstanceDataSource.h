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

class InstanceDataSource
{
public:
	virtual ~InstanceDataSource() {}

	typedef std::queue<Sqf::Parameters> ServerInstancesQueue;
	virtual void populateInstances( int serverId, ServerInstancesQueue& queue ) = 0;
	virtual void populatePlayerInstances( int serverId, ServerInstancesQueue& queue ) = 0;
	virtual bool deleteInstance( int serverId, Int64 InstanceIdent, bool byUID ) = 0;
	virtual bool deletePlayerInstance( int serverId, Int64 playerInstanceIdent, bool byUID ) = 0;
	virtual bool createPlayerInstance( int InstanceId, int characterId ) = 0;

	//Unleashed
	virtual bool createInstance( int serverId, const string& InstanceName ) = 0;
};