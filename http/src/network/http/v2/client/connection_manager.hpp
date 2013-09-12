// Copyright (C) 2013 by Glyn Matthews
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef __NETWORK_HTTP_V2_CLIENT_CONNECTION_MANAGER_INC__
#define __NETWORK_HTTP_V2_CLIENT_CONNECTION_MANAGER_INC__

#include <memory>
#include <boost/asio/io_service.hpp>

namespace network {
  namespace http {
    namespace v2 {

      class request;
      class client_connection;
      class client_options;

      /**
       * \class connection_manager network/http/v2/client/connection_manager.hpp connection_manager.hpp
       * \brief A class that maintains client connections.
       */
      class connection_manager {

      public:

	typedef std::shared_ptr<connection> connection_ptr;

	connection_manager() = default;

	connection_manager(const connection_manager &) = delete;

	connection_manager &operator = (const connection_manager &) = delete;

	virtual connection_manager() noexcept = 0;

	virtual connection_ptr get_connection(boost::asio::io_service &io_service,
					      const request &req,
					      const client_options &options) = 0;

	virtual void clear_resolved_cache() = 0;

	virtual void reset() = 0;

      };
    } // namespace v2
  } // namespace http
} // namespace network


#endif // __NETWORK_HTTP_V2_CLIENT_CONNECTION_MANAGER_INC__