#pragma comment(lib, "libsteam_api.so")

#include <cstdio>
#include <map>
#include <string>
#include <thread>
#include <vector>

#include "steam_api.h"

// k_EPersonaStateOffline = 0,			// friend is not currently logged on
// k_EPersonaStateOnline = 1,			// friend is logged on
// k_EPersonaStateBusy = 2,			// user is on, but busy
// k_EPersonaStateAway = 3,			// auto-away feature
// k_EPersonaStateSnooze = 4,			// auto-away for a long time
// k_EPersonaStateLookingToTrade = 5,	// Online, trading
// k_EPersonaStateLookingToPlay = 6,	// Online, wanting to play
// k_EPersonaStateInvisible = 7,		// Online, but appears offline to friends.  This status is
// never published to clients.

class PaydayPlayer {
  public:
	PaydayPlayer(CSteamID ID, const std::string &name, EPersonaState state,
				 const std::string &lobbyID,
				 const std::vector<std::pair<std::string, std::string>> &rpcData);
	PaydayPlayer();

	CSteamID ID;
	std::string name;
	EPersonaState state;

	std::string lobbyID;
	std::vector<std::pair<std::string, std::string>> rpcData;
};

PaydayPlayer::PaydayPlayer(CSteamID ID, const std::string &name, EPersonaState state,
						   const std::string &lobbyID,
						   const std::vector<std::pair<std::string, std::string>> &rpcData)
	: ID(ID), name(name), state(state), lobbyID(lobbyID), rpcData(rpcData) {
	//
}

PaydayPlayer::PaydayPlayer() {
	//
}

int main() {
	if (!SteamAPI_Init()) {
		return -2;
	}

	int nFriends = SteamFriends()->GetFriendCount(EFriendFlags::k_EFriendFlagImmediate);
	printf("\nAll Friends of %s: %d\n\n", SteamFriends()->GetPersonaName(), nFriends);

	// std::map<std::string, PaydayPlayer> lobbies = {};
	// return 0;

	// prepare player data
	for (int index = 0; index < nFriends; ++index) {
		// use objects for our newly parsed friend
		PaydayPlayer player;

		player.ID = SteamFriends()->GetFriendByIndex(index, k_EFriendFlagImmediate);
		player.name = SteamFriends()->GetFriendPersonaName(player.ID);

		// get the friend's online status
		EPersonaState friendState = SteamFriends()->GetFriendPersonaState(player.ID);
		// if (friendState == EPersonaState::k_EPersonaStateOffline) {
		//  continue;
		//}

		SteamFriends()->RequestFriendRichPresence(player.ID); // maybe will make some diff?
		// SteamAPI_RunCallbacks();

		int nGameInfo = SteamFriends()->GetFriendRichPresenceKeyCount(player.ID);
		if (nGameInfo <= 0) {
			continue;
		}

		// printf("Friend #%.3d: %lld - %s - %s\n", //
		printf("Friend #%.3d: %lld [%d]: %s\n", //
			   index + 1, player.ID.ConvertToUint64(), friendState, player.name.c_str());

		// fetch the currently played game by the friend
		FriendGameInfo_t friendGame{};
		SteamFriends()->GetFriendGamePlayed(player.ID, &friendGame);
		const int friendGameID = friendGame.m_gameID.ToUint64();

		const char *display = SteamFriends()->GetFriendRichPresence(player.ID, "steam_display");
		printf("playing %d: \"%s\"\n", friendGameID, display);

		// fetch 'view game info' status stuff
		for (int j = 0; j < nGameInfo; ++j) {
			const char *rpcKey = SteamFriends()->GetFriendRichPresenceKeyByIndex(player.ID, j);
			const char *rpcValue = SteamFriends()->GetFriendRichPresence(player.ID, rpcKey);
			printf("\"%s\": \"%s\"\n", rpcKey, rpcValue);
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
