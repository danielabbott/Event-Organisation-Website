#include "Server.h"
#include <sstream>
#include <iostream>
#include <optional>
#include <spdlog/spdlog.h>
#include "../DB/DB.h"

using namespace std;

namespace WebServer {

// Resource::CallbackReturn response__unseen_notifications(RequestHandle req, ResponseHandle res)
// {
// 	auto my_id_ = get_logged_in_user_id(req, res);
// 	if (!my_id_.has_value()) {
// 		return { 401, string(), MimeType::ASCII };
// 	}
// 	auto my_id = my_id_.value();

// 	return DB::unseen_notifications(my_id);
// }


Resource::CallbackReturn response__mark_notifications_seen(RequestHandle req, ResponseHandle res)
{
	auto my_id_ = get_logged_in_user_id(req, res);
	if (!my_id_.has_value()) {
		return { 401, string(), MimeType::ASCII };
	}
	auto my_id = my_id_.value();

	if(!has_param(req, "latest")) {
		return { 400, string("Missing parameter: latest"), MimeType::ASCII };
	}

	uint32_t latest = stoul(get_param_value(req, "latest"));

	DB::mark_notifications_seen(my_id, latest);
	return { 200, string(), MimeType::ASCII };
}

Resource::CallbackReturn response__notifications(RequestHandle req, ResponseHandle res)
{
	auto my_id_ = get_logged_in_user_id(req, res);
	if (!my_id_.has_value()) {
		return { 401, string(), MimeType::ASCII };
	}
	auto my_id = my_id_.value();

	// Get notifications with id > this
	optional<uint32_t> from_id;
	if(has_param(req, "from_id")) {
		from_id = stoul(get_param_value(req, "from_id"));
	}


	auto [notifications, prev_last_seen_id] = DB::get_notifications_for_user(my_id, from_id);

	if(notifications.size() == 0) {
		stringstream ss;
		ss << "{\"prevLastSeenID\": " << prev_last_seen_id << '}';
		return { 200, ss.str(), MimeType::JSON };
	}

	stringstream ss;
	ss << "{\"prevLastSeenID\": " << prev_last_seen_id << ", \"notifications\":[";

	for(auto const& n : notifications) {
		ss << "{\"id\":" << n.id << ','
		<< "\"timeSince\": " << n.time_since << ',';


		if(n.type == DB::Notification::Type::NewEvent) {
			ss << "\"type\":\"NewEvent\",";
		}
		else if(n.type == DB::Notification::Type::EventChanged) {
			ss << "\"type\":\"EventChanged\",";
		}
		else if(n.type == DB::Notification::Type::EventDeleted) {
			ss << "\"type\":\"EventDeleted\",";
		}
		else {
			ss << "\"type\":\"\",";
		}

		if(n.organiser_name.has_value()) {
			ss << "\"organiserName\":\"";
			json_filter_string(ss, n.organiser_name.value());
			ss << "\",";
		}
		if(n.event_name.has_value()) {
			ss << "\"eventName\":\"";
			json_filter_string(ss, n.event_name.value());
			ss << "\",";
		}
		if(n.event_id.has_value()) {
			ss << "\"eventID\":\"" << n.event_id.value() << "\",";
		}
		ss.seekp(-1, ios_base::end);
		ss << "},";
	}
	ss.seekp(-1, ios_base::end);
	ss << "]}";
	return { 200, ss.str(), MimeType::JSON };
}

}