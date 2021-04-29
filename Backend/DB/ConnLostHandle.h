#pragma once

#include "../Server/Server.h"
#include <mariadb++/exceptions.hpp>
#include <spdlog/spdlog.h>


namespace DB {
	void db_reconnect();
}

static inline WebServer::Resource::CallbackReturn db_handle_connection_fail(
	WebServer::Resource::CallbackFunction code_to_call,
	WebServer::RequestHandle req, WebServer::ResponseHandle res
)
{
	try {
		return code_to_call(req, res);
	}
	catch (mariadb::exception::connection const&) {
		spdlog::error("Database connection lost. Reconnecting.");
		DB::db_reconnect();
	}
	return code_to_call(req, res);
}
