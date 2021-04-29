#include "DB_.h"
#include "../AWS.h"
#include "../Random.h"

using namespace std;

extern vector<string> all_countries_list;

namespace DB {
	
static bool check_user_invited(EventID event_id, Users::ID user_id);
static vector<Users::ID> get_event_attendees_ids(EventID);
	
static vector<EventPreview> event_previews_from_result_set(result_set_ref const& result, bool show_is_published) {
	vector<EventPreview> events;

	while (result->next()) {
		EventPreview e;
		e.id = result->get_unsigned32("id");

		if(show_is_published)
			e.is_published = result->get_unsigned8("is_published");

		e.title = result->get_string("name");
		e.organiser_id = result->get_unsigned32("organiser_id");

		if(result->get_is_null("postcode")) {
			if(!result->get_is_null("url"))
				e.location = result->get_string("url");
		}
		else {
			e.location = result->get_string("postcode");
		}

		if(!result->get_is_null("description"))
			e.shortened_description = result->get_string("description");

		e.datetime = result->get_date_time("event_date_time").str();
		e.datetime.resize(16);

		e.time_zone = result->get_signed8("event_time_zone");
		e.duration = result->get_unsigned16("event_duration");

		if(!result->get_is_null("cover_image_id")) {
			MediaID em;
			data_ref dr = result->get_data("cover_image_id");
			memcpy(em.data, dr->get(), 17);
			e.cover_image = em;
		}

		events.push_back(move(e));
	}
	return events;
}


vector<EventPreview> get_recent_events(string const& name, string const& location)
{
	auto connection_data = open_database();
	result_set_ref result;

	if(name.size() && location.size()) {
		connection_data.stmnt_get_homepage_name_and_location->set_string(0, name);
		connection_data.stmnt_get_homepage_name_and_location->set_string(1, location);
		result = connection_data.stmnt_get_homepage_name_and_location->query();
	}
	else if(!name.size() && !location.size()) {
		result = connection_data.stmnt_recently_added_events->query();
	}
	else if(name.size()) {
		connection_data.stmnt_get_homepage_name->set_string(0, name);
		result = connection_data.stmnt_get_homepage_name->query();
	}
	else if(location.size()) {
		connection_data.stmnt_get_homepage_location->set_string(0, location);
		result = connection_data.stmnt_get_homepage_location->query();
	}

	return event_previews_from_result_set(result, false);
}

vector<EventPreview> get_users_events_previews(Users::ID id)
{
	auto connection_data = open_database();

	result_set_ref result;

	connection_data.stmnt_users_events->set_unsigned32(0, id);
	result = connection_data.stmnt_users_events->query();
	

	return event_previews_from_result_set(result, true);
}

vector<EventPreview> get_users_upcoming_events_previews(Users::ID id)
{
	auto connection_data = open_database();

	result_set_ref result;

	connection_data.stmnt_users_upcoming_events->set_unsigned32(0,id);
	result = connection_data.stmnt_users_upcoming_events->query();
	

	return event_previews_from_result_set(result, false);
}

vector<EventPreview> get_users_invited_events_previews(Users::ID id)
{
	auto connection_data = open_database();

	result_set_ref result;

	connection_data.stmnt_users_invited_events->set_unsigned32(0,id);
	result = connection_data.stmnt_users_invited_events->query();
	

	return event_previews_from_result_set(result, false);
}

// result should be nullptr or a result set of 1 containing organiser_id, is_published, and is_public
static ErrorType check_event_view_permission(uint32_t event_id, optional<Users::ID> logged_in_id,
	bool & is_attending, bool & invited, result_set_ref result) {


	if(!result) {
		auto connection_data = open_database();
		connection_data.stmnt_get_event_permissions_data->set_unsigned32(0, event_id);
		result = connection_data.stmnt_get_event_permissions_data->query();

		if(!result->next()) {
			return ErrorType::NotFound;
		}
	}

	auto organiser_id = result->get_unsigned32("organiser_id");
	is_attending = false;
	invited = false;

	if(!result->get_unsigned8("is_published")){
		if(!logged_in_id.has_value()) {
			return ErrorType::NotLoggedIn;
		}
		if(organiser_id != logged_in_id.value()) {
			return ErrorType::PermissionsError;
		}
	}

	else {
		if(!result->get_unsigned8("is_public")) {
			// Private event. Check user is invited or attending

			if(!logged_in_id.has_value()) {
				return ErrorType::NotLoggedIn;
			}

			if(organiser_id != logged_in_id.value()) {
				is_attending = is_user_registered_for_event(event_id, logged_in_id.value());

				if(!is_attending) {
					invited = is_user_invited_to_event(event_id, logged_in_id.value());
					if(!invited) {
						return ErrorType::PermissionsError;
					}
				}
			}

			
		}
		else if(logged_in_id.has_value() && organiser_id != logged_in_id.value()) {
			// Public event, we are not the owner
			is_attending = is_user_registered_for_event(event_id, logged_in_id.value());
		}
	}

	return ErrorType::Success;
}

ErrorMaybe<Event> get_event_(EventID id, optional<Users::ID> logged_in_id, bool for_edit) {
	if(for_edit && !logged_in_id.has_value()) return ErrorType::NotLoggedIn;

	auto connection_data = open_database();

	result_set_ref result;

	Event event;

	connection_data.stmnt_get_event->set_unsigned32(0, id);
	result = connection_data.stmnt_get_event->query();

	if(!result->next()) {
		return ErrorType::NotFound;
	}

	event.organiser_id = result->get_unsigned32("organiser_id");
	event.is_published = result->get_unsigned8("is_published") != 0;
	event.is_public = result->get_unsigned8("is_public") != 0;

	bool attending = false, invited = false;

	if(for_edit) {
		if(event.organiser_id != logged_in_id.value()) {
			return ErrorType::PermissionsError;
		}
	}
	else {
		auto permission_check = check_event_view_permission(id, logged_in_id, attending, invited, result);
		if(permission_check != ErrorType::Success) {
			return permission_check;
		}
	}

	event.is_attending = attending;

	event.name = result->get_string("name");
	event.organiser_name = result->get_string("organiser_name");

	if(!result->get_is_null("description"))
		event.description = result->get_string("description");
	if(!result->get_is_null("description2"))
		event.description2 = result->get_string("description2");

	if(!result->get_is_null("url"))
		event.url = result->get_string("url");

	if(!result->get_is_null("country")) {
		auto c = result->get_unsigned8("country");
		event.country_code = c;
		event.country = get_country_string(c);
	}
	else {
		event.country = "Online Event";
	}

	event.duration = result->get_unsigned16("event_duration");
	event.time_zone = result->get_unsigned8("event_time_zone");

	if(!result->get_is_null("youtube_video_code"))
		event.youtube_video_id = result->get_string("youtube_video_code");

	if(!result->get_is_null("address"))
		event.address = result->get_string("address");

	if(!result->get_is_null("gps_lat") && !result->get_is_null("gps_long")) {
		event.gps = Event::GPSCoords();
		event.gps.value().lat = result->get_float("gps_lat");
		event.gps.value().lon = result->get_float("gps_long");
	}

	auto dt = result->get_date_time("event_date_time");
	event.date = dt.str_date();
	event.time = dt.str().substr(11);
	event.time.resize(5);

	event.attendees_list_private = result->get_unsigned8("hide_attendees") != 0;
	event.feedback_window_duration = result->get_unsigned8("feedback_window_duration");
	event.post_code = result->get_string("postcode");

	if(!result->get_is_null("cover_image_id")) {
		MediaID em;
		data_ref dr = result->get_data("cover_image_id");
		memcpy(em.data, dr->get(), 17);
		event.cover_image = em;
	}

	if(logged_in_id.has_value()) {
		// Only fetch invite list when editing an event
		// organiser_id auth already done in first query
		connection_data.stmnt_get_event_invitations->set_unsigned32(0, id);
		result = connection_data.stmnt_get_event_invitations->query();

		while(result->next()) {
			event.invitations.push_back(result->get_string("email"));
		}
	}


	// Comments

	connection_data.stmnt_get_recent_comments->set_unsigned32(0, id);
	result = connection_data.stmnt_get_recent_comments->query();

	for(unsigned int i = 0; i < 4 && result->next(); i++) {
		optional<uint32_t> seconds_since_last_edit;
		if(!result->get_is_null("time_since_edit")) {
			seconds_since_last_edit = static_cast<uint32_t>(result->get_unsigned64("time_since_edit"));
		}

		optional<MediaID> ppic;

		if(!result->get_is_null("profile_picture_id")) {
			MediaID m;
			data_ref dr = result->get_data("profile_picture_id");
			memcpy(m.data, dr->get(), 17);
			ppic = m;
		}

		event.recent_comments.push_back(Event::Comment {
			result->get_unsigned32("id"),
			result->get_unsigned32("user_id"),
			result->get_string("name"),			
			ppic,
			result->get_string("comment_text"),
			static_cast<uint32_t>(result->get_unsigned64("ago")),
			seconds_since_last_edit,
			result->get_unsigned8("number_of_replies")
		});
	}


	// Media

	connection_data.stmnt_get_event_media->set_unsigned32(0, id);
	result = connection_data.stmnt_get_event_media->query();

	while(result->next()) {
		MediaID emid;
		memcpy(emid.data, result->get_data("id")->get(), 17);
		event.all_media.push_back(Media {
			emid,			
			result->get_unsigned8("is_cover_image") != 0,
			result->get_string("file_name")
		});
	}



	return event;
}

ErrorMaybe<vector<Event::Comment>> get_comments(EventID event_id, uint32_t start_id, optional<Users::ID> logged_in_id)
{
	bool attending, invited;
	auto permission_check = check_event_view_permission(event_id, logged_in_id, attending, invited, nullptr);
	if(permission_check != ErrorType::Success) {
		return permission_check;
	}

	auto connection_data = open_database();
	// TODO check permissions

	connection_data.stmnt_get_comments->set_unsigned32(0, event_id);
	connection_data.stmnt_get_comments->set_unsigned32(1, start_id);
	auto result = connection_data.stmnt_get_comments->query();

	vector<Event::Comment> comments;

	for(unsigned int i = 0; i < 4 && result->next(); i++) {
		optional<uint32_t> seconds_since_last_edit;
		if(!result->get_is_null("time_since_edit")) {
			seconds_since_last_edit = static_cast<uint32_t>(result->get_unsigned64("time_since_edit"));
		}

		optional<MediaID> ppic;

		if(!result->get_is_null("profile_picture_id")) {
			MediaID m;
			data_ref dr = result->get_data("profile_picture_id");
			memcpy(m.data, dr->get(), 17);
			ppic = m;
		}

		comments.push_back(Event::Comment {
			result->get_unsigned32("id"),
			result->get_unsigned32("user_id"),
			result->get_string("name"),			
			ppic,
			result->get_string("comment_text"),
			static_cast<uint32_t>(result->get_unsigned64("ago")),
			seconds_since_last_edit,
			result->get_unsigned8("number_of_replies")
		});
	}
	return comments;
}

ErrorMaybe<vector<Reply>> get_replies(uint32_t comment_id, optional<Users::ID> logged_in_id)
{
	auto connection_data = open_database();
	connection_data.stmnt_get_replies->set_unsigned32(0, comment_id);
	auto result = connection_data.stmnt_get_replies->query();

	vector<Reply> replies;
	if(!result->next()) {
		return replies;
	}


	bool attending, invited;
	auto permission_check = check_event_view_permission(result->get_unsigned32("event_id"), logged_in_id, attending, invited, nullptr);
	if(permission_check != ErrorType::Success) {
		return permission_check;
	}



	do {
		optional<uint32_t> seconds_since_last_edit;
		if(!result->get_is_null("time_since_edit")) {
			seconds_since_last_edit = static_cast<uint32_t>(result->get_unsigned64("time_since_edit"));
		}

		optional<MediaID> ppic;

		if(!result->get_is_null("profile_picture_id")) {
			MediaID m;
			data_ref dr = result->get_data("profile_picture_id");
			memcpy(m.data, dr->get(), 17);
			ppic = m;
		}

		replies.push_back(Reply {
			result->get_unsigned32("id"),
			result->get_unsigned32("user_id"),
			result->get_string("name"),	
			ppic,		
			result->get_string("comment_text"),
			static_cast<uint32_t>(result->get_unsigned64("ago")),
			seconds_since_last_edit
		});
	} while(result->next());
	return replies;
}

ErrorMaybe<uint32_t> post_comment(uint32_t event_id, Users::ID logged_in_id, string const& text)
{

	bool attending, invited;
	auto permission_check = check_event_view_permission(event_id, logged_in_id, attending, invited, nullptr);
	if(permission_check != ErrorType::Success) {
		return permission_check;
	}

	auto connection_data = open_database();
	connection_data.stmnt_post_comment->set_unsigned32(0, event_id);
	connection_data.stmnt_post_comment->set_string(1, text);
	connection_data.stmnt_post_comment->set_unsigned32(2, logged_in_id);
	auto id = connection_data.stmnt_post_comment->insert();

	if(id >= 1) {
		return id;
	}
	return ErrorType::GenericError;

}

ErrorMaybe<uint32_t> post_reply(uint32_t comment_id, Users::ID logged_in_id, string const& text)
{
	auto connection_data = open_database();
	connection_data.stmnt_get_comment_data_for_replying->set_unsigned32(0, comment_id);
	auto result = connection_data.stmnt_get_comment_data_for_replying->query();

	if(!result->next()) {
		return ErrorType::NotFound;
	}

	auto event_id = result->get_unsigned32("event_id");
	auto number_of_replies = result->get_unsigned8("number_of_replies");

	if(number_of_replies >= 100) {
		return ErrorType::BadRequest;
	}

	bool attending, invited;
	auto permission_check = check_event_view_permission(event_id, logged_in_id, attending, invited, result);
	if(permission_check != ErrorType::Success) {
		return permission_check;
	}

	connection_data.stmnt_post_reply->set_unsigned32(0, event_id);
	connection_data.stmnt_post_reply->set_unsigned32(1, comment_id);
	connection_data.stmnt_post_reply->set_string(2, text);
	connection_data.stmnt_post_reply->set_unsigned32(3, logged_in_id);
	auto id = connection_data.stmnt_post_reply->insert();

	if(id >= 1) {
		connection_data.stmnt_update_number_of_replies->set_unsigned32(0, comment_id);
		connection_data.stmnt_update_number_of_replies->set_unsigned32(1, comment_id);
		connection_data.stmnt_update_number_of_replies->execute();

		return id;
	}
	return ErrorType::GenericError;

}

ErrorType delete_comment(uint32_t comment_id, Users::ID logged_in_id)
{
	auto connection_data = open_database();
	connection_data.stmnt_delete_comment->set_unsigned32(0, comment_id);
	connection_data.stmnt_delete_comment->set_unsigned32(1, logged_in_id);
	connection_data.stmnt_delete_comment->set_unsigned32(2, logged_in_id);
	auto affected = connection_data.stmnt_delete_comment->execute();

	if(affected > 0) {
		connection_data.stmnt_delete_comment_replies->set_unsigned32(0, comment_id);
		connection_data.stmnt_delete_comment_replies->execute();

		return ErrorType::Success;
	}
	return ErrorType::BadRequest;
}

ErrorType delete_reply(uint32_t reply_id, Users::ID logged_in_id)
{
	auto connection_data = open_database();
	connection_data.stmnt_get_reply_user_and_comment_id->set_unsigned32(0, reply_id);
	auto result = connection_data.stmnt_get_reply_user_and_comment_id->query();

	if(!result->next()) return ErrorType::NotFound;
	if(result->get_unsigned32("user_id") != logged_in_id) {
		connection_data.stmnt_get_user_privilege->set_unsigned32(0, logged_in_id);
		auto result2 = connection_data.stmnt_get_user_privilege->query();
		if(!result2->next()) return ErrorType::PermissionsError;
		if(result2->get_unsigned8("privilege_level") == 0) {
			return ErrorType::PermissionsError;
		}
	}

	auto comment_id = result->get_unsigned32("parent_comment_id");


	connection_data.stmnt_delete_reply->set_unsigned32(0, reply_id);
	connection_data.stmnt_delete_reply->execute();

	connection_data.stmnt_update_number_of_replies->set_unsigned32(0, comment_id);
	connection_data.stmnt_update_number_of_replies->set_unsigned32(1, comment_id);
	connection_data.stmnt_update_number_of_replies->execute();

	return ErrorType::Success;
}

static vector<Users::ID> get_event_attendees_ids(EventID event_id)
{
	auto connection_data = open_database();

	connection_data.stmnt_get_event_attendee_ids->set_unsigned32(0, event_id);
	auto result = connection_data.stmnt_get_event_attendee_ids->query();

	vector<Users::ID> attendees;
	while(result->next()) {
		attendees.push_back(result->get_unsigned32("user_id"));
	}
	return attendees;
}

ErrorMaybe<vector<Users::Name>> get_event_attendees(EventID event_id, optional<Users::ID> logged_in_id)
{
	auto connection_data = open_database();

	connection_data.stmnt_get_event_hide_attendees->set_unsigned32(0, event_id);
	auto result = connection_data.stmnt_get_event_hide_attendees->query();
	if(!result->next()) return ErrorType::NotFound;

	if(!result->get_unsigned8("is_public")) {
		if(!logged_in_id.has_value()) return ErrorType::PermissionsError;

		if(result->get_unsigned32("organiser_id") != logged_in_id.value()) {

			if(!check_user_invited(event_id, logged_in_id.value())
				&& !is_user_registered_for_event(event_id, logged_in_id.value())) {
				return ErrorType::PermissionsError;
			}

		}
	}

	if(logged_in_id.has_value()) {
		if(result->get_unsigned8("hide_attendees") &&
			result->get_unsigned32("organiser_id") != logged_in_id.value()) return ErrorType::PermissionsError;
	}
	else {
		if(result->get_unsigned8("hide_attendees")) return ErrorType::PermissionsError;
	}


	connection_data.stmnt_get_event_attendee_names->set_unsigned32(0, event_id);
	result = connection_data.stmnt_get_event_attendee_names->query();

	vector<Users::Name> attendees;
	while(result->next()) {
		attendees.push_back(result->get_string("name"));
	}
	return attendees;
}

ErrorMaybe<Event> get_event(EventID id, optional<Users::ID> logged_in_id) {
	return get_event_(id, logged_in_id, false);
}


string const& get_country_string(unsigned int country_id)
{
	const static string unknown_country = "??";
	if(country_id >= all_countries_list.size()) return unknown_country;
	return all_countries_list[country_id];
}


ErrorMaybe<Event> get_edit_event_info(EventID event_id, Users::ID logged_in_id)
{
	return get_event_(event_id, logged_in_id, true);
}

static const unordered_map<EventAttribute, const char *> event_attrib_column_name = {
	{EventAttribute::Name, "name"},
	{EventAttribute::URL, "url"},
	{EventAttribute::IsPublic, "is_public"},
	{EventAttribute::IsPublished, "is_published"},
	{EventAttribute::AttendeesListPrivate, "hide_attendees"},
	{EventAttribute::StartDateTime, "event_date_time"},
	{EventAttribute::TimeZone, "event_time_zone"},
	{EventAttribute::Duration, "event_duration"},
	{EventAttribute::Description, "description"},
	{EventAttribute::Description2, "description2"},
	{EventAttribute::CountryCode, "country"},
	{EventAttribute::Address, "address"},
	{EventAttribute::PostCode, "postcode"},
	{EventAttribute::GPSLattitude, "gps_lat"},
	{EventAttribute::GPSLongitude, "gps_long"},
	{EventAttribute::FeedBackWindowDuration, "feedback_window_duration"},
	{EventAttribute::YTVideoCode, "youtube_video_code"}
	
};

void sql_escape_string(string & dst, string const& src) {
	dst.reserve(dst.size() + src.size());

	for(char c : src) {
		if(!c) {
			dst.append("\\0");
		}
		else if(c == '\n') {
			dst.append("\\n");
		}
		else if(c == '\r') {
			dst.append("\\r");
		}
		else if(c == '\\') {
			dst.append("\\\\");
		}
		else if(c == '\'') {
			dst.append("\\'");
		}
		else if(c == '"') {
			dst.append("\\\"");
		}
		else if(c == '\x1a') {
			dst.append("\\x1a");
		}
		else if(c == '\b') {
			dst.append("\\b");
		}
		else if(c == '\t') {
			dst.append("\\t");
		}
		else {
			dst.append(1, c);
		}
	}
}

static bool event_attribs_valid(unordered_map<EventAttribute, AttributeValue> attributes) {
	// TODO
	return true;
}

static void sql_add_value(AttributeValue const& val, string & sql)
{
	if(!val.has_value()) {
		sql.append("null,");
		return;
	}

	if(auto x = get_if<string>(&val.value())) {
		sql.append(1, '"');
		sql_escape_string(sql, *x);
		sql.append("\",");
	}
	#define AAA(type) \
	else if(auto x_ ## type = get_if<type>(&val.value())) { \
		sql.append(to_string(*(x_ ## type))); \
		sql.append(1, ','); \
	}
	AAA(uint8_t)
	AAA(uint16_t)
	AAA(uint32_t)
	AAA(uint64_t)
	AAA(int8_t)
	AAA(int16_t)
	AAA(int32_t)
	AAA(int64_t)
	AAA(float)
	AAA(double)
	#undef AAA
	else if(auto x_dt = get_if<DateTime>(&val.value())) {
		date_time dt(x_dt->year, x_dt->month, x_dt->day, x_dt->hour, x_dt->minute);
		sql.append(1, '"');
		sql.append(dt.str());
		sql.append("\",");
	}
	else if(auto x_bool = get_if<bool>(&val.value())) {
		sql.append(*x_bool ? "1," : "0,");
	}
	else {
		assert_(false);
	}
}


static void send_new_event_notifications(EventID event_id, Users::ID organiser_id, string const& event_name)
{
	auto organiser_name = get_users_name(organiser_id);
	assert(organiser_name.has_value());
	auto users = get_followers(organiser_id);

	for(auto user_id : users) {
		Notification n;
		n.type = Notification::Type::NewEvent;
		n.event_id = event_id;
		n.organiser_name = organiser_name;
		n.event_name = event_name;
		create_notification(user_id, n);
	}
}

ErrorType edit_event(EventID id, Users::ID logged_in_id, unordered_map<EventAttribute, AttributeValue> const& updated_fields,
	vector<string> const& new_invites, vector<string> const& removed_invites)
{

	if(!event_attribs_valid(updated_fields)) {
		return ErrorType::InvalidParameter;
	}

	auto connection_data = open_database();

	if(!updated_fields.size() && !new_invites.size() && !removed_invites.size()) {
		return ErrorType::Success;
	}

	// Check we are the organiser
	connection_data.stmnt_get_event_organiser->set_unsigned32(0, id);
	auto result = connection_data.stmnt_get_event_organiser->query();
	if(!result->next()) return ErrorType::NotFound;
	if(result->get_unsigned32("organiser_id") != logged_in_id) {
		return ErrorType::PermissionsError;
	}


	bool publishing_the_event = false;

	if(updated_fields.find(EventAttribute::IsPublished) != updated_fields.end()) {
		auto const& atrib_is_published = updated_fields.at(EventAttribute::IsPublished);
		if(atrib_is_published.has_value() && get<bool>(atrib_is_published.value())) {
			publishing_the_event = true;
		}
	}

	if(updated_fields.size()) {
		string sql ("UPDATE Events SET ");

		for(auto const& [attr, val] : updated_fields) {
			sql.append(event_attrib_column_name.at(attr));
			sql.append("=");

			if(val.has_value()) {
				sql_add_value(val, sql);
			}
			else {
				sql.append("null,");
			}
		}
		sql.resize(sql.size()-1);
		sql.append(" WHERE id=");
		sql.append(to_string(id));

		if(publishing_the_event) {
			// Don't allow users to republish or unpublish
			sql.append(" AND is_published=0");
		}

		cout << "SQL: [" << sql << "]\n";

		auto affected = connection_data.con->execute(move(sql));
		if(affected == 0) return ErrorType::GenericError;
	}



	optional<string> event_name;

	if(updated_fields.find(EventAttribute::Name) != updated_fields.end()) {
		auto const& atrib_name = updated_fields.at(EventAttribute::Name);
		if(atrib_name.has_value()) {
			event_name = get<string>(atrib_name.value());
		}
	}

	if(!event_name.has_value()) {
		connection_data.stmnt_get_event_name->set_unsigned32(0, id);
		result = connection_data.stmnt_get_event_name->query();
		assert_(result->next());
		event_name = result->get_string("name");
	}

	bool is_public = false;

	if(updated_fields.find(EventAttribute::IsPublic) != updated_fields.end()) {
		is_public = get<bool>(updated_fields.at(EventAttribute::IsPublic).value());
	}
	else {
		connection_data.stmnt_is_event_public->set_unsigned32(0, id);
		result = connection_data.stmnt_is_event_public->query();
		assert_(result->next());
		is_public = result->get_unsigned8("is_public") != 0;
	}


	// Invites

	if(!publishing_the_event) {
		// Update invitations in database

		for(string const& email : new_invites) {
			connection_data.stmnt_add_invitation->set_unsigned32(0, id);
			connection_data.stmnt_add_invitation->set_string(1, email);
			connection_data.stmnt_add_invitation->execute();
		}

		for(string const& email : removed_invites) {
			connection_data.stmnt_remove_invitation->set_unsigned32(0, id);
			connection_data.stmnt_remove_invitation->set_string(1, email);
			connection_data.stmnt_remove_invitation->execute();
		}
	}
	else {
		// Get invitations from database, apply new/removed lists, send out emails, send notifications

		unordered_set<string> invitations;
		connection_data.stmnt_get_event_invitations->set_unsigned32(0, id);
		result = connection_data.stmnt_get_event_invitations->query();

		while(result->next()) {
			invitations.insert(result->get_string("email"));
		}

		for(auto const& email : new_invites) {
			invitations.insert(email);
		}

		for(auto const& email : removed_invites) {
			invitations.erase(email);
		}


		Emails::send_invitation_emails(string(event_name.value()), id, invitations);

		if(is_public) {
			send_new_event_notifications(id, logged_in_id, event_name.value());
		}

		// connection_data.stmnt_clear_event_invitations->set_unsigned32(0, id);
		// connection_data.stmnt_clear_event_invitations->execute();
	}

	// Notifications

	auto organiser_name = get_users_name(logged_in_id);
	assert(organiser_name.has_value());

	auto ids = get_event_attendees_ids(id);
	for(auto usr_id : ids) {
		Notification n;
		n.type = Notification::Type::EventChanged;
		n.event_id = id;
		n.organiser_name = organiser_name;
		n.event_name = event_name;
		create_notification(usr_id, n);
	}



	return ErrorType::Success;
}

// Returns the event ID
optional<EventID> create_event(Users::ID organiser_id,
	// TODO: attributes can be const&?
		unordered_map<EventAttribute, AttributeValue> attributes, vector<string> const& invites)
{
	// Check all required data is in the map
	if(
		attributes.find(EventAttribute::Name) == attributes.end() ||
		attributes.find(EventAttribute::StartDateTime) == attributes.end() ||
		attributes.find(EventAttribute::TimeZone) == attributes.end() ||
		!event_attribs_valid(attributes)
	)
	{
		return nullopt;
	}

	auto connection_data = open_database();

	bool is_published = false;

	if(attributes.find(EventAttribute::IsPublished) != attributes.end()) {
		auto const& atrib_is_published = attributes[EventAttribute::IsPublished];
		if(atrib_is_published.has_value() && get<bool>(atrib_is_published.value())) is_published = true;
	}


	// TODO is timestamp_last_event_creation over an hour ago? if so update and rest counter
	// TODO else check counter- fail if >= 8

	// TODO check number of published events by this user


	string sql ("INSERT INTO Events (organiser_id,");

	for(const auto& [attrib, _] : attributes) {
		sql.append(event_attrib_column_name.at(attrib));
		sql.append(1, ',');
	}
	sql.resize(sql.size()-1);
	sql.append(") VALUES(");
	sql.append(to_string(organiser_id));
	sql.append(1, ',');

	for(const auto& [_, value] : attributes) {
		sql_add_value(value, sql);
	}
	sql.resize(sql.size()-1);
	sql.append(1, ')');

	cout << "SQL: [" << sql << "]\n";

	EventID id = static_cast<EventID>(connection_data.con->insert(move(sql)));
	if(id == 0) return nullopt;


	// Invites

	// Add invitations to database
	for(string const& email : invites) {
		try {
			connection_data.stmnt_add_invitation->set_unsigned32(0, id);
			connection_data.stmnt_add_invitation->set_string(1, email);
			connection_data.stmnt_add_invitation->execute();
		} catch(exception const& e) {
			spdlog::debug("Error saving invitations: {}", e.what());
		}
	}

	if(is_published) {

		// Send out emails

		Emails::send_invitation_emails(string(get<string>(attributes[EventAttribute::Name].value())), id, invites);


		if(attributes[EventAttribute::IsPublic]) {
			send_new_event_notifications(id, organiser_id, get<string>(attributes[EventAttribute::Name].value()));
		}
	}
	


	return id;
}

static bool check_user_invited(EventID event_id, Users::ID user_id)
{
	auto connection_data = open_database();

	connection_data.stmnt_is_user_invited->set_unsigned32(0, event_id);
	connection_data.stmnt_is_user_invited->set_unsigned32(1, user_id);
	auto result = connection_data.stmnt_is_user_invited->query();
	if(!result->next()) return false;
	if(result->get_signed64("n") == 0) return false;
	return true;
}

ErrorType register_attendance(EventID event_id, Users::ID user_id)
{
	auto connection_data = open_database();

	connection_data.stmnt_event_is_public->set_unsigned32(0, event_id);
	auto result = connection_data.stmnt_event_is_public->query();
	if(!result->next()) return ErrorType::NotFound;

	if(!result->get_unsigned8("is_public")) {
		// Are we invited?

		if(!check_user_invited(event_id, user_id)) return ErrorType::PermissionsError;
	}

	connection_data.stmt_register_attendance->set_unsigned32(0, event_id);
	connection_data.stmt_register_attendance->set_unsigned32(1, user_id);
	connection_data.stmt_register_attendance->execute();

	return ErrorType::Success;
}

void unregister_attendance(EventID event_id, Users::ID user_id)
{
	auto connection_data = open_database();
	connection_data.stmt_unregister_attendance->set_unsigned32(0, event_id);
	connection_data.stmt_unregister_attendance->set_unsigned32(1, user_id);
	connection_data.stmt_unregister_attendance->execute();
}

bool is_user_registered_for_event(EventID event_id, Users::ID user_id)
{
	auto connection_data = open_database();
	connection_data.stmnt_is_attendance_registered->set_unsigned32(0, event_id);
	connection_data.stmnt_is_attendance_registered->set_unsigned32(1, user_id);
	auto result = connection_data.stmnt_is_attendance_registered->query();

	if(!result->next()) return false;
	return result->get_signed64("n") != 0;
}


bool is_user_invited_to_event(EventID event_id, Users::ID user_id)
{
	auto connection_data = open_database();
	connection_data.stmnt_is_user_invited->set_unsigned32(0, event_id);
	connection_data.stmnt_is_user_invited->set_unsigned32(1, user_id);
	auto result = connection_data.stmnt_is_user_invited->query();

	if(!result->next()) return false;
	return result->get_signed64("n") != 0;
}

ErrorType delete_event(EventID event_id, Users::ID logged_in_user_id)
{
	auto connection_data = open_database();


	connection_data.stmnt_get_event_organiser_and_name->set_unsigned32(0, event_id);
	auto result = connection_data.stmnt_get_event_organiser_and_name->query();
	if(!result->next()) return ErrorType::NotFound;

	if(logged_in_user_id != result->get_unsigned32("organiser_id")) {
		connection_data.stmnt_get_user_privilege->set_unsigned32(0, logged_in_user_id);
		auto result2 = connection_data.stmnt_get_user_privilege->query();
		if(!result2->next()) return ErrorType::PermissionsError;
		if(result2->get_unsigned8("privilege_level") == 0) {
			return ErrorType::PermissionsError;
		}
	}

	string event_name = result->get_string("name");


	// Media

	connection_data.stmnt_get_event_media_ids->set_unsigned32(0, event_id);
	result = connection_data.stmnt_get_event_media_ids->query();


	// if(had_media) {
	// 	connection_data.stmnt_delete_event_media->set_unsigned32(0, event_id);
	// 	connection_data.stmnt_delete_event_media->execute();
	// }

	// Delete event

	connection_data.stmnt_delete_event->set_unsigned32(0, event_id);
	auto affected = connection_data.stmnt_delete_event->execute();

	if(affected == 0) {
		return ErrorType::GenericError;
	}
	
	// bool had_media = false;
	while(result->next()) {
		MediaID id;
		memcpy(id.data, result->get_data("id")->get(), 17);
		cout << "delete " << id.to_string() << "\n";
		// delete_s3_object(id.to_string());

		// had_media = true;
	}

	// Invitations

	// connection_data.stmnt_clear_event_invitations->set_unsigned32(0, event_id);
	// connection_data.stmnt_clear_event_invitations->execute();


	// ... polls, feedback


	// Notifications

	auto ids = get_event_attendees_ids(event_id);
	for(auto usr_id : ids) {
		Notification n;
		n.type = Notification::Type::EventDeleted;
		n.event_name = event_name;
		create_notification(usr_id, n);
	}


	// connection_data.stmnt_clear_event_attendance->set_unsigned32(0, event_id);
	// connection_data.stmnt_clear_event_attendance->execute();


	// connection_data.stmnt_clear_events_replies->set_unsigned32(0, event_id);
	// connection_data.stmnt_clear_events_replies->execute();
	// connection_data.stmnt_clear_events_comments->set_unsigned32(0, event_id);
	// connection_data.stmnt_clear_events_comments->execute();

	return ErrorType::Success;
}

bool try_add_media(optional<EventID> event_id, string file_name, bool is_cover_image) {
	auto connection_data = open_database();

	MediaID& m_id = *connection_data.media_id;

	if(event_id.has_value()) {
		connection_data.stmnt_add_media->set_unsigned32(1, event_id.value());
	}
	else {
		connection_data.stmnt_add_media->set_null(1);
	}

	connection_data.stmnt_add_media->set_string(2, file_name);
	connection_data.stmnt_add_media->set_unsigned8(3, is_cover_image ? 1 : 0);

	for(int attempts = 0; attempts < 3; attempts++) {
		m_id.data[0] = 'a';
		try_create_random(&m_id.data[1], 16);

		try {
			connection_data.stmnt_add_media->set_data(0, connection_data.d_ref_media_id);
			auto affected = connection_data.stmnt_add_media->execute();


			if(affected >= 1) {
				return true;
			}
		}
		catch (exception const& e){
			spdlog::error(e.what());
		}
	}

	return false;
}

ErrorMaybe<pair<MediaID, S3PresignedURL>> add_cover_image(EventID event_id, Users::ID logged_in_user_id, string file_name)
{
	auto connection_data = open_database();

	// Check we are the organiser and there isn't already a cover image
	connection_data.stmnt_get_event_organiser_and_cover_image->set_unsigned32(0, event_id);
	auto result = connection_data.stmnt_get_event_organiser_and_cover_image->query();
	if(!result->next()) return ErrorType::NotFound;

	if(result->get_unsigned32("organiser_id") != logged_in_user_id) {
		return ErrorType::PermissionsError;
	}
	if(!result->get_is_null("cover_image_id")) {
		return ErrorType::BadRequest;
	}

	MediaID& m_id = *connection_data.media_id;
	bool got_id = try_add_media(event_id, file_name, true);

	if(!got_id) {
		return ErrorType::GenericError;
	}

	connection_data.stmnt_set_cover_image->set_data(0, connection_data.d_ref_media_id);
	connection_data.stmnt_set_cover_image->set_unsigned32(1, event_id);
	connection_data.stmnt_set_cover_image->execute();


	return pair<MediaID, S3PresignedURL>(m_id, put_s3_object(m_id.to_string(), file_name));
}

ErrorType delete_cover_image(EventID event_id, Users::ID logged_in_user_id)
{
	auto connection_data = open_database();

	// Check we are the organiser and get the cover image id
	connection_data.stmnt_get_event_organiser_and_cover_image->set_unsigned32(0, event_id);
	auto result = connection_data.stmnt_get_event_organiser_and_cover_image->query();
	if(!result->next()) return ErrorType::NotFound;

	if(result->get_unsigned32("organiser_id") != logged_in_user_id) {
		return ErrorType::PermissionsError;
	}

	if(result->get_is_null("cover_image_id")) {
		return ErrorType::Success;
	}

	auto cover_image_id_ = result->get_data("cover_image_id");
	auto& cover_image_id = *reinterpret_cast<MediaID*>(cover_image_id_->get());


	connection_data.stmnt_delete_cover_image->set_unsigned32(0, event_id);
	connection_data.stmnt_delete_cover_image->execute();

	connection_data.stmnt_delete_media->set_data(0, cover_image_id_);
	connection_data.stmnt_delete_media->execute();

	delete_s3_object(cover_image_id.to_string());

	return ErrorType::Success;
}

ErrorType delete_media(string media_id_string, Users::ID logged_in_user_id)
{
	auto media_id = MediaID(media_id_string);

	auto connection_data = open_database();

	*connection_data.media_id = media_id;
	connection_data.stmnt_get_media_event_id->set_data(0, connection_data.d_ref_media_id);
	// TODO check if this is a cover image. if so, set cover_image_id of event to null

	auto result = connection_data.stmnt_get_media_event_id->query();
	if(!result->next()) return ErrorType::NotFound;

	auto event_id = result->get_unsigned32("event_id");


	connection_data.stmnt_get_event_organiser->set_unsigned32(0, event_id);
	result = connection_data.stmnt_get_event_organiser->query();
	if(!result->next()) return ErrorType::NotFound;

	if(result->get_unsigned32("organiser_id") != logged_in_user_id) {
		return ErrorType::PermissionsError;
	}



	connection_data.stmnt_delete_media->set_data(0, connection_data.d_ref_media_id);
	connection_data.stmnt_delete_media->execute();

	delete_s3_object(media_id_string);

	return ErrorType::Success;

}


ErrorMaybe<pair<MediaID, S3PresignedURL>> add_media(EventID event_id, Users::ID logged_in_user_id, string file_name)
{
	auto connection_data = open_database();

	// Check we are the organiser
	connection_data.stmnt_get_event_organiser->set_unsigned32(0, event_id);
	auto result = connection_data.stmnt_get_event_organiser->query();
	if(!result->next()) return ErrorType::NotFound;

	if(result->get_unsigned32("organiser_id") != logged_in_user_id) {
		return ErrorType::PermissionsError;
	}

	MediaID& m_id = *connection_data.media_id;
	bool got_id = try_add_media(event_id, file_name, false);

	if(!got_id) {
		return ErrorType::GenericError;
	}


	return pair<MediaID, S3PresignedURL>(m_id, put_s3_object(m_id.to_string(), file_name));
}

}