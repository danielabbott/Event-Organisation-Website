// Code adapted from https://stackoverflow.com/a/60766163/11498001

#include <cstdio>
#include <iostream>
#include <ctime>
#include <sstream>

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/CreateMultipartUploadRequest.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/s3/model/CompleteMultipartUploadRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/UploadPartRequest.h>


using namespace std;
using namespace Aws;
using namespace Aws::S3::Model;

// Minimum 5 MiB
char buffer[5*1024*1024];

int main() {
	Aws::SDKOptions options;
    Aws::InitAPI(options);

	Aws::Client::ClientConfiguration clientConfiguration;
	clientConfiguration.scheme = Http::Scheme::HTTPS;
	clientConfiguration.region = "us-east-1";
	Aws::S3::S3Client s3_client(clientConfiguration);

	// Generate file name using current date & time
	time_t t = time(0);
	tm* now = localtime(&t);
	stringstream ss;
	ss << "db-backup-" << (now->tm_year + 1900) << '-' << (now->tm_mon + 1) << '-' << now->tm_mday
	<< "--" << now->tm_hour << "-" << now->tm_min;
	string file_name = ss.str();



	// Create upload request

	Aws::S3::Model::CreateMultipartUploadRequest create_request;
	create_request.SetBucket("da-event-org");
	create_request.SetKey(file_name.c_str());
    create_request.SetContentType("text/plain");
	cout << "Backup file name: " << ss.str() << '\n';

	auto createMultipartUploadOutcome = s3_client.CreateMultipartUpload(create_request);

    string upload_id = createMultipartUploadOutcome.GetResult().GetUploadId();


    // Do upload

    vector<CompletedPart> completed_parts;

    unsigned int partN = 0;
    while(true) {
    	partN++;

    	// Get data from stdin
    	auto bytes = fread(buffer, 1, sizeof(buffer), stdin);
		if(!bytes) {
			bytes = fread(buffer, 1, sizeof(buffer), stdin);
			if(!bytes) {
				break;
			}
		}
		cout << "bytes: " << bytes << '\n';

		// Send data

		Aws::S3::Model::UploadPartRequest my_request;
		my_request.SetBucket("da-event-org");
		my_request.SetKey(file_name.c_str());
		my_request.SetPartNumber(partN);
		my_request.SetUploadId(upload_id.c_str());

		// string data("data to upload");
		auto stream_ptr =  Aws::MakeShared<Aws::StringStream>("WriteStream::Upload" /* log id */, string(buffer, bytes));

		my_request.SetBody(stream_ptr);

		Aws::Utils::ByteBuffer part_md5(Aws::Utils::HashingUtils::CalculateMD5(*stream_ptr));
		my_request.SetContentMD5(Aws::Utils::HashingUtils::Base64Encode(part_md5));

		my_request.SetContentLength(bytes);

		auto uploadPartOutcomeCallable1 = s3_client.UploadPartCallable(my_request);


		// Get 'CompletedPart' object and add to list

		UploadPartOutcome uploadPartOutcome1 = uploadPartOutcomeCallable1.get();
		CompletedPart completedPart1;
		completedPart1.SetPartNumber(partN);
		auto etag = uploadPartOutcome1.GetResult().GetETag();
		// if etag must not be empty
		assert(!etag.empty());
		completedPart1.SetETag(etag);
		completed_parts.push_back(completedPart1);

	}

	// Combine 'CompletedPart' objects

	CompletedMultipartUpload completedMultipartUpload;
	for(auto& p: completed_parts) {
		completedMultipartUpload.AddParts(p);
	}

	// Finish upload, check upload was successful

	Aws::S3::Model::CompleteMultipartUploadRequest completeMultipartUploadRequest;
	completeMultipartUploadRequest.SetBucket("da-event-org");
	completeMultipartUploadRequest.SetKey(file_name.c_str());
	completeMultipartUploadRequest.SetUploadId(upload_id.c_str());
	completeMultipartUploadRequest.WithMultipartUpload(completedMultipartUpload);

	auto completeMultipartUploadOutcome =
	  s3_client.CompleteMultipartUpload(completeMultipartUploadRequest);

	if (!completeMultipartUploadOutcome.IsSuccess()) {
		auto error = completeMultipartUploadOutcome.GetError();
		std::stringstream ss;
		ss << error << error.GetExceptionName() << ": " << error.GetMessage() << std::endl;
		cout << ss.str();
		return -1;
	}

	cout << "Success.\n";

	return 0;
}