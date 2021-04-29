DROP DATABASE eventorg;
CREATE DATABASE eventorg;
USE eventorg;

# Uncomment this if the cpp user has not been created yet
-- CREATE USER 'cpp'@'localhost' IDENTIFIED BY 'Yq0_Cy1y~#MBca@5Gm.vp';

GRANT SELECT ON eventorg.* TO 'cpp'@'localhost';
GRANT INSERT ON eventorg.* TO 'cpp'@'localhost';
GRANT UPDATE ON eventorg.* TO 'cpp'@'localhost';
GRANT DELETE ON eventorg.* TO 'cpp'@'localhost';


CREATE TABLE Users (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    email VARCHAR(255) NOT NULL,
    UNIQUE KEY (`email`),
    password BINARY(52) NOT NULL,
    profile_picture_id BINARY(17) DEFAULT NULL,

    name VARCHAR(40) NOT NULL,
    bio VARCHAR(255) DEFAULT NULL,
    privilege_level TINYINT DEFAULT 0,
    last_seen_notification INT UNSIGNED NOT NULL DEFAULT 0,
    unread_notifications_email_sent BOOL NOT NULL DEFAULT FALSE,

    timestamp_last_event_creation TIMESTAMP DEFAULT NULL NULL,
    number_of_events_created_this_hour TINYINT UNSIGNED NOT NULL DEFAULT 0
);

CREATE INDEX index_users_info ON Users (id, name, profile_picture_id);


CREATE TABLE Events (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    organiser_id INT UNSIGNED NOT NULL,
    is_published BOOL NOT NULL DEFAULT FALSE,
    is_public BOOL NOT NULL DEFAULT FALSE,
    description TEXT DEFAULT NULL,
    description2 TEXT DEFAULT NULL,
    cover_image_id BINARY(17) DEFAULT NULL,
    event_date_time DATETIME NOT NULL,
    event_time_zone TINYINT NOT NULL,

#   In minutes
    event_duration SMALLINT NOT NULL DEFAULT 60,

    gps_lat FLOAT DEFAULT NULL,
    gps_long FLOAT DEFAULT NULL,

#   null = online, url should be non-null in that case
    country TINYINT UNSIGNED DEFAULT NULL,

    address VARCHAR(255) DEFAULT NULL,
    postcode VARCHAR(11) DEFAULT NULL,
    url VARCHAR(255) DEFAULT NULL,
    youtube_video_code CHAR(11) DEFAULT NULL,
    hide_attendees BOOL NOT NULL DEFAULT FALSE,
    feedback_window_duration TINYINT UNSIGNED NOT NULL DEFAULT 24,

    timestamp_last_details_change TIMESTAMP NOT NULL,
    number_of_details_changes_this_hour TINYINT UNSIGNED NOT NULL DEFAULT 0,
    timestamp_last_poll_created TIMESTAMP NOT NULL DEFAULT 0,
    number_of_polls_created_this_hour TINYINT UNSIGNED NOT NULL DEFAULT 0
);
CREATE INDEX index_event_organiser ON Events (organiser_id);

# User has registered as going
CREATE TABLE EventAttendance (
    event_id INT UNSIGNED,
    user_id INT UNSIGNED,
    PRIMARY KEY (event_id, user_id)
);

# User invited to event but has not registered as going
CREATE TABLE EventInvitations (
    event_id INT UNSIGNED NOT NULL,
    email VARCHAR(255) NOT NULL,
    PRIMARY KEY(event_id, email),
    is_declined BOOL DEFAULT FALSE
);

