#include "TriviaGame.hpp"

#include <cstdio>
#include <sstream>
#include <random>

#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/regex.hpp>

#include "GatewayHandler.hpp"
#include "APIHelper.hpp"
#include "data_structures/User.hpp"

TriviaGame::TriviaGame(GatewayHandler *gh, APIHelper *ah, std::string channel_id, int total_questions) {
	this->gh = gh;
	this->ah = ah;
	this->channel_id = channel_id;

	this->total_questions = total_questions;
	questions_asked = 0;
}

TriviaGame::~TriviaGame() {
	std::string message = ":red_circle: **(" + std::to_string(questions_asked) + "/" + std::to_string(total_questions) + 
		")** Game over! **Scores:**\n";
	
	// convert map to pair vector
	std::vector<std::pair<std::string, int>> pairs;
	for (auto &s : scores) {
		pairs.push_back(s);
	}

	// sort by score, highest->lowest
	std::sort(pairs.begin(), pairs.end(), [=](std::pair<std::string, int>& a, std::pair<std::string, int>& b) {
		return a.second > b.second;
	});

	for (auto &p : pairs) {
		std::string average_time;
		average_time = std::to_string(average_times[p.first] / 1000.0f);
		average_time.pop_back(); average_time.pop_back(); average_time.pop_back();
		message += ":small_blue_diamond: <@!" + p.first + ">: " + std::to_string(p.second) + " (Avg: " + average_time + " seconds)\n";
	}
	ah->send_message(channel_id, message);

	sqlite3 *db; int rc; std::string sql;

	rc = sqlite3_open("bot/db/trivia.db", &db);
	if (rc) {
		std::cerr << "Cant't open database: " << sqlite3_errmsg(db) << std::endl;
	}

	std::string sql_in_list;
	for (int i = 1; i <= scores.size(); i++) {
		sql_in_list += "?,";
	}
	sql_in_list.pop_back(); // remove final comma

	// prepare statement
	sql = "SELECT * FROM TotalScores WHERE User IN(" + sql_in_list + ");";
	sqlite3_stmt *stmt;

	rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
	if (rc != SQLITE_OK) {
		std::cerr << "SQL error." << std::endl;
	}

	// insert arguments
	for (int i = 0; i < scores.size(); i++) {
		rc = sqlite3_bind_text(stmt, i + 1, pairs[i].first.c_str(), -1, (sqlite3_destructor_type) -1);

		if (rc != SQLITE_OK) {
			std::cerr << "SQL error." << std::endl;
			break;
		}
	}
	
	std::map<std::string, std::pair<int, int>> data;
	rc = 0; // just in case
	while (rc != SQLITE_DONE) {
		rc = sqlite3_step(stmt);

		if (rc == SQLITE_ROW) {
			std::string id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
			int total_score = sqlite3_column_int(stmt, 1);
			int average_time = sqlite3_column_int(stmt, 2);

			data[id] = std::pair<int, int>(total_score, average_time);
		} else if (rc != SQLITE_DONE) {
			sqlite3_finalize(stmt);
			std::cerr << "SQLite error." << std::endl;
			break;
		}
	}
	sqlite3_finalize(stmt);

	std::string update_sql;
	if (data.size() < scores.size()) { // some users dont have entries yet
		std::string sql = "INSERT INTO TotalScores (User, TotalScore, AverageTime) VALUES ";
		for (auto &i : scores) {
			if (data.find(i.first) == data.end()) {
				sql += "(?, ?, ?),";
			}
		}
		sql.pop_back(); // remove final comma
		sql += ";";

		rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
		if (rc != SQLITE_OK) {
			std::cerr << "SQL error." << std::endl;
		}

		int count = 1;
		for (auto &i : scores) {
			if (data.find(i.first) == data.end()) {
				sqlite3_bind_text(stmt, count, i.first.c_str(), -1, (sqlite3_destructor_type) -1);
				sqlite3_bind_int(stmt, count + 1, scores[i.first]);
				sqlite3_bind_int(stmt, count + 2, average_times[i.first]);
				count += 3;
			}
		}
		
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}
		
	for (int i = 0; i < data.size(); i++) {
		update_sql += "UPDATE TotalScores SET TotalScore=?, AverageTime=? WHERE User=?;";
	}

	if (update_sql != "") {
		std::cout << update_sql << std::endl;
		rc = sqlite3_prepare_v2(db, update_sql.c_str(), -1, &stmt, 0);
		if (rc != SQLITE_OK) {
			std::cerr << "SQL error." << std::endl;
		}

		int index = 1;
		for (auto &i : data) {
			int total_answered = i.second.first + scores[i.first];
			// (correct answers [t] * average time [t]) + (correct answers [c] * average time [c]) -- [c] is current game, [t] total
			long total = (i.second.first * i.second.second) + (scores[i.first] * average_times[i.first]);
			// total / correct answers [t+c] = new avg
			int new_avg = total / total_answered;

			rc = sqlite3_bind_int(stmt, index, total_answered);
			rc = sqlite3_bind_int(stmt, index + 1, new_avg);
			rc = sqlite3_bind_text(stmt, index + 2, i.first.c_str(), -1, (sqlite3_destructor_type) -1);

			index += 3;
		}

		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}

	sqlite3_close(db);
}

