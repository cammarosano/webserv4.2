#ifndef AREQUESTHANDLER_HPP
#define AREQUESTHANDLER_HPP

#include <ctime>

#include "FdManager.hpp"
#include "HttpRequest.hpp"
#include "macros.h"
#include "utils.h"
#include <iomanip>

// forward declaration
struct HttpRequest;
class FdManager;

/*
Abstract class for request handlers
Request handlers for a specific kind of response (ex: serve a static file)
inherit from this class and define the respond() and abort() methods
*/
class AReqHandler {
   protected:
    HttpRequest *request;
    Client &client;
    FdManager &table;
    size_t bytes_recvd; // from file in disk or CGI

    struct HttpResponse {
        std::string http_version;
        std::string status_code_phrase;
        std::map<std::string, std::string> header_fields;

        std::string header_str; // assemble_header_str() fills this up
        void assemble_header_str();
        void assemble_partial_header_str();
    } response;

    int send_str(std::string &str);
    bool response100_expected();

   public:
    AReqHandler(HttpRequest *request, FdManager &table);
    virtual ~AReqHandler();

    virtual int respond() = 0;
    virtual int time_out_code();

    bool keep_alive;
    void add_to_bytes_recvd(size_t n);

    static std::string get_mime_type(const std::string &file_name);
    static std::string get_timestamp();

	// maps extensions to content-type
    static std::map<std::string, std::string> content_type;
};

#endif