CREATE TABLE Comments (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY, 
    event_id INT UNSIGNED,
    comment_text TEXT NOT NULL,
    user_id INT UNSIGNED NOT NULL,
    number_of_replies TINYINT NOT NULL DEFAULT 0,
    last_edited TIMESTAMP NULL,
    t TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX Comments_event_id ON Comments (event_id);

CREATE TABLE Replies (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    event_id INT UNSIGNED NOT NULL, -- matches parent_comment_id->event_id
    parent_comment_id INT UNSIGNED,
    comment_text TEXT NOT NULL,
    user_id INT UNSIGNED NOT NULL,
    last_edited TIMESTAMP NULL,
    t TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX Replies_parent_comment_id ON Replies (parent_comment_id);


CREATE TABLE Polls (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY, 
    event_id INT UNSIGNED NOT NULL,
    question VARCHAR(255) NOT NULL,
    answer_1 VARCHAR(255) NOT NULL,
    answer_2 VARCHAR(255) NOT NULL,
    answer_3 VARCHAR(255) DEFAULT NULL,
    answer_4 VARCHAR(255) DEFAULT NULL,
    answer_5 VARCHAR(255) DEFAULT NULL,
    answer_6 VARCHAR(255) DEFAULT NULL
);

CREATE TABLE PollAnswers (
    poll_id INT UNSIGNED,
    answer_number TINYINT UNSIGNED,
    user_id INT UNSIGNED,
    PRIMARY KEY (poll_id, answer_number, user_id)
);

CREATE TABLE Follows (
    followed_user_id INT UNSIGNED,
    follower_user_id INT UNSIGNED,
    PRIMARY KEY (followed_user_id, follower_user_id)
);

CREATE TABLE PasswordResets (
    unique_code BINARY(16) PRIMARY KEY,
    reset_request_timestamp TIMESTAMP NOT NULL,

    INDEX(reset_request_timestamp),
    user_id INT UNSIGNED NOT NULL,

#   Copied from User record
    email VARCHAR(255) NOT NULL
);


CREATE TABLE Feedback (
    event_id INT UNSIGNED,
    user_id INT UNSIGNED,
    feedback TEXT NOT NULL,
    PRIMARY KEY (event_id, user_id)
);

-- Resets every day
CREATE TABLE Spam (
    ip BINARY(16) PRIMARY KEY,
    accounts_created SMALLINT UNSIGNED NOT NULL DEFAULT 0
);

SET GLOBAL event_scheduler=ON;
CREATE EVENT auto_wipe_spam
  ON SCHEDULE
    EVERY 1 DAY
    STARTS '2021-01-29 15:15:00' ON COMPLETION PRESERVE ENABLE 
  DO
    DELETE FROM Spam;

CREATE TABLE Sessions (
    token BINARY(16) PRIMARY KEY, 
    session_timestamp TIMESTAMP NOT NULL,
    user_id INT UNSIGNED NOT NULL,
    ip BINARY(16) NOT NULL
);

# If is_cover_image then id == event->cover_image_id
# If event_id is null then this is a profile picture
CREATE TABLE Media (
    id BINARY(17) PRIMARY KEY, 
    event_id INT UNSIGNED,
    file_name VARCHAR(255) NOT NULL,
    is_cover_image BOOL NOT NULL DEFAULT FALSE
);

CREATE INDEX event_media_event_id ON Media (event_id); 


CREATE TABLE Notifications (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY, 
    user_id INT UNSIGNED NOT NULL,

    type TINYINT NOT NULL,
    organiser_name VARCHAR(40) DEFAULT NULL,
    event_name VARCHAR(255) DEFAULT NULL,
    event_id INT UNSIGNED DEFAULT NULL,

    t timestamp NOT NULL
);

CREATE INDEX index_notification_user_id ON Notifications (user_id, id);

# Constraints

ALTER TABLE Users ADD CONSTRAINT fk_user_profile_pic
FOREIGN KEY (profile_picture_id) REFERENCES Media (id)
ON DELETE SET NULL
ON UPDATE CASCADE;


ALTER TABLE Events ADD CONSTRAINT fk_event_organiser_id 
FOREIGN KEY (organiser_id) REFERENCES Users (id)
ON DELETE CASCADE
ON UPDATE CASCADE;

ALTER TABLE Events ADD CONSTRAINT fk_event_cover_image
FOREIGN KEY (cover_image_id) REFERENCES Media (id)
ON DELETE SET NULL
ON UPDATE CASCADE;


ALTER TABLE EventAttendance ADD CONSTRAINT fk_event_attendance_event_id
FOREIGN KEY (event_id) REFERENCES Events (id)
ON DELETE CASCADE
ON UPDATE CASCADE;

ALTER TABLE EventAttendance ADD CONSTRAINT fk_event_attendance_user_id
FOREIGN KEY (user_id) REFERENCES Users (id)
ON DELETE CASCADE
ON UPDATE CASCADE;


ALTER TABLE EventInvitations ADD CONSTRAINT fk_event_invitations_event_id
FOREIGN KEY (event_id) REFERENCES Events (id)
ON DELETE CASCADE
ON UPDATE CASCADE;


ALTER TABLE Sessions ADD CONSTRAINT fk_sessions_user_id
FOREIGN KEY (user_id) REFERENCES Users (id)
ON DELETE CASCADE
ON UPDATE CASCADE;


ALTER TABLE Comments ADD CONSTRAINT fk_comments_event_id
FOREIGN KEY (event_id) REFERENCES Events (id)
ON DELETE CASCADE
ON UPDATE CASCADE;

ALTER TABLE Comments ADD CONSTRAINT fk_comments_user_id
FOREIGN KEY (user_id) REFERENCES Users (id)
ON DELETE CASCADE
ON UPDATE CASCADE;



ALTER TABLE Replies ADD CONSTRAINT fk_replies_user_id
FOREIGN KEY (user_id) REFERENCES Users (id)
ON DELETE CASCADE
ON UPDATE CASCADE;

ALTER TABLE Replies ADD CONSTRAINT fk_replies_parent
FOREIGN KEY (parent_comment_id) REFERENCES Comments (id)
ON DELETE CASCADE
ON UPDATE CASCADE;


ALTER TABLE Polls ADD CONSTRAINT fk_polls_event_id
FOREIGN KEY (event_id) REFERENCES Events (id)
ON DELETE CASCADE
ON UPDATE CASCADE;


ALTER TABLE PollAnswers ADD CONSTRAINT fk_poll_answers_poll_id
FOREIGN KEY (poll_id) REFERENCES Polls (id)
ON DELETE CASCADE
ON UPDATE CASCADE;

ALTER TABLE PollAnswers ADD CONSTRAINT fk_poll_answers_user_id
FOREIGN KEY (user_id) REFERENCES Users (id)
ON DELETE CASCADE
ON UPDATE CASCADE;


ALTER TABLE Follows ADD CONSTRAINT fk_follows_followed_id
FOREIGN KEY (followed_user_id) REFERENCES Users (id)
ON DELETE CASCADE
ON UPDATE CASCADE;

ALTER TABLE Follows ADD CONSTRAINT fk_follower_followed_id
FOREIGN KEY (follower_user_id) REFERENCES Users (id)
ON DELETE CASCADE
ON UPDATE CASCADE;


ALTER TABLE PasswordResets ADD CONSTRAINT fk_password_resets_user_id
FOREIGN KEY (user_id) REFERENCES Users (id)
ON DELETE CASCADE
ON UPDATE CASCADE;


ALTER TABLE Feedback ADD CONSTRAINT fk_feedback_user_id
FOREIGN KEY (user_id) REFERENCES Users (id)
ON DELETE CASCADE
ON UPDATE CASCADE;

ALTER TABLE Feedback ADD CONSTRAINT fk_feedback_event_id
FOREIGN KEY (event_id) REFERENCES Events (id)
ON DELETE CASCADE
ON UPDATE CASCADE;


ALTER TABLE Media ADD CONSTRAINT fk_media_event_id
FOREIGN KEY (event_id) REFERENCES Events (id)
ON DELETE CASCADE
ON UPDATE CASCADE;


ALTER TABLE Notifications ADD CONSTRAINT fk_notifications_user_id
FOREIGN KEY (user_id) REFERENCES Users (id)
ON DELETE CASCADE
ON UPDATE CASCADE;
