#pragma once

#include <llvm/Support/Error.h>
#include <string>
#include <utility>
#include <exception>
#include <system_error>

class llvm_error : std::exception {
	llvm::Error err;
	std::string prefix;

public:
	llvm_error(llvm::Error err, const char* msg_prefix = "") : err(std::move(err)), prefix(msg_prefix) {}

	llvm::Error error() {
		return std::move(err);
	}
	const std::string& msg_prefix() const {
		return prefix;
	}
};

class llvm_ec_error : llvm_error {
public:
	llvm_ec_error(std::error_code errc, const char* msg_prefix = "")
	: llvm_error(llvm::errorCodeToError(errc), msg_prefix) {}
};

class llvm_string_error : llvm_error {
public:
	llvm_string_error(const std::string& msg, const char* msg_prefix = "")
	: llvm_error(llvm::make_error<llvm::StringError>(msg, std::error_code{}), msg_prefix) {}
};

class filetype_error : llvm_string_error {
public:
	filetype_error(const std::string& msg)
	: llvm_string_error(msg, "Invalid file type: ") {}
};
