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

#include "SqlMessagingDataSource.h"
#include "Database/Database.h"

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
using boost::lexical_cast;
using boost::bad_lexical_cast;
#include <Poco/Util/AbstractConfiguration.h>

SqlMessagingDataSource::SqlMessagingDataSource( Poco::Logger& logger, shared_ptr<Database> db, const Poco::Util::AbstractConfiguration* conf ) : SqlDataSource(logger,db)
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
void SqlMessagingDataSource::populateMessages( int serverId, ServerMessagingQueue& queue )
{

	auto messageRes = getDB()->queryParams("select payload, loop_interval, start_delay from message where instance_id = '%d'", serverId);
	if (!messageRes)
	{
		_logger.error("Failed to fetch objects from database");
		return;
	}
	while (messageRes->fetchRow())
	{
		auto row = messageRes->fields();

		Sqf::Parameters msgParams;
		//msgParams.push_back(string("OBJ"));

		string payload = row[0].getString();
		string loopinterval = row[1].getString();
		string startdelay = row[2].getString();

			try
			{
				
				msgParams.push_back(lexical_cast<string>(payload));
				msgParams.push_back(lexical_cast<string>(loopinterval));
				msgParams.push_back(lexical_cast<string>(startdelay));
			}
			
		catch (const bad_lexical_cast&)
		{
			_logger.error("Skipping MessageID " + lexical_cast<string>(payload) + " load because of invalid data in db");
			continue;
		}

		queue.push(msgParams);
	}
}

