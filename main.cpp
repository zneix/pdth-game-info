#pragma comment(lib, "libsteam_api.so")

#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#include "steam_api.h"

// k_EPersonaStateOffline = 0,			// friend is not currently logged on
// k_EPersonaStateOnline = 1,			// friend is logged on
// k_EPersonaStateBusy = 2,				// user is on, but busy
// k_EPersonaStateAway = 3,				// auto-away feature
// k_EPersonaStateSnooze = 4,			// auto-away for a long time
// k_EPersonaStateLookingToTrade = 5,	// Online, trading
// k_EPersonaStateLookingToPlay = 6,	// Online, wanting to play
// k_EPersonaStateInvisible = 7,		// Online, but appears offline to friends.  This status is
// never published to clients.

// number of maximum possible players in a payday game
const int MAX_LOBBY_SIZE = 4;

class PaydayPlayer {
  public:
	PaydayPlayer();
	void printRpcData();

	CSteamID ID;
	const char *name;
	EPersonaState state;

	std::string lobbyID;
	int lobbySize;
	std::vector<std::pair<const char *, const char *>> rpcData;
};

PaydayPlayer::PaydayPlayer() {
	this->lobbyID = "";
	this->lobbySize = -1;
	this->rpcData = {};
}

void PaydayPlayer::printRpcData() {
	for (auto rpcPair : this->rpcData) {
		printf("    \"%s\": \"%s\"\n", rpcPair.first, rpcPair.second);
	}
}

class PaydayLobby {
  public:
	PaydayLobby();
	void addKnownPlayer(PaydayPlayer player);

	int playerCount;
	std::vector<PaydayPlayer> knownPlayers;
};

PaydayLobby::PaydayLobby() {
	this->playerCount = -1; // realistically it should be at least 1 (and at most 4) but we shall set this later anyway
	this->knownPlayers = std::vector<PaydayPlayer>();
}

void PaydayLobby::addKnownPlayer(PaydayPlayer player) {
	this->knownPlayers.push_back(player);
	this->playerCount = player.lobbySize;
}

int main() {
	if (!SteamAPI_Init()) {
		return -2;
	}

	int nFriends = SteamFriends()->GetFriendCount(EFriendFlags::k_EFriendFlagImmediate);
	printf("All Friends of %s: %d\n\n", SteamFriends()->GetPersonaName(), nFriends);

	std::unordered_map<std::string, PaydayLobby> lobbies = {};

	// prepare player data
	for (int index = 0; index < nFriends; ++index) {
		// use objects for our newly parsed friend
		PaydayPlayer player;

		player.ID = SteamFriends()->GetFriendByIndex(index, k_EFriendFlagImmediate);
		player.name = SteamFriends()->GetFriendPersonaName(player.ID);

		// get the friend's online status
		player.state = SteamFriends()->GetFriendPersonaState(player.ID);
		// if (friendState == EPersonaState::k_EPersonaStateOffline) {
		//  continue;
		//}

		SteamFriends()->RequestFriendRichPresence(player.ID); // maybe will make some diff?
		// SteamAPI_RunCallbacks();

		// don't process data from a player who doesn't have presence data available - this usually means they aren't playing payday at the moment
		int nGameInfo = SteamFriends()->GetFriendRichPresenceKeyCount(player.ID);
		if (nGameInfo <= 0) {
			continue;
		}

		// TODO: consider using universal friend index (I made this name up) a.k.a. 'index + 1' in PaydayPlayer object

		// fetch the currently played game by the friend
		FriendGameInfo_t friendGame{};
		SteamFriends()->GetFriendGamePlayed(player.ID, &friendGame);
		const int friendGameID = friendGame.m_gameID.ToUint64();

		const char *display = SteamFriends()->GetFriendRichPresence(player.ID, "steam_display");

		// fetch 'view game info' status information
		for (int j = 0; j < nGameInfo; ++j) {
			const char *rpcKey = SteamFriends()->GetFriendRichPresenceKeyByIndex(player.ID, j);
			const char *rpcValue = SteamFriends()->GetFriendRichPresence(player.ID, rpcKey);

			if (std::strcmp(rpcKey, "steam_player_group") == 0) {
				// set player's lobby ID
				player.lobbyID = rpcValue;
			} else if (std::strcmp(rpcKey, "steam_player_group_size") == 0) {
				// set player's lobby size
				player.lobbySize = std::stoi(rpcValue);
			} else {
				player.rpcData.emplace_back(std::pair{rpcKey, rpcValue});
			}
		}

		// when player somehow isn't in a lobby, do not initialize lobby data and instead print what we salvaged
		if (player.lobbyID.empty()) {
			printf("[not-in-lobby] %s | encountered %lu rpc pairs:\n", player.name, player.rpcData.size());
			player.printRpcData();

			printf("\n");
			continue;
		}

		// lobby hasn't been encountered yet, initialize it in the list of found lobbies
		if (!lobbies.contains(player.lobbyID)) {
			lobbies.insert({player.lobbyID, {}});
		}
		// get the lobby and add the player to it
		lobbies[player.lobbyID].addKnownPlayer(player);
	}

	// print lobby data
	for (auto lobby : lobbies) {
		printf("lobby %s\nplayers %d / %d\n", lobby.first.c_str(), lobby.second.playerCount, MAX_LOBBY_SIZE);
		// go through each player in the lobby
		for (auto lobbyPlayer : lobby.second.knownPlayers) {
			printf("  [%d] %lld %s\n", lobbyPlayer.state, lobbyPlayer.ID.ConvertToUint64(), lobbyPlayer.name);
			lobbyPlayer.printRpcData();
		}
		printf("\n");
	}

	// set your own status
	// SteamFriends()->SetRichPresence("steam_display", "danking around");
	// while (true) {
	// std::this_thread::sleep_for(std::chrono::milliseconds(100));
	// SteamAPI_RunCallbacks();
	//}
	SteamAPI_Shutdown();

	return 0;
}
