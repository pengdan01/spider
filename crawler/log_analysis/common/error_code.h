#pragma once
#include <string>
namespace log_analysis {

const std::string kUploadSign = "UPLOAD";
const std::string kMapInputError = "MapInputError";
const std::string kRedInputError = "RedInputError";
// const std::string kBase64DecodeError = "Base64DecodeError";
// const std::string kUTF8DecodeError = "UTF8DecodeError";
const std::string kInvalidURL = "InvalidURL";
// const std::string kInvalidTime = "InvalidTime";
// const std::string kInvalidMid = "InvalidMid";
// const std::string kInvalidSid = "InvalidSid";
const std::string kInvalidQuery = "InvalidQuery";
// const std::string kEmptyResult = "EmptyResult";
const std::string kMD5Conflict = "MD5Conflict";
const std::string kDuplicateKey = "DuplicateKey";
const std::string kOutputFormatError = "OutputFormatError";

const int kRandStrLen = 20;
}  // namespace log_analysis
