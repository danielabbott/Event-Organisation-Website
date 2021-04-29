#include "DB.h"
#include <mariadb++/account.hpp>
#include <mariadb++/connection.hpp>
#include <optional>
#include "../Users.h"
#include "../Assert.h"
#include "../OutOfMemoryException.h"
#include "../Email.h"
#include "../Config.h"
#include <stdexcept>
#include <spdlog/spdlog.h>

#include <iostream>

namespace DB {

using namespace std;
using namespace mariadb;


struct ConnectionData {
	connection_ref con;

	statement_ref stmnt_add_user;
	statement_ref stmnt_get_user0;
	statement_ref stmnt_create_session;
	statement_ref stmnt_delete_session;
	statement_ref stmnt_get_user_id_from_session;
	statement_ref stmnt_get_user_info_from_session;
	statement_ref stmnt_get_user_basic_info_from_email;
	statement_ref stmnt_get_user_name_from_id;
	statement_ref stmnt_get_user_name_email_from_id;
	statement_ref stmnt_create_password_reset;
	statement_ref stmnt_get_password_reset;
	statement_ref stmnt_delete_password_resets;
	statement_ref stmnt_do_password_reset1;
	statement_ref stmnt_do_password_reset2;
	statement_ref stmnt_kill_sessions;
	statement_ref stmnt_recently_added_events;
	statement_ref stmnt_users_events;
	statement_ref stmnt_get_event;
	statement_ref stmnt_get_event_invitations;
	statement_ref stmnt_add_invitation;
	statement_ref stmnt_remove_invitation;
	statement_ref stmnt_get_event_organiser;
	statement_ref stmnt_get_event_organiser_and_name;
	statement_ref stmnt_get_event_permissions_data;
	statement_ref stmnt_get_event_name;
	statement_ref stmt_register_attendance;
	statement_ref stmt_unregister_attendance;
	statement_ref stmnt_is_attendance_registered;
	statement_ref stmnt_get_event_attendee_names;
	statement_ref stmnt_get_event_attendee_ids;
	statement_ref stmnt_clear_event_attendance;
	statement_ref stmnt_clear_event_invitations;
	statement_ref stmnt_delete_event;
	statement_ref stmnt_is_user_invited;
	statement_ref stmnt_get_event_hide_attendees;
	statement_ref stmnt_event_is_public;
	statement_ref stmnt_last_seen_notification;
	statement_ref stmnt_notifications_from_id;
	statement_ref stmnt_set_last_seen_notification;
	statement_ref stmnt_set_user_notification_email_sent;
	statement_ref stmnt_users_upcoming_events;
	statement_ref stmnt_users_invited_events;
	statement_ref stmnt_create_notification;
	statement_ref stmnt_get_recent_comments;
	statement_ref stmnt_get_comments;
	statement_ref stmnt_get_replies;
	statement_ref stmnt_post_comment;
	statement_ref stmnt_get_comment_data_for_replying;
	statement_ref stmnt_post_reply;
	statement_ref stmnt_update_number_of_replies;
	statement_ref stmnt_delete_comment;
	statement_ref stmnt_delete_comment_replies;
	statement_ref stmnt_get_reply_user_and_comment_id;
	statement_ref stmnt_delete_reply;
	statement_ref stmnt_get_event_media;
	statement_ref stmnt_clear_events_replies;
	statement_ref stmnt_clear_events_comments;
	statement_ref stmnt_get_event_media_ids;
	statement_ref stmnt_delete_event_media;
	statement_ref stmnt_get_event_organiser_and_cover_image;
	statement_ref stmnt_add_media;
	statement_ref stmnt_set_cover_image;
	statement_ref stmnt_delete_cover_image;
	statement_ref stmnt_delete_media;
	statement_ref stmnt_get_media_event_id;
	statement_ref stmnt_get_user_profile;
	statement_ref stmnt_save_user_profile;
	statement_ref stmnt_set_ppic;
	statement_ref stmnt_get_ppic;
	statement_ref stmnt_count_public_events;
	statement_ref stmnt_get_user_privilege;
	statement_ref stmnt_set_user_privilege;
	statement_ref stmnt_get_homepage_name;
	statement_ref stmnt_get_homepage_location;
	statement_ref stmnt_get_homepage_name_and_location;
	statement_ref stmnt_add_user_follow;
	statement_ref stmnt_remove_user_follow;
	statement_ref stmnt_is_following;
	statement_ref stmnt_get_followers;
	statement_ref stmnt_is_event_public;

	statement_ref stmnt_antispam_get_accounts_created;
	statement_ref stmnt_antispam_new_ip;
	statement_ref stmnt_antispam_account_created;


	Users::PasswordHash* pwd_hash; // points to d_ref_hash contents
	data_ref d_ref_hash;

	IPv6Address* ip; // points to d_ref_ip contents
	data_ref d_ref_ip;

	data_ref d_ref_session_token;

	data_ref d_ref_pwd_reset_code;

	// data_ref d_ref_synd_req_blob;

	MediaID* media_id; // points to d_ref_media_id contents
	data_ref d_ref_media_id;

	ConnectionData();
	
	

};

ConnectionData& open_database();

}