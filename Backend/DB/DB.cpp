#include <string>

#include "DB_.h"


namespace DB {


static thread_local optional<ConnectionData> thead_connection_data = nullopt;

void db_reconnect() {
	thead_connection_data = ConnectionData();
}


ConnectionData& open_database() {
	if(thead_connection_data == nullopt) {
		thead_connection_data = ConnectionData();
	}
	return thead_connection_data.value();
}

ConnectionData::ConnectionData()
	: con(connection::create(account::create("localhost", 
		get_setting(Setting::database_username), 
		get_setting(Setting::database_password), 
		get_setting(Setting::database_name))))
{
	d_ref_hash = make_shared<::mariadb::data<char>>(sizeof (Users::PasswordHash));

	pwd_hash = reinterpret_cast<Users::PasswordHash*>(d_ref_hash->get());

	d_ref_ip = make_shared<::mariadb::data<char>>(sizeof (IPv6Address));
	ip = reinterpret_cast<IPv6Address*>(d_ref_ip->get());

	d_ref_media_id = make_shared<::mariadb::data<char>>(sizeof (MediaID));
	media_id = reinterpret_cast<MediaID*>(d_ref_media_id->get());

	d_ref_session_token = make_shared<::mariadb::data<char>>(Users::SessionToken::LENGTH);
	d_ref_pwd_reset_code = make_shared<::mariadb::data<char>>(Users::PASSWORD_RESET_CODE_LENGTH);


	// d_ref_synd_req_blob = make_shared<::mariadb::data<char>>(66074);

	if(!ip || !pwd_hash || !d_ref_session_token->get() || !d_ref_pwd_reset_code->get()
		|| !media_id
		 // || !d_ref_synd_req_blob->get()
		) {
		throw OutOfMemoryException();
	}

	stmnt_add_user = con->create_statement("INSERT INTO Users (email,name,password) VALUES (?,?,?)");
	stmnt_get_user0 = con->create_statement("SELECT id, name, password FROM Users WHERE email=?");
	stmnt_create_session = con->create_statement("INSERT INTO Sessions (user_id,ip,token) VALUES (?,?,?)");
	stmnt_delete_session = con->create_statement("DELETE FROM Sessions WHERE token = ?");
	stmnt_get_user_id_from_session = con->create_statement("SELECT user_id from Sessions WHERE token = ?");
	stmnt_get_user_info_from_session = con->create_statement("SELECT id, name, profile_picture_id, email, privilege_level FROM Users WHERE id = (SELECT user_id from Sessions WHERE token = ?)");
	stmnt_get_user_basic_info_from_email = con->create_statement("SELECT id, name FROM Users WHERE email=?");
	stmnt_get_user_name_from_id = con->create_statement("SELECT name FROM Users WHERE id=?");
	stmnt_get_user_name_email_from_id = con->create_statement("SELECT name,email,unread_notifications_email_sent FROM Users WHERE id=?");
	stmnt_create_password_reset = con->create_statement("INSERT INTO PasswordResets (user_id,unique_code,email) VALUES (?,?,?)");
	stmnt_get_password_reset = con->create_statement("SELECT email, NOW()-reset_request_timestamp as t_delta FROM PasswordResets WHERE unique_code=?");
	stmnt_delete_password_resets = con->create_statement("DELETE FROM PasswordResets WHERE reset_request_timestamp <= NOW()-10*60");
	stmnt_do_password_reset1 = con->create_statement("SELECT user_id FROM PasswordResets WHERE unique_code=? AND reset_request_timestamp >= NOW()-10*60");
	stmnt_do_password_reset2 = con->create_statement("UPDATE Users SET password = ? WHERE id = ?");
	stmnt_kill_sessions = con->create_statement("DELETE FROM Sessions WHERE user_id = ?");
	stmnt_users_events = con->create_statement("SELECT id,name,organiser_id,postcode,url,LEFT(description,255) as description,event_date_time,event_time_zone,event_duration,country,postcode,gps_lat,gps_long,is_published,cover_image_id FROM Events WHERE organiser_id=? ORDER BY id DESC");
	stmnt_get_event = con->create_statement("SELECT Events.*, Users.name as organiser_name FROM Events INNER JOIN Users ON Events.organiser_id = Users.id WHERE Events.id=?");
	stmnt_get_event_invitations = con->create_statement("SELECT email FROM EventInvitations WHERE event_id=?");
	stmnt_add_invitation = con->create_statement("INSERT INTO EventInvitations (event_id, email) VALUES(?,?)");
	stmnt_remove_invitation = con->create_statement("DELETE FROM EventInvitations WHERE event_id=? AND email=?");
	stmnt_get_event_organiser = con->create_statement("SELECT organiser_id FROM Events WHERE id=?");
	stmnt_get_event_organiser_and_name = con->create_statement("SELECT organiser_id,name FROM Events WHERE id=?");
	stmnt_get_event_permissions_data = con->create_statement("SELECT organiser_id, is_published, is_public FROM Events WHERE id=?");
	stmnt_get_event_name = con->create_statement("SELECT name FROM Events WHERE id=?");
	stmt_register_attendance = con->create_statement("INSERT INTO EventAttendance (event_id, user_id) VALUES(?,?)");
	stmt_unregister_attendance = con->create_statement("DELETE FROM EventAttendance WHERE event_id=? AND user_id=?");
	stmnt_is_attendance_registered = con->create_statement("SELECT COUNT(*) as n FROM EventAttendance WHERE event_id=? AND user_id=?");
	stmnt_get_event_hide_attendees = con->create_statement("SELECT hide_attendees,organiser_id,is_public FROM Events WHERE id=?");
	stmnt_get_event_attendee_names = con->create_statement("SELECT name FROM Users WHERE id IN (SELECT user_id FROM EventAttendance WHERE event_id=?)");
	stmnt_get_event_attendee_ids = con->create_statement("SELECT user_id FROM EventAttendance WHERE event_id=?");
	stmnt_clear_event_attendance = con->create_statement("DELETE FROM EventAttendance WHERE event_id=?");
	stmnt_is_user_invited = con->create_statement("SELECT COUNT(*) as n FROM EventInvitations WHERE event_id=? AND email=(SELECT email FROM Users WHERE id=?)");
	stmnt_clear_event_invitations = con->create_statement("DELETE FROM EventInvitations WHERE event_id=?");
	stmnt_delete_event = con->create_statement("DELETE FROM Events WHERE id=?");
	stmnt_event_is_public = con->create_statement("SELECT is_public FROM Events WHERE id=?");
	stmnt_last_seen_notification = con->create_statement("SELECT last_seen_notification FROM Users WHERE id=?");
	stmnt_notifications_from_id = con->create_statement("SELECT id, type, organiser_name, event_name, event_id, (UNIX_TIMESTAMP(now())-UNIX_TIMESTAMP(t)) as time_since FROM Notifications WHERE user_id=? AND id >= ?");
	stmnt_set_last_seen_notification = con->create_statement("UPDATE Users SET last_seen_notification=?, unread_notifications_email_sent=FALSE WHERE id=?");
	stmnt_set_user_notification_email_sent = con->create_statement("UPDATE Users SET unread_notifications_email_sent=TRUE WHERE id=?");
	stmnt_users_upcoming_events = con->create_statement("SELECT id,name,organiser_id,postcode,url,LEFT(description,255) as description,event_date_time,event_time_zone,event_duration,country,postcode,gps_lat,gps_long,is_published,cover_image_id FROM Events WHERE id IN (SELECT event_id FROM EventAttendance WHERE user_id=?)");
	stmnt_users_invited_events = con->create_statement("SELECT id,name,organiser_id,postcode,url,LEFT(description,255) as description,event_date_time,event_time_zone,event_duration,country,postcode,gps_lat,gps_long,is_published,cover_image_id FROM Events WHERE id IN (SELECT event_id FROM EventInvitations WHERE email = (SELECT email FROM Users WHERE id=?))");
	stmnt_create_notification = con->create_statement("INSERT INTO Notifications (user_id, type, organiser_name, event_name, event_id) VALUES (?,?,?,?,?)");
	stmnt_get_recent_comments = con->create_statement(
		"SELECT Comments.id, Comments.comment_text, Comments.user_id, Comments.number_of_replies, " 
		"(UNIX_TIMESTAMP(now())-UNIX_TIMESTAMP(Comments.last_edited)) as time_since_edit, "
		"(UNIX_TIMESTAMP(now())-UNIX_TIMESTAMP(Comments.t)) as ago, Users.name, Users.profile_picture_id "
		"FROM Comments LEFT JOIN Users ON Comments.user_id = Users.id WHERE Comments.event_id = ? ORDER BY Comments.id DESC LIMIT 4"
	);
	stmnt_get_comments = con->create_statement(
		"SELECT Comments.id, Comments.comment_text, Comments.user_id, Comments.number_of_replies, " 
		"(UNIX_TIMESTAMP(now())-UNIX_TIMESTAMP(Comments.last_edited)) as time_since_edit, "
		"(UNIX_TIMESTAMP(now())-UNIX_TIMESTAMP(Comments.t)) as ago, Users.name, Users.profile_picture_id "
		"FROM Comments LEFT JOIN Users ON Comments.user_id = Users.id WHERE Comments.event_id = ? AND Comments.id <= ? ORDER BY Comments.id DESC LIMIT 4"
	);
	stmnt_get_replies = con->create_statement(
		"SELECT Replies.id, Replies.event_id, Replies.comment_text, Replies.user_id, " 
		"(UNIX_TIMESTAMP(now())-UNIX_TIMESTAMP(Replies.last_edited)) as time_since_edit, "
		"(UNIX_TIMESTAMP(now())-UNIX_TIMESTAMP(Replies.t)) as ago, Users.name, Users.profile_picture_id "
		"FROM Replies LEFT JOIN Users ON Replies.user_id = Users.id WHERE Replies.parent_comment_id = ?"
	);
	stmnt_post_comment = con->create_statement("INSERT INTO Comments (event_id, comment_text, user_id) VALUES (?,?,?)");
	stmnt_get_comment_data_for_replying = con->create_statement("SELECT Comments.event_id, Comments.number_of_replies, Events.organiser_id, Events.is_published, Events.is_public FROM Comments INNER JOIN Events ON Comments.event_id = Events.id WHERE Comments.id=?");
	stmnt_post_reply = con->create_statement("INSERT INTO Replies (event_id, parent_comment_id, comment_text, user_id) VALUES (?,?,?,?)");
	stmnt_update_number_of_replies = con->create_statement("UPDATE Comments SET number_of_replies=(SELECT COUNT(*) FROM Replies WHERE parent_comment_id=?) WHERE id=?");
	stmnt_delete_comment = con->create_statement("DELETE FROM Comments WHERE id=? AND (user_id=? OR (SELECT privilege_level FROM Users WHERE id=?) >= 1)");
	stmnt_delete_comment_replies = con->create_statement("DELETE FROM Replies WHERE parent_comment_id=?");
	stmnt_get_reply_user_and_comment_id = con->create_statement("SELECT user_id, parent_comment_id FROM Replies WHERE id=?");
	stmnt_delete_reply = con->create_statement("DELETE FROM Replies WHERE id=?");
	stmnt_get_event_media = con->create_statement("SELECT id, file_name, is_cover_image FROM Media WHERE event_id=?");
	stmnt_clear_events_replies = con->create_statement("DELETE FROM Replies WHERE event_id=?");
	stmnt_clear_events_comments = con->create_statement("DELETE FROM Comments WHERE event_id=?");
	stmnt_get_event_media_ids = con->create_statement("SELECT id FROM Media WHERE event_id=?");
	stmnt_delete_event_media = con->create_statement("DELETE FROM Media WHERE event_id=?");
	stmnt_get_event_organiser_and_cover_image = con->create_statement("SELECT organiser_id,cover_image_id FROM Events WHERE id=?");
	stmnt_add_media = con->create_statement("INSERT INTO Media (id,event_id,file_name,is_cover_image) VALUES(?,?,?,?)");
	stmnt_set_cover_image = con->create_statement("UPDATE Events SET cover_image_id=? WHERE id=?");
	stmnt_delete_cover_image = con->create_statement("UPDATE Events SET cover_image_id=null WHERE id=?");
	stmnt_delete_media = con->create_statement("DELETE FROM Media WHERE id=?");
	stmnt_get_media_event_id =  con->create_statement("SELECT event_id FROM Media WHERE id=?");
	stmnt_get_user_profile = con->create_statement("SELECT name,bio,profile_picture_id,privilege_level FROM Users WHERE id=?");
	stmnt_save_user_profile = con->create_statement("UPDATE Users SET name=?, bio=? WHERE id=?");
	stmnt_set_ppic = con->create_statement("UPDATE Users SET profile_picture_id=? WHERE id=?");
	stmnt_get_ppic = con->create_statement("SELECT profile_picture_id FROM Users WHERE id=?");
	stmnt_get_user_privilege = con->create_statement("SELECT privilege_level FROM Users WHERE id=?");
	stmnt_set_user_privilege = con->create_statement("UPDATE Users SET privilege_level=? WHERE id=?");
	stmnt_is_event_public = con->create_statement("SELECT is_public FROM Events WHERE id=?");

	stmnt_add_user_follow = con->create_statement("INSERT INTO Follows (followed_user_id, follower_user_id) VALUES (?,?)");
	stmnt_remove_user_follow = con->create_statement("DELETE FROM Follows WHERE followed_user_id=? AND follower_user_id=?");
	stmnt_is_following = con->create_statement("SELECT COUNT(*) FROM Follows WHERE followed_user_id=? AND follower_user_id=?");
	stmnt_get_followers = con->create_statement("SELECT follower_user_id FROM Follows WHERE followed_user_id=?");
	
	
	stmnt_recently_added_events = con->create_statement("SELECT id,name,organiser_id,postcode,url,LEFT(description,255) as description,event_date_time,event_time_zone,event_duration,country,postcode,gps_lat,gps_long,cover_image_id FROM Events WHERE is_public != 0 AND is_published != 0 ORDER BY id DESC LIMIT 10");
	
	stmnt_get_homepage_name = con->create_statement("SELECT id,name,organiser_id,postcode,url,LEFT(description,255) as description,event_date_time,event_time_zone,event_duration,country,postcode,gps_lat,gps_long,cover_image_id FROM Events WHERE LOWER(name) LIKE LOWER(?) AND is_public != 0 AND is_published != 0 AND id+100 > (SELECT MAX(id) FROM Events) ORDER BY id DESC LIMIT 10");
	stmnt_get_homepage_location = con->create_statement("SELECT id,name,organiser_id,postcode,url,LEFT(description,255) as description,event_date_time,event_time_zone,event_duration,country,postcode,gps_lat,gps_long,cover_image_id FROM Events WHERE LOWER(address) LIKE LOWER(?) AND is_public != 0 AND is_published != 0 AND id+100 > (SELECT MAX(id) FROM Events) ORDER BY id DESC LIMIT 10");
	stmnt_get_homepage_name_and_location = con->create_statement("SELECT id,name,organiser_id,postcode,url,LEFT(description,255) as description,event_date_time,event_time_zone,event_duration,country,postcode,gps_lat,gps_long,cover_image_id FROM Events WHERE LOWER(name) LIKE LOWER(?) AND LOWER(address) LIKE LOWER(?) AND is_public != 0 AND is_published != 0 AND id+100 > (SELECT MAX(id) FROM Events) ORDER BY id DESC LIMIT 10");
	
	stmnt_antispam_get_accounts_created = con->create_statement("SELECT accounts_created FROM Spam WHERE ip=?");
	stmnt_antispam_new_ip = con->create_statement("INSERT INTO Spam (ip) VALUES (?)");
	stmnt_antispam_account_created = con->create_statement("UPDATE Spam SET accounts_created=accounts_created+1 WHERE ip=?");
	
	
}

}
