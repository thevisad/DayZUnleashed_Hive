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

#include "SqlAntiHackDataSource.h"
#include "Database/Database.h"
#include <Poco/Util/AbstractConfiguration.h>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
using boost::lexical_cast;
using boost::bad_lexical_cast;


SqlAntiHackDataSource::SqlAntiHackDataSource( Poco::Logger& logger, shared_ptr<Database> db, const Poco::Util::AbstractConfiguration* conf ) : SqlDataSource(logger,db)
{
	static const string defaultTable = "antihack_admins"; 
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

void SqlAntiHackDataSource::fetchAntiHackAdmins(  int serverId, int adminlevel, AntiHackQueue& queue )
{

	auto antiHackRes = getDB()->queryParams(
			"SELECT antihack_admins.playerID FROM antihack_admins WHERE antihack_admins.instance = %d and antihack_admins.adminlevel = %d", serverId, adminlevel);
	
	if (!antiHackRes)
	{
		_logger.error("Failed to fetch admins from database");
		return;
	}
	string adminId;
	while (antiHackRes->fetchRow())
	{
		Sqf::Parameters ahParams;
		auto row = antiHackRes->fields();
		//ahParams.push_back(string("ADMIN"));

		try
		{
			adminId = row[0].getString(); //objectId should be stringified 
			ahParams.push_back(adminId);
			_logger.information("Pushed Admin (" + lexical_cast<string>(adminId) + ")");
		}
			
		catch (const bad_lexical_cast&)
		{
			_logger.error("Skipping Admin " + lexical_cast<string>(adminId) + " load because of invalid data in db");
			continue;
		}
		queue.push(ahParams);
	}
	
}

void SqlAntiHackDataSource::fetchAntiHackBans( AntiHackQueue& queue )
{
	
	auto antiHackRes = getDB()->queryParams(
			"SELECT antihack_bans.playerID FROM antihack_bans");
	
	if (!antiHackRes)
	{
		_logger.error("Failed to fetch bans from database");
		return;
	}
	
	string banId;
	while (antiHackRes->fetchRow())
	{
		Sqf::Parameters ahParams;
		auto row = antiHackRes->fields();
		//ahParams.push_back(string("BANS"));

		try
		{
			banId = row[0].getString(); //objectId should be stringified 
			ahParams.push_back(banId);
			_logger.information("Pushed Bans (" + lexical_cast<string>(banId) + ")");
		}
			
		catch (const bad_lexical_cast&)
		{
			_logger.error("Skipping Ban " + lexical_cast<string>(banId) + " load because of invalid data in db");
			continue;
		}
		queue.push(ahParams);
	}
	

}


void SqlAntiHackDataSource::fetchAntiHackWhitelist( AntiHackQueue& queue )
{
	
	auto antiHackRes = getDB()->queryParams(
			"SELECT antihack_whitelist.playerID FROM antihack_whitelist");
	
	if (!antiHackRes)
	{
		_logger.error("Failed to fetch whitelist from database");
		return;
	}
	string banId;
	while (antiHackRes->fetchRow())
	{
		Sqf::Parameters ahParams;
		auto row = antiHackRes->fields();
		//ahParams.push_back(string("BANS"));

		try
		{
			banId = row[0].getString(); //objectId should be stringified 
			ahParams.push_back(banId);
			_logger.information("Pushed whitelists (" + lexical_cast<string>(banId) + ")");
		}
			
		catch (const bad_lexical_cast&)
		{
			_logger.error("Skipping whitelist " + lexical_cast<string>(banId) + " load because of invalid data in db");
			continue;
		}
		queue.push(ahParams);
	}
}

