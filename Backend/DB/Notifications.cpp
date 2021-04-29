#include "DB_.h"

namespace DB {

pair<vector<Notification>, uint32_t> get_notifications_for_user(Users::ID user_id, optional<uint32_t> from_id)
{
	auto connection_data = open_database();
	vector<Notification> notifications;

	result_set_ref result;

	connection_data.stmnt_last_seen_notification->set_unsigned32(0, user_id);
	result = connection_data.stmnt_last_seen_notification->query();
	assert_(result->next());

	uint32_t prev_last_seen_id = result->get_unsigned32("last_seen_notification");


	connection_data.stmnt_notifications_from_id->set_unsigned32(0, user_id);
	if(from_id.has_value()) {
		connection_data.stmnt_notifications_from_id->set_unsigned32(1, from_id.value());
	}
	else {
		connection_data.stmnt_notifications_from_id->set_unsigned32(1, prev_last_seen_id+1);
	}
	result = connection_data.stmnt_notifications_from_id->query();


	uint32_t biggest_id = 0;

	while (result->next()) {
		Notification n;
		n.id = result->get_unsigned32("id");

		if(n.id > biggest_id) {
			biggest_id = n.id;
		}

		n.type = static_cast<Notification::Type>(result->get_unsigned8("type"));
		n.time_since = result->get_unsigned64("time_since");

		if(!result->get_is_null("organiser_name"))
			n.organiser_name = result->get_string("organiser_name");
		if(!result->get_is_null("event_name"))
			n.event_name = result->get_string("event_name");
		if(!result->get_is_null("event_id"))
			n.event_id = result->get_unsigned32("event_id");
		notifications.push_back(move(n));
	}	

	return {notifications, prev_last_seen_id};
}

void mark_notifications_seen(Users::ID user_id, uint32_t latest_id)
{
	auto connection_data = open_database();
	connection_data.stmnt_set_last_seen_notification->set_unsigned32(0, latest_id);
	connection_data.stmnt_set_last_seen_notification->set_unsigned32(1, user_id);
	connection_data.stmnt_set_last_seen_notification->execute();
}

void create_notification(Users::ID user_id, Notification const& n)
{
	auto connection_data = open_database();

	connection_data.stmnt_create_notification->set_unsigned32(0, user_id);
	connection_data.stmnt_create_notification->set_unsigned8(1, n.type);

	if(n.event_id.has_value())
		connection_data.stmnt_create_notification->set_unsigned32(4, n.event_id.value());
	else
		connection_data.stmnt_create_notification->set_null(4);

	if(n.organiser_name.has_value())
		connection_data.stmnt_create_notification->set_string(2, n.organiser_name.value());
	else
		connection_data.stmnt_create_notification->set_null(2);

	if(n.event_name.has_value())
		connection_data.stmnt_create_notification->set_string(3, n.event_name.value());
	else
		connection_data.stmnt_create_notification->set_null(3);

	auto affected = connection_data.stmnt_create_notification->execute();
	if(affected < 1) return;

	connection_data.stmnt_get_user_name_email_from_id->set_unsigned32(0, user_id);
	auto result = connection_data.stmnt_get_user_name_email_from_id->query();

	if(result->next() && result->get_unsigned8("unread_notifications_email_sent") == 0) {
		using namespace Emails;
		send_email(make_unique<UnreadNotificationsEmail>(result->get_string("email"), result->get_string("name")));
		connection_data.stmnt_set_user_notification_email_sent->set_unsigned32(0, user_id);
		connection_data.stmnt_set_user_notification_email_sent->execute();
	}

}

}