#pragma once

#include <string>
#include <optional>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <variant>
#include <stdexcept>
#include <array>
#include "../Users.h"
#include "../IP.h"
#include "../ErrorMaybe.h"
#include "../HexString.h"
#include "Media.h"

namespace DB {

using EventID = uint32_t;



struct EventPreview {
	EventID id;
	std::string title;
	Users::ID organiser_id;
	// std::string organiser_name
	std::string location; // url or postcode
	std::string shortened_description;
	std::string datetime;
	int time_zone; // Relative to GMT+0
	unsigned int duration; // in minutes
	bool is_published;

	std::optional<MediaID> cover_image;
};
std::vector<EventPreview> get_recent_events(std::string const& name, std::string const& location);

std::vector<EventPreview> get_users_events_previews(Users::ID);
std::vector<EventPreview> get_users_upcoming_events_previews(Users::ID);
std::vector<EventPreview> get_users_invited_events_previews(Users::ID);




struct Event {
	Users::ID organiser_id;
	std::string organiser_name;
	
	std::string name;
	std::string url;
	bool is_public;
	bool is_published;
	bool attendees_list_private;

	std::string date;
	std::string time;
	int time_zone;

	unsigned int duration;
	std::string description;
	std::string description2;
	std::string country;
	std::optional<uint8_t> country_code;
	std::string address;
	std::string post_code;

	struct GPSCoords {
		float lat, lon;
	};
	std::optional<GPSCoords> gps;

	unsigned int feedback_window_duration;

	std::string youtube_video_id;

	// List of emails
	std::vector<std::string> invitations;

	// For get_event only, private events only
	bool is_attending = false;


	struct Comment {
		uint32_t id;
		Users::ID user_id;
		std::string commenter_name;
		std::optional<MediaID> commenter_profile_picture;
		std::string text;
		uint32_t seconds_ago;
		std::optional<uint32_t> seconds_since_last_edit;
		int replies;
	};

	std::vector<Comment> recent_comments;

	std::optional<MediaID> cover_image;

	// Includes the cover image ^
	std::vector<Media> all_media;
	
};
ErrorMaybe<Event> get_event(EventID, std::optional<Users::ID> logged_in_id);

// start_id is highest ID, comments sorted by ID descending
ErrorMaybe<std::vector<Event::Comment>> get_comments(EventID, uint32_t start_id, std::optional<Users::ID> logged_in_id);

struct Reply {
	uint32_t id;
	Users::ID user_id;
	std::string commenter_name;
	std::optional<MediaID> commenter_profile_picture;
	std::string text;
	uint32_t seconds_ago;
	std::optional<uint32_t> seconds_since_last_edit;
};

ErrorMaybe<std::vector<Reply>> get_replies(uint32_t comment_id, std::optional<Users::ID> logged_in_id);

ErrorMaybe<uint32_t> post_comment(uint32_t event_id, Users::ID logged_in_id, std::string const& text);
ErrorMaybe<uint32_t> post_reply(uint32_t comment_id, Users::ID logged_in_id, std::string const& text);

ErrorType delete_comment(uint32_t comment_id, Users::ID logged_in_id);
ErrorType delete_reply(uint32_t reply_id, Users::ID logged_in_id);

std::string const& get_country_string(unsigned int country_id);

ErrorMaybe<Event> get_edit_event_info(EventID event_id, Users::ID logged_in_id);

ErrorMaybe<std::vector<Users::Name>> get_event_attendees(EventID, std::optional<Users::ID> logged_in_id);


using uint24_t = uint32_t;
using int24_t = int32_t;
using TimeStamp = uint32_t;

struct DateTime {
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;

	DateTime(uint16_t year_, uint8_t month_, uint8_t day_, uint8_t hour_, uint8_t minute_)
	: year(year_), month(month_), day(day_), hour(hour_), minute(minute_)
	{}
};

using AttributeValue = std::optional<std::variant<
	std::string, 
	bool, 
	uint8_t, uint16_t, uint32_t, uint64_t,
	int8_t, int16_t, int32_t, int64_t,
	float, double,
	// TimeStamp,
	DateTime
>>;

enum class EventAttribute {
	Name,
	URL,
	IsPublic,
	IsPublished,
	AttendeesListPrivate,
	StartDateTime,
	TimeZone,
	Duration,
	Description,
	Description2,
	CountryCode,
	Address,
	PostCode,
	GPSLattitude,
	GPSLongitude,
	FeedBackWindowDuration,
	YTVideoCode
};

// Returns true on success
// Will fail if the logged_in_user_id is not the organiser_id
ErrorType edit_event(EventID, Users::ID logged_in_id, std::unordered_map<EventAttribute, AttributeValue> const&, 
	std::vector<std::string> const& new_invites, std::vector<std::string> const& removed_invites);

// Returns the event ID
std::optional<EventID> create_event(Users::ID organiser_id, std::unordered_map<EventAttribute, AttributeValue>, std::vector<std::string> const& invites);

ErrorType register_attendance(EventID, Users::ID user_id);
void unregister_attendance(EventID, Users::ID user_id);
bool is_user_registered_for_event(EventID, Users::ID user_id);

bool is_user_invited_to_event(EventID, Users::ID user_id);

ErrorType delete_event(EventID, Users::ID logged_in_user_id);


using S3PresignedURL = std::string;
ErrorMaybe<std::pair<MediaID, S3PresignedURL>> add_cover_image(EventID, Users::ID logged_in_user_id, std::string file_name);

ErrorType delete_cover_image(EventID, Users::ID logged_in_user_id);

ErrorMaybe<std::pair<MediaID, S3PresignedURL>> add_media(EventID, Users::ID logged_in_user_id, std::string file_name);

ErrorType delete_media(std::string media_id_string, Users::ID logged_in_user_id);
}