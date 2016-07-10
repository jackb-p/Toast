#include <sqlite3.h>
#include <cstdio>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

/**
/ Questions obtained from https://sourceforge.net/projects/triviadb/
/ Questions are split into 14 files - b[01-14].txt
/
/ Format:
/ [CATEGORY: ]QUESTION*ANSWER1[*ANSWER2 ...]
/ where things in [] are not always present
/
/ Hideous code, but only needs to be run one time.
**/

static int callback(void *x, int argc, char **argv, char **azColName) {
	int i;
	for (i = 0; i<argc; i++) {
		std::cout << azColName[i] << " = " << (argv[i] ? argv[i] : "NULL") << std::endl;
	}
	return 0;
}

int load_questions() {
	sqlite3 *db;
	char *zErrMsg = 0;
	int rc;

	rc = sqlite3_open("bot/db/trivia.db", &db);

	if (rc) {
		std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
		return 1;
	}
	else {
		std::cout << "Opened database successfully" << std::endl;
	}

	for (int i = 1; i <= 14; i++) {
		std::stringstream ss;
		ss.fill('0');
		ss.width(2);
		ss << i;

		std::ifstream file("data_management/questions/b" + ss.str() + ".txt");
		std::string str;
		std::string file_contents;
		while (std::getline(file, str)) {
			std::string category, question, answer;
			bool strange_question = true;

			category = "Uncategorised";
			if (str.find(':') != std::string::npos) {
				category = str.substr(0, str.find(':'));
				str = str.substr(str.find(':') + 2); // account for space after :

				strange_question = false;
			}
			question = str.substr(0, str.find('*'));
			answer = str.substr(str.find('*') + 1);

			sqlite3_stmt *insert_question;
			std::string sql = "INSERT INTO Questions (Category, Question, Answer) VALUES (?1, ?2, ?3);";
			sqlite3_prepare_v2(db, sql.c_str(), -1, &insert_question, NULL);
			sqlite3_bind_text(insert_question, 1, category.c_str(), -1, ((sqlite3_destructor_type)-1));
			sqlite3_bind_text(insert_question, 2, question.c_str(), -1, ((sqlite3_destructor_type)-1));
			sqlite3_bind_text(insert_question, 3, answer.c_str(), -1, ((sqlite3_destructor_type)-1));

			int result = sqlite3_step(insert_question);

			std::cout << (result == 101 ? "OK" : std::to_string(result)) << " ";
			
		}

		file.close();
	}

	std::cout << std::endl;

	sqlite3_close(db);

	std::getchar();
	return 0;
}