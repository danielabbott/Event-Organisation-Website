#pragma once

#include <string>
#include <functional>
#include <tuple>
#include <optional>
#include "../Users.h"

namespace WebServer {

// Blocks
void start_server();

using RequestHandle = uintptr_t;
using ResponseHandle = uintptr_t;
using ReponseCode = uint16_t;

enum Method {
	Get, Post
};

enum MimeType {
	ASCII, JSON
};

struct Resource {
	Method method;
	std::string name; // e.g. /d/log_in.json

	using CallbackReturn = std::tuple<ReponseCode, std::string, MimeType>;
	using CallbackFunction = std::function<CallbackReturn (RequestHandle, ResponseHandle)>;

	// Returns HTTP response code and response
	CallbackFunction callback;
};

// GET: ?name=value
// POST: URI encoded data or multipart form data in body
bool has_param(RequestHandle, std::string const& name);
std::string get_param_value(RequestHandle, std::string const& name);

// HTTP headers (cookies etc.)
// Returns number of times the header appears
unsigned int has_header(RequestHandle, std::string const& name);
std::string get_header_value(RequestHandle, std::string const& name, unsigned int i = 0);
void set_header(ResponseHandle, std::string const& name, std::string const& value);


void log_out(RequestHandle req);
void set_session_cookie(ResponseHandle res, Users::SessionToken const&);
void find_session_cookie(RequestHandle req, char * session_string);
void clear_session_cookie(ResponseHandle res);
std::optional<Users::ID> get_logged_in_user_id(RequestHandle req, ResponseHandle res);
void json_filter_string(std::stringstream & ss, std::string const& s);

#ifndef SERVER_RESOURCES_CPP
extern const std::vector<Resource> resources;
#endif




};