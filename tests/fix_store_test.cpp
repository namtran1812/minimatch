#include "minimatch/fix_store.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

namespace {

using minimatch::FixSessionSnapshot;
using minimatch::FixStore;
using minimatch::StoredFixMessage;

std::filesystem::path database_path() {
  return std::filesystem::temp_directory_path() /
      "minimatch_fix_store_test.sqlite";
}

void remove_database() {
  std::filesystem::remove(database_path());
}

TEST(FixStore, SavesAndLoadsSessionSequenceState) {
  remove_database();

  {
    FixStore store(
        database_path().string()
    );

    store.save_session(
        FixSessionSnapshot{
            .session_id =
                "MINIMATCH->CLIENT",
            .next_incoming_sequence = 7,
            .next_outgoing_sequence = 9,
            .last_received_time_ns = 100,
            .last_sent_time_ns = 200
        }
    );
  }

  {
    FixStore store(
        database_path().string()
    );

    const auto snapshot =
        store.load_session(
            "MINIMATCH->CLIENT"
        );

    EXPECT_EQ(
        snapshot.next_incoming_sequence,
        7
    );

    EXPECT_EQ(
        snapshot.next_outgoing_sequence,
        9
    );

    EXPECT_EQ(
        snapshot.last_received_time_ns,
        100U
    );

    EXPECT_EQ(
        snapshot.last_sent_time_ns,
        200U
    );
  }

  remove_database();
}

TEST(FixStore, ReturnsDefaultsForUnknownSession) {
  remove_database();

  FixStore store(
      database_path().string()
  );

  const auto snapshot =
      store.load_session(
          "UNKNOWN"
      );

  EXPECT_EQ(snapshot.session_id, "UNKNOWN");
  EXPECT_EQ(snapshot.next_incoming_sequence, 1);
  EXPECT_EQ(snapshot.next_outgoing_sequence, 1);

  remove_database();
}

TEST(FixStore, PersistsInboundAndOutboundMessages) {
  remove_database();

  FixStore store(
      database_path().string()
  );

  store.append_message(
      StoredFixMessage{
          .session_id =
              "MINIMATCH->CLIENT",
          .direction = "IN",
          .sequence_number = 1,
          .message_type = "A",
          .wire_message = "INBOUND",
          .timestamp_ns = 100
      }
  );

  store.append_message(
      StoredFixMessage{
          .session_id =
              "MINIMATCH->CLIENT",
          .direction = "OUT",
          .sequence_number = 1,
          .message_type = "A",
          .wire_message = "OUTBOUND",
          .timestamp_ns = 110
      }
  );

  const auto messages =
      store.messages_for_session(
          "MINIMATCH->CLIENT"
      );

  ASSERT_EQ(messages.size(), 2U);

  EXPECT_EQ(messages[0].direction, "IN");
  EXPECT_EQ(messages[1].direction, "OUT");

  remove_database();
}

TEST(FixStore, LoadsOutboundMessagesFromSequence) {
  remove_database();

  FixStore store(
      database_path().string()
  );

  for (int sequence = 1;
       sequence <= 4;
       ++sequence) {
    store.append_message(
        StoredFixMessage{
            .session_id =
                "MINIMATCH->CLIENT",
            .direction = "OUT",
            .sequence_number = sequence,
            .message_type = "8",
            .wire_message =
                "MESSAGE-" +
                std::to_string(sequence),
            .timestamp_ns =
                static_cast<std::uint64_t>(
                    sequence * 100
                )
        }
    );
  }

  const auto messages =
      store.outbound_messages_from_sequence(
          "MINIMATCH->CLIENT",
          3
      );

  ASSERT_EQ(messages.size(), 2U);

  EXPECT_EQ(messages[0].sequence_number, 3);
  EXPECT_EQ(messages[1].sequence_number, 4);

  remove_database();
}

TEST(FixStore, UpdatesExistingSessionSnapshot) {
  remove_database();

  FixStore store(
      database_path().string()
  );

  store.save_session(
      FixSessionSnapshot{
          .session_id =
              "MINIMATCH->CLIENT",
          .next_incoming_sequence = 2,
          .next_outgoing_sequence = 2
      }
  );

  store.save_session(
      FixSessionSnapshot{
          .session_id =
              "MINIMATCH->CLIENT",
          .next_incoming_sequence = 8,
          .next_outgoing_sequence = 11
      }
  );

  const auto snapshot =
      store.load_session(
          "MINIMATCH->CLIENT"
      );

  EXPECT_EQ(
      snapshot.next_incoming_sequence,
      8
  );

  EXPECT_EQ(
      snapshot.next_outgoing_sequence,
      11
  );

  remove_database();
}

}  // namespace
