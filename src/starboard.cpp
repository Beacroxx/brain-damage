#define VERBOSE_DEBUG

#include "starboard.hpp"
#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>
#include <algorithm>
#include <dpp/dpp.h>

#ifdef VERBOSE_DEBUG
#define LOG_DEBUG(msg) std::cout << "[DEBUG] " << msg << std::endl
#else
#define LOG_DEBUG(msg)
#endif

template <typename EventType>
void updateStarboardMessage(custom_cluster &bot, const EventType &event) {
  LOG_DEBUG("Attempting to lock starboard mutex");
  std::unique_lock<std::mutex> lock(bot.starboard_mutex, std::defer_lock);

  // If the mutex is already locked, wait for it to unlock
  if (!lock.try_lock()) {
    LOG_DEBUG("Starboard mutex is locked, waiting...");
    auto start = std::chrono::high_resolution_clock::now();
    lock.lock();
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    LOG_DEBUG("Starboard mutex acquired after " + std::to_string(duration.count()) + "ms");
  } else {
    LOG_DEBUG("Starboard mutex acquired immediately");
  }

  LOG_DEBUG("Fetching message details");
  // Get the message, channel, and star count
  const dpp::message msg = bot.message_get_sync(event.message_id, event.channel_id);
  const auto starCountIt = std::find_if(msg.reactions.begin(), msg.reactions.end(),
                                        [](const dpp::reaction &r) { return r.emoji_name == "⭐"; });
  const int starCount = starCountIt != msg.reactions.end() ? starCountIt->count : 0;
  const dpp::channel channel = bot.channel_get_sync(event.channel_id);
  const long timestamp = msg.get_creation_time();

  // Check if the message is already in the starboard
  auto &starboard = bot.starboard;
  const bool starboarded = starboard.find(msg.get_url()) != starboard.end();

  // If the message has less than 2 stars and a reaction has been removed, remove it from the starboard
  if (starCount < 2) {
    if (starboarded && std::is_same_v<EventType, dpp::message_reaction_remove_t>) {
      LOG_DEBUG("Removing message from starboard");
      bot.message_delete(starboard[msg.get_url()].id, starboard[msg.get_url()].channel_id);
      starboard.erase(msg.get_url());
      auto thread_it = bot.starboard_threads.find(msg.get_url());
      if (thread_it != bot.starboard_threads.end()) {
        bot.starboard_threads.erase(msg.get_url());
      }
    }
    return;
  }

  LOG_DEBUG("Creating embed message");
  // Create the embed message
  dpp::embed e;
  e.set_author(msg.author.username, msg.author.get_url(), msg.author.get_avatar_url());
  e.set_color(dpp::colors::yellow);
  e.set_timestamp(timestamp);

  // Get the referenced message
  const dpp::message::message_ref reference = msg.message_reference;
  dpp::message ref;
  if (!reference.message_id.empty()) {
    ref = bot.message_get_sync(reference.message_id, event.channel_id);
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
    LOG_DEBUG("Editing starboard message");
    // Edit the starboard message
    dpp::message &m = starboard[msg.get_url()];
    m.embeds.at(0) = e;
    m.set_content("⭐ **" + std::to_string(starCount) + "** | [`# " + channel.name + "`](<" + msg.get_url() + ">)");
    m = bot.message_edit_sync(m);
  } else if (starCount == 2 && std::is_same_v<EventType, dpp::message_reaction_add_t>) {
    LOG_DEBUG("Posting message to starboard channel");
    // Post in starboard channel
    const dpp::channel starboard_channel = bot.channel_get_sync(bot.get_config().at("starboardChannel").get<dpp::snowflake>());
    starboard[msg.get_url()] = bot.message_create_sync(dpp::message("⭐ **" + std::to_string(starCount) + "** | [`# " +
    channel.name + "`](<" + msg.get_url() + ">)")
    .add_embed(e)
    .set_channel_id(starboard_channel.id));

    // Remove the message from ram after 3 days ( thread magic )
    std::string url = msg.get_url();
    auto thread = std::make_shared<std::thread>([botPtr = &bot, url]() {
      LOG_DEBUG("Thread started for message removal after delay");
      std::this_thread::sleep_for(std::chrono::hours(24 * 3));
      std::unique_lock<std::mutex> lock(botPtr->starboard_mutex, std::defer_lock);
      if (!lock.try_lock()) {
        LOG_DEBUG("Thread: Starboard mutex is locked, waiting...");
        auto start = std::chrono::high_resolution_clock::now();
        lock.lock();
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
        LOG_DEBUG("Thread: Starboard mutex acquired after " + std::to_string(duration.count()) + "ms");
      }
      auto &starboard = botPtr->starboard;
      if (starboard.find(url) != starboard.end() &&
           botPtr->starboard_threads.find(url) != botPtr->starboard_threads.end()) {
        starboard.erase(url);
      }
      botPtr->starboard_threads.erase(url);
      LOG_DEBUG("Thread: Message url removed from memory");
    });

    // Store the thread and detach it
    bot.starboard_threads[msg.get_url()] = thread;
    thread->detach();
  }
}

// Explicit template instantiation
template void updateStarboardMessage<dpp::message_reaction_add_t>(custom_cluster &bot, const dpp::message_reaction_add_t &event);
template void updateStarboardMessage<dpp::message_reaction_remove_t>(custom_cluster &bot, const dpp::message_reaction_remove_t &event);