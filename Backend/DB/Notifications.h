#pragma once

#include <string>
#include <optional>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <variant>
#include "../Users.h"
#include "../IP.h"
#include "../ErrorMaybe.h"

namespace DB {

uint32_t unseen_notifications(Users::ID);

struct Notification {
	uint32_t id;
	enum Type {
		None, NewEvent, EventChanged, EventDeleted
	};
	Type type = Type::None;

	uint64_t time_since; // milliseconds

	std::optional<EventID> event_id; // NewEvent, EventChanged
	std::optional<std::string> organiser_name; // NewEvent
	std::optional<std::string> event_name; // NewEvent, EventChanged, EventDeleted
};

// if from_id is nullopt then fetches unseen notifications
// ^ set to 0 or 1 to get all notifications
// Returns notifications and id of last notification seen before these were requested
std::pair<std::vector<Notification>, uint32_t> get_notifications_for_user(Users::ID, std::optional<uint32_t> from_id = std::nullopt);

void mark_notifications_seen(Users::ID, uint32_t latest_id);

void create_notification(Users::ID, Notification const&);

}