#include "ARequestHandler.hpp"

AReqHandler::AReqHandler(HttpRequest *request, FdManager &table)
    : request(request),
      client(request->client),
      table(table),
      client_disconnected(false),
      bytes_sent(0) {
    update_last_io_activ();
}

AReqHandler::~AReqHandler() {}

HttpRequest *AReqHandler::getRequest() { return request; }

void AReqHandler::HttpResponse::assemble_header_str() {
    typedef std::map<std::string, std::string>::iterator iterator;

	// default-additions
    if (http_version.empty())
        http_version = "HTTP/1.1";
    header_fields["server"] = "webserv";

    // status-line
    header_str =
        http_version + ' ' + status_code_phrase + "\r\n";

    // header-fiels
    for (iterator it = header_fields.begin();
         it != header_fields.end(); ++it)
        header_str += it->first + ": " + it->second + "\r\n";

    // end header
    header_str += "\r\n";

    // log status-line
    std::cout << "Response: " << http_version << " "
              << status_code_phrase << std::endl;
}


bool AReqHandler::response100_expected() {
    std::map<std::string, std::string>::iterator it =
        request->header_fields.find("expect");

    if (it != request->header_fields.end() &&
        str_tolower(it->second) == "100-continue")
        return (true);
    return (false);
}

void AReqHandler::update_last_io_activ() { std::time(&last_io_activity); }

bool AReqHandler::is_time_out() {
    if (std::difftime(time(NULL), last_io_activity) > RESPONSE_TIME_OUT)
        return (true);
    return (false);
}

// trying to unlock a disconnectedd client
// or one locked by a different RH instance has no effect
void AReqHandler::unlock_client() {
    if (client_disconnected) return;
    if (client.rh_locked && client.ongoing_response == this) {
        client.rh_locked = false;
        client.ongoing_response = NULL;
    }
}

// trying to lock a already locked client raises an exception
void AReqHandler::lock_client() {
    if (client.rh_locked) throw(std::exception());
    client.rh_locked = true;
    client.ongoing_response = this;
}

void AReqHandler::disconnect_client() { client_disconnected = true; }

void AReqHandler::add_to_bytes_sent(size_t n) { bytes_sent += n; }

// transfer the content of a string to client.unsent_data (ex: http header, or
// an auto-generated html page)
// Returns 1 if complete, 0 otherwise
int AReqHandler::send_str(std::string &str) {
    Client &client = request->client;
    int max_bytes;

    max_bytes = BUFFER_SIZE - client.unsent_data.size();
    if (max_bytes <= 0) return 0;

    client.unsent_data += str.substr(0, max_bytes);
    str.erase(0, max_bytes);
    table.set_pollout(client.socket);
    if (str.empty()) return 1;
    return 0;
}

int AReqHandler::time_out_abort()
{
    abort();
    return (408); // in doubt, blame it on the client
}

// static variable
std::map<std::string, std::string> AReqHandler::content_type;

std::string get_extension(const std::string &file_name)
{
	size_t pos = file_name.rfind('.');
	std::string extension;

	// if found and not the last char
	if (pos != std::string::npos && pos < file_name.size() - 1)
		extension = file_name.substr(pos + 1);
	return (extension);
}

// returns the mime_type based on file_name's extension
std::string AReqHandler::get_mime_type(const std::string &file_name) const
{
    std::map<std::string, std::string>::iterator it;
    std::string ext = get_extension(file_name);

    if (ext.empty())  // no extension
        return (DEFAULT_MIME);
    if (ext == "html")
        return ("text/html");  // optimization: save a map look-up
    it = AReqHandler::content_type.find(ext);
    if (it == AReqHandler::content_type.end())  // ext not found
        return (DEFAULT_MIME);
    return (it->second);
}
