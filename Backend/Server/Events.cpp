#include "Server.h"
#include "../Email.h"
#include <sstream>
#include <iostream>
#include <spdlog/spdlog.h>
#include "../DB/DB.h"
#include "../Email.h"
#include "../ArrayPointer.h"

using namespace std;

namespace WebServer {


static void print_event_preview_json(DB::EventPreview const& e, stringstream & ss) {
	ss << "{"
	"\"id\":" << e.id << ","
	"\"title\":\"";
	json_filter_string(ss, e.title);
	ss << "\","
	"\"organiserId\":" << e.organiser_id << ","
	"\"location\":\"";
	json_filter_string(ss, e.location);
	ss << "\","
	"\"description\":\"";
	json_filter_string(ss, e.shortened_description);
	ss << "\","
	"\"dateTime\":\"" << e.datetime << "\","
	"\"timeZone\":" << e.time_zone << ","
	"\"duration\":" << e.duration << ",";

	if(e.cover_image.has_value()) {
		ss << "\"coverImage\":\"" << e.cover_image.value().to_string() << "\",";
	}

	ss << "\"isPublished\":" << e.is_published << "},"
	;
}

static void print_comments(stringstream & ss, vector<DB::Event::Comment> const& comments) {
	if(!comments.size()) return;

	ss << "\"comments\":[";
	for(unsigned int i = 0; i < comments.size(); i++) {
		auto const& c = comments[i];
		ss << "{\"id\": " << c.id <<
		",\"userID\": " << c.user_id <<
		",\"commentersName\": \"";
		json_filter_string(ss, c.commenter_name);
		if(c.commenter_profile_picture.has_value()) {
			ss << "\", \"commenterProfPic\":\"" << c.commenter_profile_picture.value().to_string();
		}
		ss << "\",\"text\": \"";
		json_filter_string(ss, c.text);
		ss << "\", \"ago\": " << c.seconds_ago;

		if(c.seconds_since_last_edit.has_value()) {
			ss << ", \"timeSinceLastEdit\": " << c.seconds_since_last_edit.value();
		}

		ss << ", \"numberOfReplies\": " << c.replies << "},";
	}
	// Remove ','
	ss.seekp(-1, ios_base::end);
	ss << "],";
}

static string print_event_json(DB::Event const& event);
static string print_event_json(DB::Event const& event) {
	stringstream ss;
	ss << "{\"name\":\"";
	json_filter_string(ss, event.name);
	ss << "\",";

	ss << "\"organiserName\":\"";
	json_filter_string(ss, event.organiser_name);
	ss << "\",";

	ss << "\"attending\": " << (event.is_attending ? "true" : "false") << ",";

	ss << "\"organiser_id\":" << event.organiser_id << ","

	"\"description\":\"";
	json_filter_string(ss, event.description);
	ss << "\",";

	if(!event.description2.empty()) {
		ss << "\"description2\":\"";
		json_filter_string(ss, event.description2);
		ss << "\",";
	}
	if(!event.url.empty()) {
		ss << "\"url\":\"";
		json_filter_string(ss, event.url);
		ss << "\",";
	}
	if(!event.youtube_video_id.empty()) {
		ss << "\"youtubeVideoCode\":\"";
		json_filter_string(ss, event.youtube_video_id);
		ss << "\",";
	}
	ss << "\"public\":" << (event.is_public ? "true" : "false") << ","
	"\"isPublished\":" << (event.is_published ? "true" : "false") << ","
	"\"attendeesListPrivate\":" << (event.attendees_list_private ? "true" : "false") << ","
	"\"date\":\"" << event.date << "\","
	"\"time\":\"" << event.time << "\","
	"\"timeZone\":" << event.time_zone << ","
	"\"durationHours\":" << (event.duration/60) << ","
	"\"durationMinutes\":" << (event.duration%60) << ",";

	if(event.country_code.has_value()) {
		ss << "\"countryCode\": " << static_cast<unsigned int>(event.country_code.value()) << ",";
	}
	ss << "\"country\":\"" << event.country << "\",";

	if(event.cover_image.has_value()) {
		ss << "\"coverImage\":\"" << event.cover_image.value().to_string() << "\",";
	}

	if(!event.address.empty()) {
		ss << "\"address\":\"";
		json_filter_string(ss, event.address);
		ss << "\",";
	}
	if(!event.post_code.empty()) {
		ss << "\"postCode\":\"";
		json_filter_string(ss, event.post_code);
		ss << "\",";
	}

	ss << "\"feedbackWindowDuration\":" << event.feedback_window_duration << ",";

	if(event.invitations.size() > 0) {
		ss << "\"invitations\": [";

		for(const string& email : event.invitations) {
			ss << '"' << email << "\",";
		}
		ss.seekp(-1, ios_base::end);


		ss << "],";
	}

	if(event.gps.has_value()) {
		ss << "\"gpsLat\":" << event.gps.value().lat << ","
		<< "\"gpsLong\":" << event.gps.value().lon << ',';
	}

	if(event.recent_comments.size()) {
		print_comments(ss, event.recent_comments);
	}

	if(event.all_media.size()) {
		ss << "\"media\":[";
		for(unsigned int i = 0; i < event.all_media.size(); i++) {
			auto const& m = event.all_media[i];
			ss << "{\"id\":\"" << m.id.to_string() <<
			"\",\"isCoverImage\": " << (m.is_cover_image ? "true" : "false") <<
			",\"fileName\": \"";
			json_filter_string(ss, m.file_name);
			ss << "\"},";
		}
		// Remove ','
		ss.seekp(-1, ios_base::end);
		ss << "],";
	}
	ss.seekp(-1, ios_base::end);
	
	ss << "}";
	return ss.str(); // Copies data into a new string object
};

Resource::CallbackReturn response__homepage_events(RequestHandle req, ResponseHandle res)
{
	(void)req;
	(void)res;
	
	auto events = DB::get_recent_events(get_param_value(req, "name"), get_param_value(req, "location"));


	stringstream ss;
	ss << "{\"events\":[";


	for(const auto& e : events) {
		print_event_preview_json(e, ss);
	}

	if(events.size() > 0) {
		ss.seekp(-1, ios_base::end);
	}
	ss << "]}";

	return { 200, ss.str(), MimeType::JSON };
}

Resource::CallbackReturn response__my_events(RequestHandle req, ResponseHandle res)
{
	auto my_id = get_logged_in_user_id(req, res);
	if (!my_id.has_value()) {
		return { 401, string(), MimeType::ASCII };
	}

	auto events = DB::get_users_events_previews(my_id.value());


	stringstream ss;
	ss << "{\"events\":[";


	for(const auto& e : events) {
		print_event_preview_json(e, ss);
	}

	if(events.size() > 0) {
		ss.seekp(-1, ios_base::end);
	}
	ss << "]}";

	return { 200, ss.str(), MimeType::JSON };
}

Resource::CallbackReturn response__event(RequestHandle req, ResponseHandle res)
{
	if (!has_param(req, "id")) {
		return { 400, string("Missing parameter: id"), MimeType::ASCII };
	}

	uint32_t event_id = stoul(get_param_value(req, "id"));
	auto user_id = get_logged_in_user_id(req, res);

	auto ev = DB::get_event(event_id, user_id);

	if (!ev.has_value()) {
		return { ev.get_http_error(), string(), MimeType::ASCII };
	}

	return { 200, print_event_json(ev.value()), MimeType::JSON };
}


Resource::CallbackReturn response__comments(RequestHandle req, ResponseHandle res)
{
	if (!has_param(req, "eventid")) {
		return { 400, string("Missing parameter: eventid"), MimeType::ASCII };
	}

	uint32_t start_id = 0xffffffff;
	if (has_param(req, "startid")) {
		start_id = stoul(get_param_value(req, "startid"));
	}

	uint32_t event_id = stoul(get_param_value(req, "eventid"));
	auto user_id = get_logged_in_user_id(req, res);

	auto comments = DB::get_comments(event_id, start_id, user_id);

	if (!comments.has_value()) {
		return { comments.get_http_error(), string(), MimeType::ASCII };
	}


	if(!comments.value().size()) {
		return { 200, "{}", MimeType::JSON };
	}

	stringstream ss;
	ss << "{";

	print_comments(ss, comments.value());
	ss.seekp(-1, ios_base::end);

	ss << "}";

	return { 200, ss.str(), MimeType::JSON };
}


Resource::CallbackReturn response__replies(RequestHandle req, ResponseHandle res)
{
	if (!has_param(req, "id")) {
		return { 400, string("Missing parameter: id"), MimeType::ASCII };
	}

	uint32_t id = stoul(get_param_value(req, "id"));
	auto user_id = get_logged_in_user_id(req, res);

	auto replies = DB::get_replies(id, user_id);

	if (!replies.has_value()) {
		return { replies.get_http_error(), string(), MimeType::ASCII };
	}


	if(!replies.value().size()) {
		return { 200, "{}", MimeType::JSON };
	}

	stringstream ss;

	ss << "{\"replies\":[";
	for(unsigned int i = 0; i < replies.value().size(); i++) {
		auto const& c = replies.value()[i];
		ss << "{\"id\": " << c.id <<
		",\"userID\": " << c.user_id <<
		",\"commentersName\": \"";
		json_filter_string(ss, c.commenter_name);
		if(c.commenter_profile_picture.has_value()) {
			ss << "\", \"commenterProfPic\":\"" << c.commenter_profile_picture.value().to_string();
		}
		ss << "\",\"text\": \"";
		json_filter_string(ss, c.text);
		ss << "\", \"ago\": " << c.seconds_ago;

		if(c.seconds_since_last_edit.has_value()) {
			ss << ", \"timeSinceLastEdit\": " << c.seconds_since_last_edit.value();
		}

		ss << "},";
	}

	// Remove ','
	ss.seekp(-1, ios_base::end);
	ss << "]}";

	return { 200, ss.str(), MimeType::JSON };
}

Resource::CallbackReturn response__post_comment(RequestHandle req, ResponseHandle res)
{
	if (!has_param(req, "text")) {
		return { 400, string("Missing parameter: text"), MimeType::ASCII };
	}

	uint32_t event_id = stoul(get_param_value(req, "event_id"));
	string text = get_param_value(req, "text");
	auto user_id = get_logged_in_user_id(req, res);

	if(!user_id.has_value()) {
		return { get_http_error(ErrorType::NotLoggedIn), string(), MimeType::ASCII };
	}

	auto id = DB::post_comment(event_id, user_id.value(), text);

	if(id.has_value()) {
		return { 200, to_string(id.value()), MimeType::ASCII };
	}
	return { id.get_http_error(), string(), MimeType::ASCII };

	
}

Resource::CallbackReturn response__post_reply(RequestHandle req, ResponseHandle res)
{
	if (!has_param(req, "comment_id")) {
		return { 400, string("Missing parameter: comment_id"), MimeType::ASCII };
	}
	if (!has_param(req, "text")) {
		return { 400, string("Missing parameter: text"), MimeType::ASCII };
	}

	uint32_t comment_id = stoul(get_param_value(req, "comment_id"));
	string text = get_param_value(req, "text");
	auto user_id = get_logged_in_user_id(req, res);

	if(!user_id.has_value()) {
		return { get_http_error(ErrorType::NotLoggedIn), string(), MimeType::ASCII };
	}

	auto id = DB::post_reply(comment_id, user_id.value(), text);

	if(id.has_value()) {
		return { 200, to_string(id.value()), MimeType::ASCII };
	}
	return { id.get_http_error(), string(), MimeType::ASCII };

	
}

Resource::CallbackReturn response__delete_comment(RequestHandle req, ResponseHandle res)
{
	if (!has_param(req, "id")) {
		return { 400, string("Missing parameter: id"), MimeType::ASCII };
	}
	uint32_t comment_id = stoul(get_param_value(req, "id"));
	
	auto user_id = get_logged_in_user_id(req, res);

	if(!user_id.has_value()) {
		return { get_http_error(ErrorType::NotLoggedIn), string(), MimeType::ASCII };
	}

	auto err = DB::delete_comment(comment_id, user_id.value());

	return { get_http_error(err), string(), MimeType::ASCII };
}

Resource::CallbackReturn response__delete_reply(RequestHandle req, ResponseHandle res)
{
	if (!has_param(req, "id")) {
		return { 400, string("Missing parameter: id"), MimeType::ASCII };
	}
	uint32_t comment_id = stoul(get_param_value(req, "id"));
	
	auto user_id = get_logged_in_user_id(req, res);

	if(!user_id.has_value()) {
		return { get_http_error(ErrorType::NotLoggedIn), string(), MimeType::ASCII };
	}

	auto err = DB::delete_reply(comment_id, user_id.value());

	return { get_http_error(err), string(), MimeType::ASCII };
}

Resource::CallbackReturn response__event_edit_info(RequestHandle req, ResponseHandle res)
{
	if (!has_param(req, "id")) {
		return { 400, string("Missing parameter: id"), MimeType::ASCII };
	}


	auto id = get_logged_in_user_id(req, res);
	if (!id.has_value()) {
		return { 401, string(), MimeType::ASCII };
	}

	DB::EventID event_id = stoul(get_param_value(req, "id"));
	auto ev = DB::get_edit_event_info(event_id, id.value());

	if(!ev.has_value() && ev.get_error() == ErrorType::PermissionsError) {
		string ip_string = get_header_value(req, "X-Real-IP");
		spdlog::warn("Authorisation failure fetching information for event edit page: user id [{}], event id [{}], ip [{}]", id.value(), event_id, ip_string);
	}

	if (!ev.has_value()) {
		return { ev.get_http_error(), string(), MimeType::ASCII };
	}

	return { 200, print_event_json(ev.value()), MimeType::JSON };
}

Resource::CallbackReturn response__save_event(RequestHandle req, ResponseHandle res)
{
	optional<uint32_t> event_id;
	if (has_param(req, "id")) {
		event_id = stoul(get_param_value(req, "id"));
	}

	auto logged_in_user_id_ = get_logged_in_user_id(req, res);
	if (!logged_in_user_id_.has_value()) { 
		return { 401, string(), MimeType::ASCII };
	}
	auto logged_in_user_id = logged_in_user_id_.value();

	unordered_map<DB::EventAttribute, DB::AttributeValue> attributes;


	
	#define GET_STRING(attrib, json_name) \
	if (has_param(req, json_name)) { \
		attributes[DB::EventAttribute::attrib] = DB::AttributeValue(get_param_value(req, json_name)); \
	}
	GET_STRING(Name, "name")
	GET_STRING(URL, "url")
	GET_STRING(Description, "description")
	GET_STRING(Description2, "description2")
	GET_STRING(Address, "address")
	GET_STRING(PostCode, "postCode")
	GET_STRING(YTVideoCode, "youtubeVideoCode")
	#undef GET_STRING

	#define GET_INT(attrib, data_type, json_name) \
	if (has_param(req, json_name)) { \
		attributes[DB::EventAttribute::attrib] = \
			DB::AttributeValue(static_cast<data_type>(stoll(get_param_value(req, json_name)))); \
	}
	GET_INT(TimeZone, int8_t, "timeZone")
	GET_INT(CountryCode, uint8_t, "countryCode")
	GET_INT(FeedBackWindowDuration, uint8_t, "feedbackWindowDuration")
	#undef GET_INT

	#define GET_BOOL(attrib, json_name) \
	if (has_param(req, json_name)) { \
		auto s = get_param_value(req, json_name); \
		attributes[DB::EventAttribute::attrib] = \
			DB::AttributeValue(s.size() == 1 && s[0] == '1'); \
	}
	GET_BOOL(IsPublic, "public")
	GET_BOOL(AttendeesListPrivate, "attendeesListPrivate")
	#undef GET_BOOL

	#define GET_FLOAT(attrib, json_name) \
	if (has_param(req, json_name)) { \
		attributes[DB::EventAttribute::attrib] = \
			DB::AttributeValue(stof(get_param_value(req, json_name))); \
	}
	GET_FLOAT(GPSLattitude, "gpsLat")
	GET_FLOAT(GPSLongitude, "gpsLong")
	#undef GET_FLOAT

	if (has_param(req, "durationHours") && has_param(req, "durationMinutes")) {
		attributes[DB::EventAttribute::Duration] = DB::AttributeValue(
			stoul(get_param_value(req, "durationHours"))*60 + 
			stoul(get_param_value(req, "durationMinutes"))
		);
	}
	else if (has_param(req, "duration")) {
		attributes[DB::EventAttribute::Duration] = DB::AttributeValue(
			stoul(get_param_value(req, "duration"))
		);
	}

	if(has_param(req, "date") && has_param(req, "time")) {
		auto d = get_param_value(req, "date");
		auto t = get_param_value(req, "time");

		if(d.size() == 10 && d[4] == '-' && d[7] == '-'
			&& t.size() == 5 && t[2] == ':') {
			unsigned int yr = stoul(&d[0]);
			unsigned int month = stoul(&d[5]);
			unsigned int day = stoul(&d[8]);
			unsigned int hour = stoul(&t[0]);
			unsigned int minute = stoul(&t[3]);
			attributes[DB::EventAttribute::StartDateTime] = DB::AttributeValue(
				DB::DateTime(yr, month, day, hour, minute)
			);
		}
	}

	if (has_param(req, "publish")) {
		auto s = get_param_value(req, "publish");
		if(s.size() == 1 && s[0] == '1') {
			attributes[DB::EventAttribute::IsPublished] = DB::AttributeValue(true);
		}
	}

	vector<string> new_invites;
	vector<string> removed_invites;

	const auto add_to_list = [](vector<string> & email_list, string && invitee_list) {
		size_t pos = 0;
		while(pos+4 <= invitee_list.size()) {
			size_t end_pos = invitee_list.find(',', pos);
			if(end_pos == string::npos) end_pos = invitee_list.size();

			if(end_pos - pos >= 4) {
				string email(invitee_list, pos, end_pos-pos);
				if(Emails::email_is_valid(email.c_str())) {
					email_list.push_back(move(email));
				}
			}
			else break;

			pos = end_pos+1;
		}
	};

	if (has_param(req, "addInvitee")) {
		add_to_list(new_invites, get_param_value(req, "addInvitee"));
	}
	if (has_param(req, "removeInvitee")) {
		add_to_list(removed_invites, get_param_value(req, "removeInvitee"));
	}


	string ip_string = get_header_value(req, "X-Real-IP");
	if(event_id.has_value()) {
		auto err = DB::edit_event(event_id.value(), logged_in_user_id, attributes, new_invites, removed_invites);
		if(err == ErrorType::PermissionsError) {
			spdlog::warn("Authorisation failure saving changes to event: user id [{}], event id [{}], ip [{}]", logged_in_user_id, event_id.value(), ip_string);
		}
		else if(err == ErrorType::Success) {
			spdlog::info("Saved info: user id [{}], event id [{}], ip [{}]", logged_in_user_id, event_id.value(), ip_string);
		}

		return { get_http_error(err), string(), MimeType::ASCII };
	}
	else {
		event_id = DB::create_event(logged_in_user_id, attributes, new_invites);
		if(event_id.has_value()) {
			spdlog::info("Created event: user id [{}], event id [{}], ip [{}]", logged_in_user_id, event_id.value(), ip_string);

			return { 200, to_string(event_id.value()), MimeType::ASCII };
		}
		else {
			return { 500, string(), MimeType::ASCII };
		}
	}
	return { 200, string(), MimeType::ASCII };
}

Resource::CallbackReturn response__register_attendance(RequestHandle req, ResponseHandle res)
{
	if (!has_param(req, "id")) {
		return { 400, string("Missing parameter: id"), MimeType::ASCII };
	}

	uint32_t event_id = stoul(get_param_value(req, "id"));

	auto logged_in_user_id_ = get_logged_in_user_id(req, res);
	if (!logged_in_user_id_.has_value()) { 
		return { 401, string(), MimeType::ASCII };
	}
	auto const& logged_in_user_id = logged_in_user_id_.value();

	auto err = DB::register_attendance(event_id, logged_in_user_id);

	if(err == ErrorType::PermissionsError) {
		spdlog::warn("Authorisation failure registering attendance to event: user id [{}], event id [{}], ip [{}]", logged_in_user_id, event_id, get_header_value(req, "X-Real-IP"));
	}

	return { get_http_error(err), string(), MimeType::ASCII };
}

Resource::CallbackReturn response__unregister_attendance(RequestHandle req, ResponseHandle res)
{
	if (!has_param(req, "id")) {
		return { 400, string("Missing parameter: id"), MimeType::ASCII };
	}

	uint32_t event_id = stoul(get_param_value(req, "id"));

	auto logged_in_user_id_ = get_logged_in_user_id(req, res);
	if (!logged_in_user_id_.has_value()) { 
		return { 401, string(), MimeType::ASCII };
	}
	auto const& logged_in_user_id = logged_in_user_id_.value();

	DB::unregister_attendance(event_id, logged_in_user_id);
	return { 200, string(), MimeType::ASCII };
}

Resource::CallbackReturn response__attendees(RequestHandle req, ResponseHandle res)
{
	if (!has_param(req, "id")) {
		return { 400, string("Missing parameter: id"), MimeType::ASCII };
	}

	auto user_id = get_logged_in_user_id(req, res);

	uint32_t event_id = stoul(get_param_value(req, "id"));

	auto attendees = DB::get_event_attendees(event_id, user_id);
	if(!attendees.has_value()) {
		if(attendees.get_error() == ErrorType::PermissionsError) {
			string user_id_string;
			if(user_id.has_value()) user_id_string = to_string(user_id.value());
			spdlog::warn("Authorisation failure getting attendees list for event: user id [{}], event id [{}], ip [{}]", user_id_string, event_id, get_header_value(req, "X-Real-IP"));
		}

		return { attendees.get_http_error(), string(), MimeType::ASCII };
	}

	stringstream ss;
	ss << '{';
	if(attendees.value().size() > 0) {
		ss << "\"attendees\": [";

		for(const string& name : attendees.value()) {
			ss << "{\"name\":\"" << name << "\"},";
		}
		ss.seekp(-1, ios_base::end);


		ss << ']';
	}
	ss << '}';
	return { 200, ss.str(), MimeType::JSON };
}

Resource::CallbackReturn response__delete_event(RequestHandle req, ResponseHandle res)
{
	if (!has_param(req, "id")) {
		return { 400, string("Missing parameter: id"), MimeType::ASCII };
	}

	DB::EventID event_id = stoul(get_param_value(req, "id"));

	auto logged_in_user_id_ = get_logged_in_user_id(req, res);
	if (!logged_in_user_id_.has_value()) { 
		return { 401, string(), MimeType::ASCII };
	}
	auto const& logged_in_user_id = logged_in_user_id_.value();

	string ip_string = get_header_value(req, "X-Real-IP");

	auto err = DB::delete_event(event_id, logged_in_user_id);
	if(err == ErrorType::PermissionsError) {
		spdlog::warn("Authorisation failure deleting event: user id [{}], event id [{}], ip [{}]", logged_in_user_id, event_id, ip_string);
	}
	else if(err == ErrorType::Success) {
		spdlog::info("Deleted event: event id [{}], ip [{}]", event_id, ip_string);
	}

	return { get_http_error(err), string(), MimeType::ASCII };
}


Resource::CallbackReturn response__events_invited_to(RequestHandle req, ResponseHandle res)
{
	auto my_id = get_logged_in_user_id(req, res);
	if (!my_id.has_value()) {
		return { 401, string(), MimeType::ASCII };
	}

	auto events = DB::get_users_invited_events_previews(my_id.value());


	stringstream ss;
	ss << "{\"events\":[";


	for(const auto& e : events) {
		print_event_preview_json(e, ss);
	}

	if(events.size() > 0) {
		ss.seekp(-1, ios_base::end);
	}
	ss << "]}";

	return { 200, ss.str(), MimeType::JSON };
}

Resource::CallbackReturn response__upcoming_events(RequestHandle req, ResponseHandle res)
{
	auto my_id = get_logged_in_user_id(req, res);
	if (!my_id.has_value()) {
		return { 401, string(), MimeType::ASCII };
	}

	auto events = DB::get_users_upcoming_events_previews(my_id.value());


	stringstream ss;
	ss << "{\"events\":[";


	for(const auto& e : events) {
		print_event_preview_json(e, ss);
	}

	if(events.size() > 0) {
		ss.seekp(-1, ios_base::end);
	}
	ss << "]}";

	return { 200, ss.str(), MimeType::JSON };
}


Resource::CallbackReturn response__add_cover_image(RequestHandle req, ResponseHandle res)
{
	auto my_id = get_logged_in_user_id(req, res);
	if (!my_id.has_value()) {
		return { 401, string(), MimeType::ASCII };
	}

	if (!has_param(req, "event_id")) {
		return { 400, string("Missing parameter: event_id"), MimeType::ASCII };
	}

	DB::EventID event_id = stoul(get_param_value(req, "event_id"));

	if (!has_param(req, "file_name")) {
		return { 400, string("Missing parameter: file_name"), MimeType::ASCII };
	}

	string file_name = get_param_value(req, "file_name");

	auto x = DB::add_cover_image(event_id, my_id.value(), file_name);
	if(!x.has_value()) {
		return { x.get_http_error(), string(), MimeType::ASCII };
	}

	auto const& [id, presigned_url] = x.value();


	stringstream ss;
	ss << "{\"id\":\"" << id.to_string() << "\", \"presignedURL\":\"" << presigned_url << "\"}";
	return { 200, ss.str(), MimeType::JSON };
}

Resource::CallbackReturn response__delete_cover_image(RequestHandle req, ResponseHandle res)
{
	auto my_id = get_logged_in_user_id(req, res);
	if (!my_id.has_value()) {
		return { 401, string(), MimeType::ASCII };
	}

	if (!has_param(req, "event_id")) {
		return { 400, string("Missing parameter: event_id"), MimeType::ASCII };
	}

	DB::EventID event_id = stoul(get_param_value(req, "event_id"));

	ErrorType e = DB::delete_cover_image(event_id, my_id.value());
	return { get_http_error(e), string(), MimeType::ASCII };
}

Resource::CallbackReturn response__delete_media(RequestHandle req, ResponseHandle res)
{
	auto my_id = get_logged_in_user_id(req, res);
	if (!my_id.has_value()) {
		return { 401, string(), MimeType::ASCII };
	}

	if (!has_param(req, "id")) {
		return { 400, string("Missing parameter: id"), MimeType::ASCII };
	}

	string id = get_param_value(req, "id");

	ErrorType e = DB::delete_media(id, my_id.value());
	return { get_http_error(e), string(), MimeType::ASCII };
}

Resource::CallbackReturn response__add_media(RequestHandle req, ResponseHandle res)
{
	auto my_id = get_logged_in_user_id(req, res);
	if (!my_id.has_value()) {
		return { 401, string(), MimeType::ASCII };
	}

	if (!has_param(req, "file_name")) {
		return { 400, string("Missing parameter: file_name"), MimeType::ASCII };
	}

	string file_name = get_param_value(req, "file_name");

	if (!has_param(req, "event_id")) {
		return { 400, string("Missing parameter: event_id"), MimeType::ASCII };
	}

	DB::EventID event_id = stoul(get_param_value(req, "event_id"));

	auto x = DB::add_media(event_id, my_id.value(), file_name);
	if(!x.has_value()) {
		return { x.get_http_error(), string(), MimeType::ASCII };
	}


	auto const& [id, presigned_url] = x.value();


	stringstream ss;
	ss << "{\"id\":\"" << id.to_string() << "\", \"presignedURL\":\"" << presigned_url << "\"}";
	return { 200, ss.str(), MimeType::JSON };
}

};