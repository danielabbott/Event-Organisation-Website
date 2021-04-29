#define SERVER_RESOURCES_CPP
#include "Server.h"

#include <stdexcept>

using namespace std;

namespace WebServer {

Resource::CallbackReturn response__die(RequestHandle req, ResponseHandle res)
{
	(void)req;
	(void)res;
	throw std::runtime_error("DEATH");
	return {200, std::string(), MimeType::ASCII};
}

Resource::CallbackReturn response__shutdown(RequestHandle req, ResponseHandle res)
{
	(void)req;
	(void)res;
	exit(0);
}

Resource::CallbackReturn response__hello(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__my_ip(RequestHandle, ResponseHandle);

Resource::CallbackReturn response__sign_up(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__log_in(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__my_basic_info(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__logout(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__get_password_reset_link(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__verify_password_reset_link(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__password_reset(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__user_profile(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__save_user_profile(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__new_profile_picture(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__remove_profile_picture(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__set_user_priv(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__follow(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__unfollow(RequestHandle, ResponseHandle);


Resource::CallbackReturn response__homepage_events(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__my_events(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__event(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__event_edit_info(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__save_event(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__register_attendance(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__unregister_attendance(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__attendees(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__delete_event(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__upcoming_events(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__comments(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__replies(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__post_comment(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__post_reply(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__delete_comment(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__delete_reply(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__add_cover_image(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__delete_cover_image(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__delete_media(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__add_media(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__events_invited_to(RequestHandle, ResponseHandle);



// Resource::CallbackReturn response__unseen_notifications(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__notifications(RequestHandle, ResponseHandle);
Resource::CallbackReturn response__mark_notifications_seen(RequestHandle, ResponseHandle);

std::vector<Resource> resources = {
	Resource { Method::Get, "/d/hello", response__hello},

#ifdef DEBUG
	Resource { Method::Get, "/d/die", response__die},
	Resource { Method::Get, "/d/shutdown", response__shutdown},
#endif

	Resource { Method::Get, "/d/my_ip", response__my_ip},

	Resource { Method::Post, "/d/signup.json", response__sign_up},
	Resource { Method::Post, "/d/login.json", response__log_in},
	Resource { Method::Get, "/d/my_basic_info.json", response__my_basic_info},
	Resource { Method::Post, "/d/logout", response__logout},
	Resource { Method::Post, "/d/get_password_reset_link", response__get_password_reset_link},
	Resource { Method::Get, "/d/verify_password_reset_link.json", response__verify_password_reset_link},
	Resource { Method::Post, "/d/password_reset", response__password_reset},
	Resource { Method::Get, "/d/user_profile.json", response__user_profile},
	Resource { Method::Post, "/d/save_profile", response__save_user_profile},
	Resource { Method::Post, "/d/new_profile_picture.json", response__new_profile_picture},
	Resource { Method::Post, "/d/remove_profile_picture", response__remove_profile_picture},
	Resource { Method::Post, "/d/set_user_priv", response__set_user_priv},
	Resource { Method::Post, "/d/follow", response__follow},
	Resource { Method::Post, "/d/unfollow", response__unfollow},


	Resource { Method::Get, "/d/homepage_events.json", response__homepage_events},
	Resource { Method::Get, "/d/my_events.json", response__my_events},
	Resource { Method::Get, "/d/event.json", response__event},
	Resource { Method::Get, "/d/event_edit_info.json", response__event_edit_info},
	Resource { Method::Post, "/d/save_event", response__save_event},
	Resource { Method::Post, "/d/register_attendance", response__register_attendance},
	Resource { Method::Post, "/d/unregister_attendance", response__unregister_attendance},
	Resource { Method::Get, "/d/attendees.json", response__attendees},
	Resource { Method::Post, "/d/delete_event", response__delete_event},
	Resource { Method::Get, "/d/upcoming_events.json", response__upcoming_events},
	Resource { Method::Get, "/d/comments.json", response__comments},
	Resource { Method::Get, "/d/replies.json", response__replies},
	Resource { Method::Post, "/d/post_comment", response__post_comment},
	Resource { Method::Post, "/d/post_reply", response__post_reply},
	Resource { Method::Post, "/d/delete_comment", response__delete_comment},
	Resource { Method::Post, "/d/delete_reply", response__delete_reply},
	Resource { Method::Post, "/d/add_cover_image.json", response__add_cover_image},
	Resource { Method::Post, "/d/delete_cover_image", response__delete_cover_image},
	Resource { Method::Post, "/d/delete_media", response__delete_media},
	Resource { Method::Post, "/d/add_media.json", response__add_media},
	Resource { Method::Get, "/d/events_invited_to.json", response__events_invited_to},
	



	// Resource { Method::Get, "/d/unseen_notifications", response__unseen_notifications},
	Resource { Method::Get, "/d/notifications.json", response__notifications},
	Resource { Method::Post, "/d/mark_notifications_seen", response__mark_notifications_seen}

};

}