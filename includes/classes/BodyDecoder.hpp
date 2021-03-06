#ifndef BODYDECODER_HPP
#define BODYDECODER_HPP

#include "HttpRequest.hpp"
#include "macros.h"
#include <cstdlib>

/*
 How to use it:
- Instantiate it with a HttpRequest
Call decode_body(), check return value.
- 1 means all the body content was transfered from Client's received_data
into decoded_body.
- 0 means there's still content to be fetched, so you need to do one more
IO round before calling it again (use the same instance of this class -
keep it inside your request handler instance).

Inside the do_io loop the decode_body buffer will be emptied (data transfered
to a CGI pipe or to a file being uploaded, and the received_data buffer might
be filled with new data received from the client socket.
*/

class BodyDecoder
{
  private:
	std::string &raw_data;	   // ref to Client's received_data buffer
	std::string &decoded_data; // ref to Client's decoded_body buffer
	size_t length_decoded;
	enum e_type
	{
		chunked,
		content_length,
		other
	} type;

	// Content-Length type
	long content_len;
	long bytes_left;

	// Transfer-Encoding chunked type
	long bytes_left_last_chunk;
	bool removeCRLF;
	bool done;

	// methods
	e_type resolve_type(HttpRequest &request);
	int decode_known_len();
	int decode_chunked();
	int transfer_n_bytes(long n);
	int finish();

  public:
	BodyDecoder(HttpRequest &request);
	~BodyDecoder();

	int decode_body();
	size_t getLengthDecoded() const;
};

#endif
