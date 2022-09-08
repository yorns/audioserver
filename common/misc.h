#ifndef COMMON_MISC_H
#define COMMON_MISC_H

#include <memory>

#include "webserver/sessionhandler.h"
#include "snc/client.h"
#include "playerinterface/Player.h"
#include "database/SimpleDatabase.h"
#include "logger.h"
#include "webserver/playeraccess.h"
#include "webserver/databaseaccess.h"

namespace Common {

void printPaths();

boost::uuids::uuid extractUUID (const std::string& uuidString);

void audioserver_updateUI(SessionHandler &sessionHandler,
                          std::shared_ptr<snc::Client> sncClient,
                          PlayerAccess& player,
                          DatabaseAccess &database);;

void audioserver_externalSelect(DatabaseAccess &database,
                                PlayerAccess &playerWrapper,
                                SessionHandler &sessionHandler,
                                std::shared_ptr<snc::Client> sncClient,
                                const std::string& other,
                                const std::string& raw_msg);


}

#endif
