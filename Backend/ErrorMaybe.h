#pragma once

#include "Assert.h"
#include <optional>

enum class ErrorType {
	Success,
	GenericError,
	InvalidParameter,
	MissingParameter,
	OutOfMemory,
	NotLoggedIn,
	PermissionsError,
	NotFound,
	Timeout,
	TooManyRequests,
	ResourceLimitations,
	BadRequest
};

static inline unsigned short int get_http_error(ErrorType error) {
	switch(error) {
		case ErrorType::Success:
			return 200;
		case ErrorType::GenericError:
		case ErrorType::OutOfMemory:
		case ErrorType::Timeout:
		case ErrorType::ResourceLimitations:
			return 500;
		case ErrorType::InvalidParameter:
		case ErrorType::MissingParameter:
		case ErrorType::BadRequest:
			return 400;
		case ErrorType::NotLoggedIn:
			return 401;
		case ErrorType::PermissionsError:
			return 403;
		case ErrorType::NotFound:
			return 404;
		case ErrorType::TooManyRequests:
			return 429;
	}
	return 500;
}

template <typename T>
class ErrorMaybe
{
	std::optional<T> x;


	ErrorType error = ErrorType::Success;

public:
	constexpr ErrorMaybe(T && t) : x(std::forward<T>(t)) {}

	ErrorMaybe() noexcept : error(ErrorType::GenericError) {}
	ErrorMaybe(ErrorType e) noexcept : error(e) {}

	bool has_value() const noexcept {
		return x.has_value();
	}

	T & value() {
		assert_(has_value());
		return x.value();
	}

	std::optional<T> & get_optional() noexcept {
		return x;
	}

	ErrorType get_error() const noexcept {
		return error;
	}

	unsigned short int get_http_error() const noexcept {
		return ::get_http_error(error);
	}


};
		
