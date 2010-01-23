// Copyright 2009 (c) Dean Michael Berris <mikhailberis@gmail.com>
// Copyright 2009 (c) Tarroo, Inc.
// Adapted from Christopher Kholhoff's Boost.Asio Example, released under
// the Boost Software License, Version 1.0. (See acccompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_NETWORK_HTTP_CONNECTION_HPP_
#define BOOST_NETWORK_HTTP_CONNECTION_HPP_

#ifndef BOOST_HTTP_SERVER_BUFFER_SIZE
#define BOOST_HTTP_SERVER_BUFFER_SIZE 1024
#endif

#include <boost/enable_shared_from_this.hpp>
#include <boost/network/protocol/http/request_parser.hpp>
#include <boost/network/protocol/http/request.hpp>
#include <boost/network/protocol/http/header.hpp>
#include <boost/network/protocol/http/response.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/bind.hpp>

namespace boost { namespace network { namespace http {

    using boost::asio::io_service;
    namespace system = boost::system;
    using boost::asio::ip::tcp;
    using boost::array;
    using boost::tribool;
    using boost::tuple;
    namespace tuples = boost::tuples;
    using boost::tuples::tie;
    using boost::bind;
    using boost::to_lower_copy;

    template <class Tag, class Handler>
    struct connection : boost::enable_shared_from_this<connection<Tag,Handler> > {

        connection(io_service & service, Handler & handler)
        : service_(service)
        , handler_(handler)
        , socket_(service_)
        , wrapper_(service_)
        {
        }

        tcp::socket & socket() {
            return socket_;
        }

        void start() {
            // This is HTTP so we really want to just
            // read and parse a request that's incoming
            // and then pass that request object to the
            // handler_ instance.
            //
            boost::system::error_code option_error;
            socket_.set_option(tcp::no_delay(true), option_error);
            if (option_error) handler_.log(system::system_error(option_error).what());
            socket_.async_read_some(
                boost::asio::buffer(buffer_),
                wrapper_.wrap(
                    bind(
                        &connection<Tag,Handler>::handle_read_headers,
                        connection<Tag,Handler>::shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred
                        )
                    )
                );
        }

        private:

        struct is_content_length {
            template <class Header>
            bool operator()(Header const & header) {
                return to_lower_copy(header.name) == "content-length";
            }
        };

        void handle_read_headers(system::error_code const &ec, size_t bytes_transferred) {
            if (!ec) {
                tribool done;
                tie(done,tuples::ignore) = parser_.parse_headers(request_, buffer_.data(), buffer_.data() + bytes_transferred);
                if (done) {
                    if (request_.method[0] == 'P') {
                        // look for the content-length header
                        std::vector<request_header>::iterator it = 
                            find_if(
                                request_.headers.begin(), 
                                request_.headers.end(),
                                is_content_length()
                                );
                        if (it == request_.headers.end()) {
                            response_= basic_response<Tag>::stock_reply(basic_response<Tag>::bad_request);
                            boost::asio::async_write(
                                socket_,
                                response_.to_buffers(),
                                wrapper_.wrap(
                                    bind(
                                        &connection<Tag,Handler>::handle_write,
                                        connection<Tag,Handler>::shared_from_this(),
                                        boost::asio::placeholders::error
                                        )
                                    )
                                );
                            return;
                        }

                        size_t content_length = 0;

                        try {
                            content_length = boost::lexical_cast<size_t>(it->value);
                        } catch (...) {
                            response_= basic_response<Tag>::stock_reply(basic_response<Tag>::bad_request);
                            boost::asio::async_write(
                                socket_,
                                response_.to_buffers(),
                                wrapper_.wrap(
                                    bind(
                                        &connection<Tag,Handler>::handle_write,
                                        connection<Tag,Handler>::shared_from_this(),
                                        boost::asio::placeholders::error
                                        )
                                    )
                                );
                            return;
                        }

                        if (content_length != 0) {
                            async_read(
                                socket_,
                                boost::asio::buffer(buffer_),
                                boost::asio::transfer_at_least(content_length),
                                wrapper_.wrap(
                                    bind(
                                        &connection<Tag,Handler>::handle_read_body_contents,
                                        connection<Tag,Handler>::shared_from_this(),
                                        boost::asio::placeholders::error,
                                        content_length,
                                        boost::asio::placeholders::bytes_transferred
                                        )
                                    )
                                );
                            return;
                        }

                        handler_(request_, response_);
                        boost::asio::async_write(
                            socket_,
                            response_.to_buffers(),
                            wrapper_.wrap(
                                bind(
                                    &connection<Tag,Handler>::handle_write,
                                    connection<Tag,Handler>::shared_from_this(),
                                    boost::asio::placeholders::error
                                    )
                                )
                            );
                    } else {
                        handler_(request_, response_);
                        boost::asio::async_write(
                            socket_,
                            response_.to_buffers(),
                            wrapper_.wrap(
                                bind(
                                    &connection<Tag,Handler>::handle_write,
                                    connection<Tag,Handler>::shared_from_this(),
                                    boost::asio::placeholders::error
                                    )
                                )
                            );
                    }
                } else if (!done) {
                    response_= basic_response<Tag>::stock_reply(basic_response<Tag>::bad_request);
                    boost::asio::async_write(
                        socket_,
                        response_.to_buffers(),
                        wrapper_.wrap(
                            bind(
                                &connection<Tag,Handler>::handle_write,
                                connection<Tag,Handler>::shared_from_this(),
                                boost::asio::placeholders::error
                                )
                            )
                        );
                } else {
                    socket_.async_read_some(
                        boost::asio::buffer(buffer_),
                        wrapper_.wrap(
                            bind(
                                &connection<Tag,Handler>::handle_read_headers,
                                connection<Tag,Handler>::shared_from_this(),
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred
                                )
                            )
                        );
                }
            }
            // TODO Log the error?
        }

        void handle_read_body_contents(boost::system::error_code const & ec, size_t bytes_to_read, size_t bytes_transferred) {
            if (!ec) {
                size_t difference = bytes_to_read - bytes_transferred;
                request_.body.append(buffer_.begin(), buffer_.end());
                if (difference == 0) {
                    handler_(request_, response_);
                    boost::asio::async_write(
                        socket_,
                        response_.to_buffers(),
                        wrapper_.wrap(
                            bind(
                                &connection<Tag,Handler>::handle_write,
                                connection<Tag,Handler>::shared_from_this(),
                                boost::asio::placeholders::error
                                )
                            )
                        );
                } else {
                    socket_.async_read_some(
                        boost::asio::buffer(buffer_),
                        wrapper_.wrap(
                            bind(
                                &connection<Tag,Handler>::handle_read_body_contents,
                                connection<Tag,Handler>::shared_from_this(),
                                boost::asio::placeholders::error,
                                difference,
                                boost::asio::placeholders::bytes_transferred
                                )
                            )
                        );
                }
            }
            // TODO Log the error?
        }

        void handle_write(boost::system::error_code const & ec) {
            if (!ec) {
                boost::system::error_code ignored_ec;
                socket_.shutdown(tcp::socket::shutdown_both, ignored_ec);
            }
        }

        io_service & service_;
        Handler & handler_;
        tcp::socket socket_;
        io_service::strand wrapper_;
        array<char,BOOST_HTTP_SERVER_BUFFER_SIZE> buffer_;
        request_parser parser_;
        basic_request<Tag> request_;
        basic_response<Tag> response_;
    };


} // namespace http

} // namespace network

} // namespace boost

#endif // BOOST_NETWORK_HTTP_CONNECTION_HPP_

