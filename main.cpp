#include "steam_api_common.h"
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

int main() {
	if (!SteamAPI_Init()) {
		return -2;
	}

	int nFriends = SteamFriends()->GetFriendCount(EFriendFlags::k_EFriendFlagImmediate);
	printf("\nAll Friends of %s: %d\n\n", SteamFriends()->GetPersonaName(), nFriends);

	// prepare player data
	for (int index = 0; index < nFriends; ++index) {
		// fetch ID of the current friend
		CSteamID friendSteamID = SteamFriends()->GetFriendByIndex(index, k_EFriendFlagImmediate);
		// fetch name
		const char *friendName = SteamFriends()->GetFriendPersonaName(friendSteamID);

		// get the friend's online status
		EPersonaState friendState = SteamFriends()->GetFriendPersonaState(friendSteamID);
		// if (friendState == EPersonaState::k_EPersonaStateOffline) {
		//  continue;
		//}

		SteamFriends()->RequestFriendRichPresence(friendSteamID); // maybe will make some diff?
		// SteamAPI_RunCallbacks();

		int nGameInfo = SteamFriends()->GetFriendRichPresenceKeyCount(friendSteamID);
		if (nGameInfo <= 0) {
			continue;
		}

		// printf("Friend #%.3d: %lld - %s - %s\n", //
		printf("Friend #%.3d: %lld [%d]: %s\n", //
			   index + 1, friendSteamID.ConvertToUint64(), friendState, friendName);

		// fetch the currently played game by the friend
		FriendGameInfo_t friendGame{};
		SteamFriends()->GetFriendGamePlayed(friendSteamID, &friendGame);
		const int friendGameID = friendGame.m_gameID.ToUint64();

		const char *display = SteamFriends()->GetFriendRichPresence(friendSteamID, "steam_display");
		printf("playing %d: \"%s\"\n", friendGameID, display);

		// fetch 'view game info' status stuff
		for (int j = 0; j < nGameInfo; ++j) {
			const char *rpcKey = SteamFriends()->GetFriendRichPresenceKeyByIndex(friendSteamID, j);
			const char *rpcValue = SteamFriends()->GetFriendRichPresence(friendSteamID, rpcKey);
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
