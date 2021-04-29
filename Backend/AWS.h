#pragma once

#include <string>

void aws_init();
void aws_deinit();

void delete_s3_object(std::string name);
std::string put_s3_object(std::string id, std::string file_name);
