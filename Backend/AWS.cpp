#include <aws/core/Aws.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/utils/logging/LogLevel.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <optional>
#include <stdexcept>
#include <spdlog/spdlog.h>

using namespace Aws;
using namespace std;

constexpr const char * S3_BUCKET_NAME = "da-event-org";

static SDKOptions options;

void aws_init() {
    options.loggingOptions.logLevel = Utils::Logging::LogLevel::Error;
    InitAPI(options);
}

void aws_deinit() {
    ShutdownAPI(options);
}

struct AWSThreadData {
	S3::S3Client s3_client;

	AWSThreadData() {
	    Client::ClientConfiguration config;
	    config.scheme = Http::Scheme::HTTPS;

	    s3_client = S3::S3Client(config);
	}
};

thread_local optional<AWSThreadData> thread_data;

static AWSThreadData& get_data() {
	if(thread_data == nullopt) {
		thread_data = AWSThreadData();
	}
	return thread_data.value();
}


void delete_s3_object(string name) {
    spdlog::info("Delete S3 Object: {}", name);

	auto aws = get_data();

	S3::Model::DeleteObjectRequest request;
    request.SetBucket(S3_BUCKET_NAME);
    request.SetKey(name.c_str());

    auto outcome = aws.s3_client.DeleteObject(request);

    if (!outcome.IsSuccess()) {
    	spdlog::error("Error deleting AWS S3 object {}", name);
    	throw runtime_error("AWS S3 Delete Error");
    }
}


string put_s3_object(string id, string file_name)
{
    spdlog::debug("Create S3 Object: {} {}", id, file_name);

	auto aws = get_data();

	Aws::S3::Model::PutObjectRequest request;
    request.SetBucket(S3_BUCKET_NAME);
    request.SetKey(id.c_str());
	request.WithACL(Aws::S3::Model::ObjectCannedACL::public_read);

	auto outcome = aws.s3_client.PutObject(request);

	if (outcome.IsSuccess()) {
        Http::HeaderValueCollection headers;
        headers["x-amz-acl"] = "public-read";

        string dispo;
        headers["cache-control"] = "public, max-age=31536000, immutable";
        if(!file_name.empty()) {
            dispo = string("attachment; filename=\"") + file_name + string("\"");
            headers["content-disposition"] = dispo.c_str();
        }
        

        // TODO headers["content-length"] = file_size;
        
        long long expiration = 5*60; // seconds
        Aws::String url = aws.s3_client.GeneratePresignedUrl(S3_BUCKET_NAME, id.c_str(), Http::HttpMethod::HTTP_PUT, headers, expiration);
    	
    	return string(url.c_str(), url.size());
    }
    else 
    {
		spdlog::error("PutObject failed: {}", outcome.GetError().GetMessage());
		throw runtime_error("AWS S3 Put Error");
    }
}
