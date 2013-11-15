// Copyright (C) 2013 by Glyn Matthews
// Copyright Dean Michael Berris 2012.
// Copyright Google, Inc. 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 * \file
 * \brief An HTTP client and configuration options.
 */

#ifndef __NETWORK_HTTP_V2_CLIENT_CLIENT_INC__
#define __NETWORK_HTTP_V2_CLIENT_CLIENT_INC__

#include <future>
#include <memory>
#include <cstdint>
#include <algorithm>
#include <string>
#include <vector>
#include <chrono>
#include <boost/asio/io_service.hpp>
#include <boost/optional.hpp>
#include "network/http/v2/client/request.hpp"
#include "network/http/v2/client/response.hpp"

namespace network {
  namespace http {
    namespace v2 {
      /**
       * \ingroup http_client
       * \class client_options network/http/v2/client/client.hpp
       * \brief A set of options to configure an HTTP client.
       */
      class client_options {

      public:

        /**
         * \brief Constructor.
         */
        client_options()
          : io_service_(boost::none)
          , follow_redirects_(false)
          , cache_resolved_(false)
          , use_proxy_(false)
          , timeout_(30000) { }

        /**
         * \brief Copy constructor.
         */
        client_options(client_options const &) = default;

        /**
         * \brief Move constructor.
         */
        client_options(client_options &&) = default;

        /**
         * \brief Assignment operator.
         */
        client_options &operator = (client_options other) {
          other.swap(*this);
          return *this;
        }

        /**
         * \brief Destructor.
         */
        ~client_options() noexcept {

        }

        /**
         * \brief Swap.
         */
        void swap(client_options &other) noexcept {
          using std::swap;
          std::swap(io_service_, other.io_service_);
          swap(follow_redirects_, other.follow_redirects_);
          swap(cache_resolved_, other.cache_resolved_);
          swap(use_proxy_, other.use_proxy_);
          swap(timeout_, other.timeout_);
          swap(openssl_certificate_paths_, other.openssl_certificate_paths_);
          swap(openssl_verify_paths_, other.openssl_verify_paths_);
        }

        /**
         * \brief Overrides the client's I/O service.
         * \param io_service The new io_service object to use.
         */
        client_options &io_service(boost::asio::io_service &io_service) {
          io_service_ = io_service;
          return *this;
        }

        /**
         * \brief Gets the overridden I/O service.
         * \returns An optional io_service object.
         */
        boost::optional<boost::asio::io_service &> io_service() const {
          return io_service_;
        }

        /**
         * \brief Tells the client to follow redirects.
         * \param follow_redirects If \c true, then the client must
         *        follow redirects, if \c false it doesn't.
         * \returns \c *this
         */
        client_options &follow_redirects(bool follow_redirects) noexcept {
          follow_redirects_ = follow_redirects;
          return *this;
        }

        /**
         * \brief Tests if the client follows redirects.
         * \returns \c true if the client follows redirects, \c false
         *          otherwise.
         */
        bool follow_redirects() const noexcept {
          return follow_redirects_;
        }

        /**
         * \brief Tells the client to cache resolved connections.
         * \param cache_resolved If \c true, then the client must
         *        cache resolved connections, if \c false it
         *        doesn't.
         * \returns \c *this
         */
        client_options &cache_resolved(bool cache_resolved) noexcept {
          cache_resolved_ = cache_resolved;
          return *this;
        }

        /**
         * \brief Tests if the client caches resolved connections.
         * \returns \c true if the client caches resolved connections,
         *          \c false otherwise.
         */
        bool cache_resolved() const noexcept {
          return cache_resolved_;
        }

        /**
         * \brief Tells the client to use a proxy.
         * \param use_proxy If \c true, then the client must use a
         *        proxy, if \c false it doesn't.
         */
        client_options &use_proxy(bool use_proxy) noexcept {
          use_proxy_ = use_proxy;
          return *this;
        }

