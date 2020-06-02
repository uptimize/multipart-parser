#ifndef _MULTIPART_READER_H_
#define _MULTIPART_READER_H_

#include <map>
#include <utility>
#include <functional>
#include "MultipartParser.h"

class MultipartHeaders: public std::multimap<std::string, std::string> {
private:
	std::string empty;
public:
	const std::string &operator[](const std::string &key) const {
		const_iterator it = find(key);
		if (it == end()) {
			return empty;
		} else {
			return it->second;
		}
	}
};

class MultipartReader {
public:
  using PartBeginCallback = std::function<void(const MultipartHeaders &)>;
  using PartDataCallback = std::function<void(const char *, size_t)>;
  using Callback = std::function<void()>;

private:
	MultipartParser parser;
	bool headersProcessed;
	MultipartHeaders currentHeaders;
	std::string currentHeaderName, currentHeaderValue;

	void resetReaderCallbacks() {
		onPartBegin = nullptr;
		onPartData  = nullptr;
		onPartEnd   = nullptr;
		onEnd       = nullptr;
	}

	void setParserCallbacks() {
		parser.onPartBegin   = cbPartBegin;
		parser.onHeaderField = cbHeaderField;
		parser.onHeaderValue = cbHeaderValue;
		parser.onHeaderEnd   = cbHeaderEnd;
		parser.onHeadersEnd  = cbHeadersEnd;
		parser.onPartData    = cbPartData;
		parser.onPartEnd     = cbPartEnd;
		parser.onEnd         = cbEnd;
		parser.userData      = this;
	}

	static void cbPartBegin(const char *buffer, size_t start, size_t end, void *userData) {
		MultipartReader *self = (MultipartReader *) userData;
		self->headersProcessed = false;
		self->currentHeaders.clear();
		self->currentHeaderName.clear();
		self->currentHeaderValue.clear();
	}

	static void cbHeaderField(const char *buffer, size_t start, size_t end, void *userData) {
		MultipartReader *self = (MultipartReader *) userData;
		self->currentHeaderName.append(buffer + start, end - start);
	}

	static void cbHeaderValue(const char *buffer, size_t start, size_t end, void *userData) {
		MultipartReader *self = (MultipartReader *) userData;
		self->currentHeaderValue.append(buffer + start, end - start);
	}

	static void cbHeaderEnd(const char *buffer, size_t start, size_t end, void *userData) {
		MultipartReader *self = (MultipartReader *) userData;
		self->currentHeaders.insert(std::make_pair(self->currentHeaderName,
			self->currentHeaderValue));
		self->currentHeaderName.clear();
		self->currentHeaderValue.clear();
	}

	static void cbHeadersEnd(const char *buffer, size_t start, size_t end, void *userData) {
		MultipartReader *self = (MultipartReader *) userData;
		if (self->onPartBegin != nullptr) {
			self->onPartBegin(self->currentHeaders);
		}
		self->currentHeaders.clear();
		self->currentHeaderName.clear();
		self->currentHeaderValue.clear();
	}

	static void cbPartData(const char *buffer, size_t start, size_t end, void *userData) {
		MultipartReader *self = (MultipartReader *) userData;
		if (self->onPartData != nullptr) {
			self->onPartData(buffer + start, end - start);
		}
	}

	static void cbPartEnd(const char *buffer, size_t start, size_t end, void *userData) {
		MultipartReader *self = (MultipartReader *) userData;
		if (self->onPartEnd != nullptr) {
			self->onPartEnd();
		}
	}

	static void cbEnd(const char *buffer, size_t start, size_t end, void *userData) {
		MultipartReader *self = (MultipartReader *) userData;
		if (self->onEnd != nullptr) {
			self->onEnd();
		}
	}

public:
	PartBeginCallback onPartBegin;
	PartDataCallback onPartData;
	Callback onPartEnd;
	Callback onEnd;

	MultipartReader(const std::string &boundary): parser(boundary) {
		resetReaderCallbacks();
		setParserCallbacks();
	}

	void reset() {
		parser.reset();
	}

	void setBoundary(const std::string &boundary) {
		parser.setBoundary(boundary);
	}

	size_t feed(const char *buffer, size_t len) {
		return parser.feed(buffer, len);
	}

	bool succeeded() const {
		return parser.succeeded();
	}

	bool hasError() const {
		return parser.hasError();
	}

	bool stopped() const {
		return parser.stopped();
	}

  std::string getErrorMessage() const {
		return parser.getErrorMessage();
	}
};

#endif /* _MULTIPART_READER_H_ */