void TriviaGame::end_game() {
	gh->delete_game(channel_id);
}

void TriviaGame::start() {
	question();
}

void TriviaGame::question() {
	sqlite3 *db; int rc; char *sql;

	/// open db
	rc = sqlite3_open("bot/db/trivia.db", &db);
	if (rc) {
		std::cerr << "Cant't open database: " << sqlite3_errmsg(db) << std::endl;
	}

	// prepare statement
	sqlite3_stmt *stmt;
	sql = "SELECT * FROM Questions ORDER BY RANDOM() LIMIT 1;";
	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

	if (rc != SQLITE_OK) {
		std::cerr << "SQL error." << std::endl;
	}

	rc = sqlite3_step(stmt);
	if (rc == SQLITE_ROW) {
		// result received
		std::string id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)); // converts id to string for us
		std::string category = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
		std::string question = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
		std::string answer = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

		current_question = "#" + id + " [" + category + "] **" + question + "**";
		boost::split(current_answers, boost::algorithm::to_lower_copy(answer), boost::is_any_of("*"));
		
	} else if (rc != SQLITE_DONE) {
		sqlite3_finalize(stmt);
		std::cerr << "SQLite error." << std::endl;
	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);

	questions_asked++;
	ah->send_message(channel_id, ":question: **(" + std::to_string(questions_asked) + "/" + std::to_string(total_questions) + ")** " + current_question);
	question_start = boost::posix_time::microsec_clock::universal_time();

	current_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&TriviaGame::give_hint, this, 0, "")));
}

void TriviaGame::give_hint(int hints_given, std::string hint) {
	boost::this_thread::sleep(boost::posix_time::seconds(10));
	
	std::string answer = *current_answers.begin();

	bool print = false;

	if (hints_given == 0) {
		hint = answer;
		// probably shouldn't use regex here
		boost::regex regexp("[a-zA-Z0-9]+?");
		hint = boost::regex_replace(hint, regexp, std::string(1, hide_char));

		print = true;
	}
	else {
		std::stringstream hint_stream(hint);

		std::random_device rd;
		std::mt19937 rng(rd());

		std::vector<std::string> hint_words, answer_words;
		boost::split(hint_words, hint, boost::is_any_of(" "));
		boost::split(answer_words, answer, boost::is_any_of(" "));

		hint = "";
		for (int i = 0; i < hint_words.size(); i++) {
			std::string word = hint_words[i];

			// count number of *s
			int length = 0;
			for (int i = 0; i < word.length(); i++) {
				if (word[i] == hide_char) {
					length++;
				}
			}

			if (length > 1) {
				std::uniform_int_distribution<int> uni(0, word.length() - 1);

				bool replaced = false;
				while (!replaced) {
					int replace_index = uni(rng);
					if (word[replace_index] == hide_char) {
						word[replace_index] = answer_words[i][replace_index];

						print = true;
						replaced = true;
					}
				}
			}

			hint += word + " ";
		}
	}

	hints_given++; // now equal to the amount of [hide_char]s that need to be present in each word
	
	if (print) {
		ah->send_message(channel_id, ":small_orange_diamond: Hint: **`" + hint + "`**");
	}

	if (hints_given < 4) {
		current_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&TriviaGame::give_hint, this, hints_given, hint)));
	} else {
		current_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&TriviaGame::question_failed, this)));
	}
}

void TriviaGame::question_failed() {
	boost::this_thread::sleep(boost::posix_time::seconds(10));
	ah->send_message(channel_id, ":exclamation: Question failed. Answer: ** `" + *current_answers.begin() + "` **");

	if (questions_asked < 10) {
		question();
	} else {
		end_game();
	}
}

void TriviaGame::handle_answer(std::string answer, DiscordObjects::User sender) {
	boost::algorithm::to_lower(answer);
	if (current_answers.find(answer) != current_answers.end()) {
		current_thread->interrupt();

		boost::posix_time::time_duration diff = boost::posix_time::microsec_clock::universal_time() - question_start;

		std::string time_taken = std::to_string(diff.total_milliseconds() / 1000.0f);
		// remove the last three 0s
		time_taken.pop_back(); time_taken.pop_back(); time_taken.pop_back();

		ah->send_message(channel_id, ":heavy_check_mark: <@!" + sender.id + "> You got it! (" + time_taken + " seconds)");

		increase_score(sender.id);
		update_average_time(sender.id, diff.total_milliseconds());

		if (questions_asked < 10) {
			question();
		} else {
			end_game();
		}
	} else if (answer == "`s" || answer == "`stop") {
		current_thread->interrupt();

		end_game();
	}
}

void TriviaGame::increase_score(std::string user_id) {
	if (scores.find(user_id) == scores.end()) { 
		// user entry not found, add one
		scores[user_id] = 1;
	}
	else {
		scores[user_id]++;
	}
}

void TriviaGame::update_average_time(std::string user_id, int time) {
	if (average_times.find(user_id) == average_times.end()) {
		// user entry not found, add one
		average_times[user_id] = time;
	}
	else {
		int questions_answered = scores[user_id];
		// questions_answered was updated before this, -1 to get to previous value for avg calc
		int total = average_times[user_id] * (questions_answered - 1);
		total += time;

		// yeah it probably loses accuracy here, doesn't really matter
		average_times[user_id] = (int) (total / questions_answered);
	}
}