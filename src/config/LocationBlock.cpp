#include "../../inc/config/LocationBlock.hpp"

Location::Location()
    : path("")
    , root("")
    , index("")
    , client_max_body_size(0)
    , allowed_methods()
    , autoindex(false)
    , upload_dir("")
    , redirect_code(0)
    , redirect_url("")
    , cgi_pass()
{
}