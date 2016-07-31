#trivia-bot <img src="https://cdn.discordapp.com/attachments/164732409919569920/205700949304541184/emoji.png" width="30" height="30" /> <img src="https://travis-ci.org/jackb-p/trivia-bot.svg?branch=release" />

A bot which provides a Trivia game for [Discord](https://discordapp.com/).

### Adding to a server
You can either self-host trivia-bot, or use the hosted version.

To invite the bot to your server use [this invite link](https://discordapp.com/oauth2/authorize?client_id=199657080083316737&scope=bot).
It requires no special permissions at this time (only read/write to channels).


### Installation
[Releases](https://github.com/jackb-p/trivia-bot/releases) are available for tagged versions. These are compiled on Ubuntu by Travis CI. To run on other systems you will need to compile yourself - a CMake configuration is included. Windows releases will happen one day.

If you want to install a version for which a release does not exist, you will also have to compile manually. Compilation instructions are available for Linux below.


### Running
To run simply execute the program: `./TriviaBot`

If you do not want to be prompted for your token every launch, provide it as an argument: `./TriviaBot {TOKEN}`

### Trivia Questions
Questions are obtained from [trivia-db on Sourceforge](https://sourceforge.net/projects/triviadb/).

To parse the `.txt` question files, place them in `/data_management/questions/` then run `LoadDB.cpp`. 
You need to create the database first. Use the included schema and create the database as `/bot/db/trivia.db`.

LoadDB.cpp takes some time to execute.


### Commands
`` `trivia`` is the base command.

| Argument                | Description                                                                                                                                                                |
| ----------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `questions` `interval`  | Where `questions` and `interval` are integers. Makes the game last `questions` number of questions, optionally sets the time interval between hints to `interval` seconds. | 
| stop                    | Stops the trivia game currently in the channel the message is sent from, if there is one.                                                                                  |
| help                    | Prints a help list, similar to this table.                                                                                                                                 |


### Compiling
#### Dependencies
| Name          | Website                                                       | Notes                                                                                            |
| ------------- | ------------------------------------------------------------- | ------------------------------------------------------------------------------------------------ |
| boost         | [boost.org](http://www.boost.org/)                            |                                                                                                  |
| websocketpp   | [zaphoyd/websocketpp](https://github.com/zaphoyd/websocketpp) | Included as submodule.                                                                           |
| cURL          | [curl.haxx.se](https://curl.haxx.se/)                         |                                                                                                  |
| sqlite3       | [sqlite.org](https://www.sqlite.org/)                         | Included as submodule. Uses a [different repo](https://github.com/azadkuh/sqlite-amalgamation/). |
| nlohmann/json | [nlohmann/json](https://github.com/nlohmann/json)             | (Slightly modified) source file included in repo.                                                |

#### Linux (debian)
c++14 support is required. gcc 5 and above recommended.

1. Clone the github repo: `git clone https://github.com/jackb-p/TriviaDiscord.git TriviaDiscord`
2. Navigate to repository directory: `cd TriviaDiscord`
3. Clone the submodules: `git submodule init` and `git submodule update`
4. Install other dependencies: `sudo apt-get install cmake libboost-all-dev libcurl-dev` (Package managers and names may vary, but all of these should be easy to find through a simple Google search.)
5. `cd TriviaBot`
6. `cmake .`
7. `make`
