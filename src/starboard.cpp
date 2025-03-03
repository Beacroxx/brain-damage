#include "starboard.hpp"
#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>
#include <algorithm>
#include <dpp/dpp.h>

template <typename EventType>
dpp::task<void> updateStarboardMessage(custom_cluster &bot, const EventType &event) {
  std::unique_lock<std::mutex> lock(bot.starboard_mutex, std::defer_lock);

  // If the mutex is already locked, wait for it to unlock
  if (!lock.try_lock()) {
    std::cout << "Starboard: Waiting for mutex..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    lock.lock();
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    std::cout << "Starboard: Mutex acquired after " << duration.count() << "ms" << std::endl;
  }

  // Get the message, channel, and star count
  auto msg_result = co_await bot.co_message_get(event.message_id, event.channel_id);
  const dpp::message msg = std::get<dpp::message>(msg_result.value);
  const auto starCountIt = std::find_if(msg.reactions.begin(), msg.reactions.end(),
                                        [](const dpp::reaction &r) { return r.emoji_name == "⭐"; });
  const int starCount = starCountIt != msg.reactions.end() ? starCountIt->count : 0;
  auto channel_result = co_await bot.co_channel_get(event.channel_id);
  const dpp::channel channel = std::get<dpp::channel>(channel_result.value);
  const long timestamp = msg.get_creation_time();

  // Check if the message is already in the starboard
  auto &starboard = bot.starboard;
  const bool starboarded = starboard.find(msg.get_url()) != starboard.end();

  // If the message has less than 2 stars and a reaction has been removed, remove it from the starboard
  if (starCount < 2) {
    if (starboarded && std::is_same_v<EventType, dpp::message_reaction_remove_t>) {
      co_await bot.co_message_delete(starboard[msg.get_url()].id, starboard[msg.get_url()].channel_id);
      starboard.erase(msg.get_url());
      auto thread_it = bot.starboard_threads.find(msg.get_url());
      if (thread_it != bot.starboard_threads.end()) {
        bot.starboard_threads.erase(msg.get_url());
      }
    }
    co_return;
  }

  // Create the embed message
  dpp::embed e;
  e.set_author(msg.author.username, msg.author.get_url(), msg.author.get_avatar_url());
  e.set_color(dpp::colors::yellow);
  e.set_timestamp(timestamp);

  // Get the referenced message
  const dpp::message::message_ref reference = msg.message_reference;
  dpp::message ref;
  if (!reference.message_id.empty()) {
    auto ref_result = co_await bot.co_message_get(reference.message_id, event.channel_id);
    ref = std::get<dpp::message>(ref_result.value);
  }

  // Format the referenced message content
  std::string refcontent = ref.content;
  if (!refcontent.empty()) {
    refcontent = "> **" + ref.author.username + ":**" + "\n" + refcontent;
    std::stringstream tmp;
    std::for_each(refcontent.begin(), refcontent.end(), [&tmp](char c) {
      if (c == '\n')
        tmp << "\n> ";
      else
        tmp << c;
    });
    refcontent = tmp.str();
  }

  // Create the message
  e.set_description((refcontent.empty() ? "" : refcontent + "\n\n") + msg.content + "\n\n");

  // Add attachments
  if (!msg.attachments.empty()) {
    const dpp::attachment a = msg.attachments.at(0);
    if (a.content_type.find("image") != std::string::npos) {
      e.set_image(a.url);
    } else if (a.content_type.find("video") != std::string::npos) {
      e.set_video(a.url);
    } else {
      e.add_field("Attachment", a.url);
    }
  }

  // Check if the message is starred
  if (starboarded) {
    // Edit the starboard message
    dpp::message &m = starboard[msg.get_url()];
    m.embeds.at(0) = e;
    m.set_content("⭐ **" + std::to_string(starCount) + "** | [`# " + channel.name + "`](<" + msg.get_url() + ">)");
    auto m_res = co_await bot.co_message_edit(m);
    m = std::get<dpp::message>(m_res.value);
  } else if (starCount == 2 && std::is_same_v<EventType, dpp::message_reaction_add_t>) {
    // Post in starboard channel
    auto c_res = co_await bot.co_channel_get(bot.get_config().at("starboardChannel").get<dpp::snowflake>());
    const dpp::channel starboard_channel = std::get<dpp::channel>(c_res.value);
    auto m_res2 = co_await bot.co_message_create(dpp::message("⭐ **" + std::to_string(starCount) + "** | [`# " +
      channel.name + "`](<" + msg.get_url() + ">)")
      .add_embed(e)
      .set_channel_id(starboard_channel.id));
    starboard[msg.get_url()] = std::get<dpp::message>(m_res2.value);

    // Remove the message after 3 days ( thread magic )
    std::string url = msg.get_url();
    auto thread = std::make_shared<std::thread>([botPtr = &bot, url]() {
      std::this_thread::sleep_for(std::chrono::hours(24 * 3));
      std::unique_lock<std::mutex> lock(botPtr->starboard_mutex, std::defer_lock);
      if (!lock.try_lock()) {
        std::cout << "Thread: Waiting for mutex..." << std::endl;
        auto start = std::chrono::high_resolution_clock::now();
        lock.lock();
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
        std::cout << "Thread: Mutex acquired after " << duration.count() << "ms" << std::endl;
      }
      auto &starboard = botPtr->starboard;
      if (starboard.find(url) != starboard.end() &&
           botPtr->starboard_threads.find(url) != botPtr->starboard_threads.end()) {
        starboard.erase(url);
      }
      botPtr->starboard_threads.erase(url);
    });

    // Store the thread and detach it
    bot.starboard_threads[msg.get_url()] = thread;
    thread->detach();
  }
}

// Explicit template instantiation
template dpp::task<void> updateStarboardMessage<dpp::message_reaction_add_t>(custom_cluster &bot, const dpp::message_reaction_add_t &event);
template dpp::task<void> updateStarboardMessage<dpp::message_reaction_remove_t>(custom_cluster &bot, const dpp::message_reaction_remove_t &event);