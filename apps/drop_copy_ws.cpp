#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <regex>
#include <string>
#include <thread>
#include <vector>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;

using tcp = asio::ip::tcp;

namespace {

struct Event {
  std::uint64_t id{0};
  std::string json;
};

std::string fetch_drop_copy(
    const std::string& host,
    const std::string& port
) {
  asio::io_context io;

  tcp::resolver resolver(io);

  beast::tcp_stream stream(io);

  const auto endpoints =
      resolver.resolve(
          host,
          port
      );

  stream.connect(
      endpoints
  );

  http::request<
      http::empty_body
  > request{
      http::verb::get,
      "/api/drop-copy",
      11
  };

  request.set(
      http::field::host,
      host
  );

  request.set(
      http::field::user_agent,
      "minimatch-drop-copy-ws"
  );

  http::write(
      stream,
      request
  );

  beast::flat_buffer buffer;

  http::response<
      http::string_body
  > response;

  http::read(
      stream,
      buffer,
      response
  );

  beast::error_code ec;

  stream.socket().shutdown(
      tcp::socket::
          shutdown_both,
      ec
  );

  return response.body();
}

std::vector<Event>
parse_events(
    const std::string& body
) {
  std::vector<Event> result;

  std::size_t pos = 0;

  while (true) {
    const auto begin =
        body.find(
            '{',
            pos
        );

    if (
        begin ==
        std::string::npos
    ) {
      break;
    }

    const auto end =
        body.find(
            '}',
            begin
        );

    if (
        end ==
        std::string::npos
    ) {
      break;
    }

    const auto object =
        body.substr(
            begin,
            end - begin + 1
        );

    static const std::regex
        id_pattern(
            R"("id":([0-9]+))"
        );

    std::smatch match;

    if (
        std::regex_search(
            object,
            match,
            id_pattern
        )
    ) {
      result.push_back(
          Event{
              .id =
                  std::stoull(
                      match[1].str()
                  ),
              .json =
                  object
          }
      );
    }

    pos =
        end + 1;
  }

  return result;
}

void run_session(
    tcp::socket socket
) {
  websocket::stream<
      tcp::socket
  > ws(
      std::move(socket)
  );

  ws.set_option(
      websocket::
          stream_base::
              timeout::
                  suggested(
                      beast::role_type::
                          server
                  )
  );

  beast::flat_buffer request_buffer;

  http::request<
      http::string_body
  > request;

  http::read(
      ws.next_layer(),
      request_buffer,
      request
  );

  std::uint64_t last_seen_id =
      0;

  const std::string target =
      std::string(
          request.target()
      );

  const std::string key =
      "lastSeenId=";

  const auto pos =
      target.find(
          key
      );

  if (
      pos !=
      std::string::npos
  ) {
    const auto begin =
        pos + key.size();

    const auto end =
        target.find(
            '&',
            begin
        );

    const auto value =
        target.substr(
            begin,
            end == std::string::npos
                ? std::string::npos
                : end - begin
        );

    try {
      last_seen_id =
          std::stoull(
              value
          );
    } catch (...) {
      last_seen_id = 0;
    }
  }

  ws.accept(
      request
  );

  std::cerr
      << "Drop-copy client connected"
      << " lastSeenId="
      << last_seen_id
      << "\n";

  while (true) {
    try {
      const auto body =
          fetch_drop_copy(
              "127.0.0.1",
              "8081"
          );

      const auto events =
          parse_events(
              body
          );

      // REST response is newest-first.
      // Iterate backwards so WS emits oldest-first.
      for (
          auto iterator =
              events.rbegin();
          iterator !=
              events.rend();
          ++iterator
      ) {
        if (
            iterator->id <=
            last_seen_id
        ) {
          continue;
        }

        ws.write(
            asio::buffer(
                iterator->json
            )
        );

        last_seen_id =
            iterator->id;
      }

      std::this_thread::
          sleep_for(
              std::chrono::
                  milliseconds(
                      100
                  )
          );

    } catch (
        const beast::
            system_error& error
    ) {
      if (
          error.code() ==
          websocket::error::
              closed
      ) {
        break;
      }

      throw;
    }
  }

  std::cerr
      << "Drop-copy client disconnected\n";
}

}  // namespace

int main(
    int argc,
    char** argv
) {
  try {
    const unsigned short port =
        argc > 1
            ? static_cast<
                  unsigned short
              >(
                  std::stoul(
                      argv[1]
                  )
              )
            : 8092;

    asio::io_context io;

    tcp::acceptor acceptor(
        io,
        {
            tcp::v4(),
            port
        }
    );

    std::cout
        << "MiniMatch drop-copy WebSocket "
        << "listening on ws://127.0.0.1:"
        << port
        << "\n";

    while (true) {
      tcp::socket socket(
          io
      );

      acceptor.accept(
          socket
      );

      std::thread(
          run_session,
          std::move(socket)
      ).detach();
    }

  } catch (
      const std::exception& ex
  ) {
    std::cerr
        << "Drop-copy WebSocket failed: "
        << ex.what()
        << "\n";

    return 1;
  }
}
