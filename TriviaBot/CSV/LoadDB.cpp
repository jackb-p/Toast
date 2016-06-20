#include <stdio.h>
#include <sqlite3.h>
#include <cstdio>
#include <string>
#include <iostream>
#include "csv.h"

/**
/ Takes the questions stored in the CSV downloaded from https://www.reddit.com/r/trivia/comments/3wzpvt/free_database_of_50000_trivia_questions/
/ Questions are stored in a weird format, which makes it a lot harder. To make it easier to process them, I replaced any double commas with single commas,
/	and renamed the repeated headers to Category1, Category2, Question1, etc... There was also one question with incorrect escaping which was fixed manually.

/ Hideous code, but only needs to be run one time.
**/

static int callback(void *x, int argc, char **argv, char **azColName) {
	int i;
	for (i = 0; i<argc; i++) {
		std::cout << azColName[i] << " = " << (argv[i] ? argv[i] : "NULL") << std::endl;
	}
	return 0;
}

int main(int argc, char* argv[]) {
	sqlite3 *db;
	char *zErrMsg = 0;
	int rc;

	rc = sqlite3_open("../trivia.db", &db);

	if (rc) {
		std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
		return(0);
	}
	else {
		std::cout << "Opened database successfully" << std::endl;
	}

	io::CSVReader<9, io::trim_chars<' ', '\t'>, io::double_quote_escape<',', '"'>> in("trivia.csv");
	in.read_header(io::ignore_extra_column, "Category", "Question", "Answer", "Category1", "Question1", "Answer1", "Category2", "Question2", "Answer2");
	std::string c0, q0, a0, c1, q1, a1, c2, q2, a2;
	int row = 0;
	sqlite3_stmt *insertThreeQuestions;
	std::string sql;
	while (in.read_row(c0, q0, a0, c1, q1, a1, c2, q2, a2)) {
		// Process three at a time because why not?
		sql = "INSERT INTO Questions (Category, Question, Answer) VALUES (?1, ?2, ?3), (?4, ?5, ?6), (?7, ?8, ?9);";
		sqlite3_prepare_v2(db, sql.c_str(), -1, &insertThreeQuestions, NULL);
		sqlite3_bind_text(insertThreeQuestions, 1, c0.c_str(), -1, ((sqlite3_destructor_type)-1));
		sqlite3_bind_text(insertThreeQuestions, 2, q0.c_str(), -1, ((sqlite3_destructor_type)-1));
		sqlite3_bind_text(insertThreeQuestions, 3, a0.c_str(), -1, ((sqlite3_destructor_type)-1));
		sqlite3_bind_text(insertThreeQuestions, 4, c1.c_str(), -1, ((sqlite3_destructor_type)-1));
		sqlite3_bind_text(insertThreeQuestions, 5, q1.c_str(), -1, ((sqlite3_destructor_type)-1));
		sqlite3_bind_text(insertThreeQuestions, 6, a1.c_str(), -1, ((sqlite3_destructor_type)-1));
		sqlite3_bind_text(insertThreeQuestions, 7, c2.c_str(), -1, ((sqlite3_destructor_type)-1));
		sqlite3_bind_text(insertThreeQuestions, 8, q2.c_str(), -1, ((sqlite3_destructor_type)-1));
		sqlite3_bind_text(insertThreeQuestions, 9, a2.c_str(), -1, ((sqlite3_destructor_type)-1));

		int result = sqlite3_step(insertThreeQuestions);
		std::cout << result << " ";

	}
	std::cout << std::endl;

	sqlite3_close(db);

	std::getchar();
	return 0;
}