        /**
         * \brief Tests if the client uses a proxy.
         * \returns \c true if the client uses a proxy, \c false
         *          otherwise.
         */
        bool use_proxy() const noexcept {
          return use_proxy_;
        }

        /**
         * \brief Sets the client timeout in milliseconds.
         * \param timeout The timeout value in milliseconds.
         */
        client_options &timeout(std::chrono::milliseconds timeout) noexcept {
          timeout_ = timeout;
          return *this;
        }

        /**
         * \brief Gets the current timeout value.
         * \returns The timeout value in milliseconds.
         */
        std::chrono::milliseconds timeout() const noexcept {
          return timeout_;
        }

        /**
         * \brief Adds an OpenSSL certificate path.
         * \param path The certificate path.
         */
        client_options &openssl_certificate_path(std::string path) {
          openssl_certificate_paths_.emplace_back(std::move(path));
          return *this;
        }

        /**
         * \brief Returns a list of OpenSSL certificate paths.
         * \returns A list of OpenSSL certificate paths.
         */
        std::vector<std::string> openssl_certificate_paths() const {
          return openssl_certificate_paths_;
        }

        /**
         * \brief Adds an OpenSSL verify path.
         * \param path The verify path.
         */
        client_options &openssl_verify_path(std::string path) {
          openssl_verify_paths_.emplace_back(std::move(path));
          return *this;
        }

        /**
         * \brief Returns a list of OpenSSL verify paths.
         * \returns A list of OpenSSL verify paths.
         */
        std::vector<std::string> openssl_verify_paths() const {
          return openssl_verify_paths_;
        }

      private:

        boost::optional<boost::asio::io_service &> io_service_;
        bool follow_redirects_;
        bool cache_resolved_;
        bool use_proxy_;
        std::chrono::milliseconds timeout_;
        std::vector<std::string> openssl_certificate_paths_;
        std::vector<std::string> openssl_verify_paths_;

      };

      inline
      void swap(client_options &lhs, client_options &rhs) noexcept {
        lhs.swap(rhs);
      }

      template <typename Handler, typename Signature>
      struct handler_type { };

      /**
       * \ingroup http_client
       * \class client network/http/v2/client/client.hpp
       * \brief A class that encapsulates the operations and methods
       *        for communicating with an HTTP servier.
       */
      class client {

        client(client const &) = delete;
        client(client &&) = delete;

      public:

        /**
         * \typedef The client's string_type.
         */
        typedef request::string_type string_type;

        /**
         * Constructor.
         * \params options Client options.
         */
        explicit client(client_options options = client_options());

        /**
         * Destructor.
         */
        ~client() noexcept;

        /**
         * \brief Makes an HTTP GET request.
         * \param request_ The request object.
         * \param options The request options.
         */
        std::future<response> get(request request_, request_options options = request_options());

        /**
         * \brief Makes an HTTP POST request.
         * \param request_ The request object.
         * \param options The request options.
         */
        std::future<response> post(request request_, request_options options = request_options());

        /**
         * \brief Makes an HTTP PUT request.
         * \param request_ The request object.
         * \param options The request options.
         */
        std::future<response> put(request request_, request_options options = request_options());

        /**
         * \brief Makes an HTTP DELETE request.
         * \param request_ The request object.
         * \param options The request options.
         */
        std::future<response> delete_(request request_, request_options options = request_options());

        /**
         * \brief Makes an HTTP HEAD request.
         * \param request_ The request object.
         * \param options The request options.
         */
        std::future<response> head(request request_, request_options options = request_options());

        /**
         * \brief Makes an HTTP OPTIONS request.
         * \param request_ The request object.
         * \param options The request options.
         */
        std::future<response> options(request request_, request_options options = request_options());

      private:

        struct impl;
        impl *pimpl_;

      };
    } // namespace v2
  } // namespace http
} // namespace network

#endif // __NETWORK_HTTP_V2_CLIENT_CLIENT_INC__