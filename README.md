# Toast <img src="https://www.ahealthiermichigan.org/wp-content/uploads/2014/09/Transform-toast-into-breakfast.jpg" width="30" height="30" />

A bot which provides a Trivia game for [Discord](https://discordapp.com/).

### Adding to a server
You can either self-host trivia-bot, or use the hosted version.

To invite the bot to your server use [this invite link](https://discordapp.com/oauth2/authorize?client_id=199657080083316737&scope=bot).
It requires no special permissions at this time (only read/write to channels).


### Installation
[Releases](https://github.com/jackb-p/trivia-bot/releases) are available for tagged versions. These are compiled on Debian Jessie by Jenkins. Note that you still require the dependency library files, so you will still have to build V8 and add it to your `LD_LIBRARY_PATH`. To run on other systems you will need to compile yourself - a CMake configuration is included. Windows releases will happen one day.

If you want to install a version for which a release does not exist, you will also have to compile manually. Compilation instructions are available for Linux below.


### Running
To run simply execute the program: `./Toast`

#### Configuration
The config file is automatically generated if it is not present. The JSON format is used. You must edit the config file for the bot to work correctly, the bot token is required.

The current configuration options are as follows:

1. **General**

| Field | Description |
| --- | --- |
| `api_cert_file` | The path to the Discord API .crt file for HTTPS. |
| `bot_token` | Your Discord bot token. |
| `owner_id` | The user ID of the owner of the bot. This allows owner-only (maintenance) commands, such as `shutdown`. |
| `js_allowed_roles` | List of role names which are allowed to use the `createjs` ands `js` commands. |

### Trivia Questions
Questions are obtained from [trivia-db on Sourceforge](https://sourceforge.net/projects/triviadb/).

To parse the `.txt` question files, place them in `/data_management/questions/` then run `LoadDB.cpp`. 
You need to create the database first. Use the included schema and create the database as `/bot/db/trivia.db`.

LoadDB.cpp takes some time to execute.


### Commands
#### Trivia Game
`` `trivia`` is the base command.

| Argument | Description |
| --- | --- |
| `questions` `interval` | Where `questions` and `interval` are integers. Makes the game last `questions` number of questions, optionally sets the time interval between hints to `interval` seconds. | 
| stop | Stops the trivia game currently in the channel the message is sent from, if there is one. |
| help | Prints a help list, similar to this table. |

#### Javascript Commands
The Javascript system is designed to mirror the old [Boobot implementation](https://www.boobot.party/). For now there are some exceptions:

1. Message objects aren't implemented.
2. Properties *are* case sensitive. You must use `server.Name`, not `server.name`. This will not be changed.

### Compiling
#### Dependencies
| Name | Website | Notes |
| --- | --- | --- |
| boost | [boost.org](http://www.boost.org/) | |
| websocketpp | [zaphoyd/websocketpp](https://github.com/zaphoyd/websocketpp) | Included as submodule. |
| cURL | [curl.haxx.se](https://curl.haxx.se/) | |
| sqlite3 | [sqlite.org](https://www.sqlite.org/) | Included as submodule. Uses a [different repo](https://github.com/azadkuh/sqlite-amalgamation/). |
| nlohmann/json | [nlohmann/json](https://github.com/nlohmann/json) | (Slightly modified) source file included in repo. |
| V8 | [Google V8](https://developers.google.com/v8/) | Debian/Ubuntu `libv8` packages are too outdated. Must be built manually. |

#### Linux (Debian)
c++14 support is required. gcc 5 and above recommended, however it compiles on 4.9.2 (and possibly some versions below.)

1. Clone the github repo: `git clone https://github.com/jackb-p/Toast.git ToastBot`
2. Navigate to repository directory: `cd ToastBot`
3. Clone the submodules: `git submodule init` and `git submodule update`
4. Install other dependencies: `sudo apt-get install build-essential cmake libboost-all-dev libcurl4-openssl-dev libssl-dev` (Package managers and names may vary, but all of these should be easy to find through a simple Google search.) V8 may require other dependencies.
5. Build V8. Put the library files into lib/v8/lib/ and the include files into lib/v8/include. More instructions will be added at some point for this step.
6. `cd Toast`
7. `cmake .`
8. `make`
