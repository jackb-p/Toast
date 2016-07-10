#ifndef BOT_QUESTIONHELPER
#define BOT_QUESTIONHELPER

#include <iostream>
#include <map>
#include <string>
#include <set>

#include <sqlite3.h>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

class GatewayHandler;
class APIHelper;
namespace DiscordObjects {
	class User;
}


class TriviaGame {
public:
	TriviaGame(GatewayHandler *gh, APIHelper *ah, std::string channel_id, int total_questions);
	~TriviaGame();

	void start();
	void handle_answer(std::string answer, DiscordObjects::User sender);

private:
	int questions_asked;
	int total_questions;

	void end_game();
	void question();
	void give_hint(int hints_given, std::string hint);
	void question_failed();
	void increase_score(std::string user_id);
	void update_average_time(std::string user_id, int time);

	std::string channel_id;
	GatewayHandler *gh;
	APIHelper *ah;

	const char hide_char = '#';

	std::string current_question;
	std::set<std::string> current_answers;

	// <user_id, score>
	std::map<std::string, int> scores;
	// <user_id, average_time>
	std::map<std::string, int> average_times;

	boost::shared_ptr<boost::thread> current_thread;

	boost::posix_time::ptime question_start;
};

#endif