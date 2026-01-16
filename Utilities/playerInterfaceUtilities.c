/**
 * Implements player interface utility functions for managing player interactions
 * with the game level, such as planet selection and sending orders.
 * @author abmize
 * @file playerInterfaceUtilities.c
 */

#include "Utilities/playerInterfaceUtilities.h"

/**
 * Resets the player's selection state to accommodate a new number of planets.
 * Typically called when the level is (re)initialized.
 * Allocates or reallocates the selection buffer as needed.
 * @param state Pointer to the PlayerSelectionState to reset.
 * @param planetCount The number of planets in the level.
 * @return true if the reset was successful, false otherwise.
 */
bool PlayerSelectionReset(PlayerSelectionState *state, size_t planetCount) {
    // Basic validation of input pointer.
    if (state == NULL) {
        return false;
    }

    // Free any existing selection buffer.
    PlayerSelectionFree(state);

    // If planetCount is zero, simply set fields to zero and return,
    // as there are no planets to select and thus no buffer is needed.
    if (planetCount == 0) {
        state->selectedPlanets = NULL;
        state->capacity = 0;
        state->count = 0;
        return true;
    }
   
    // Otherwise, allocate a new selection buffer, appropriately sized
    // to be able to hold up to planetCount selections.
    state->selectedPlanets = (bool *)calloc(planetCount, sizeof(bool));
    if (state->selectedPlanets == NULL) {
        state->capacity = 0;
        state->count = 0;
        printf("Failed to allocate selection buffer.\n");
        return false;
    }

    // Mark the selection state as having a capacity for planetCount planets,
    // and zero currently selected so code elsewhere can track selection count.
    state->capacity = planetCount;
    state->count = 0;
    return true;
}

/**
 * Clears the player's current selection state,
 * deselecting all currently selected planets.
 * @param state Pointer to the PlayerSelectionState to clear.
 */
void PlayerSelectionClear(PlayerSelectionState *state) {
    // Basic validation of input pointer.
    if (state == NULL) {
        return;
    }

    // If there is no selection buffer, nothing to clear.
    if (state->selectedPlanets == NULL || state->capacity == 0) {
        state->count = 0;
        return;
    }

    // Otherwise, clear all selections by simply setting all booleans to false (0).
    memset(state->selectedPlanets, 0, state->capacity * sizeof(bool));

    // Reset the count of selected planets to zero.
    state->count = 0;
}

/**
 * Toggles the selection state of a planet at the given index.
 * If additive is false, clears the current selection before toggling.
 * @param state Pointer to the PlayerSelectionState to modify.
 * @param index The index of the planet to toggle.
 * @param additive If true, adds/removes from the current selection;
 *                 if false, replaces the selection with just this planet.
 * @return true if the toggle was successful, false otherwise.
 */
bool PlayerSelectionToggle(PlayerSelectionState *state, size_t index, bool additive) {
    // Basic validation of input parameters.
    if (state == NULL || state->selectedPlanets == NULL || index >= state->capacity) {
        return false;
    }

    // If we're not adding to the current selection,
    // we're making a new one and throwing out the old selection.
    if (!additive) {
        // In the special case where the planet is already the only selected one,
        // we can simply clear the selection and return early.
        if (state->selectedPlanets[index] && state->count == 1) {
            PlayerSelectionClear(state);
            return true;
        }
        PlayerSelectionClear(state);
    }

    // Toggle the selection state of the specified planet,
    // and update the count of selected planets accordingly.
    if (state->selectedPlanets[index]) {
        state->selectedPlanets[index] = false;
        if (state->count > 0) {
            state->count -= 1;
        }
    } else {
        state->selectedPlanets[index] = true;
        state->count += 1;
    }

    return true;
}

/**
 * Frees the resources associated with the player's selection state.
 * @param state Pointer to the PlayerSelectionState to free.
 */
void PlayerSelectionFree(PlayerSelectionState *state) {
    // Basic validation of input pointer.
    if (state == NULL) {
        return;
    }

    // Free the selection buffer if it exists,
    // and reset all fields to zero/null.
    free(state->selectedPlanets);
    state->selectedPlanets = NULL;
    state->capacity = 0;
    state->count = 0;
}

/**
 * Sends a move order to the server for the selected planets
 * to move fleets to the specified destination planet.
 * @param state Pointer to the PlayerSelectionState containing selected planets.
 * @param socket The UDP socket used to send the order.
 * @param serverAddress Pointer to the server's address information.
 * @param level Pointer to the current Level structure.
 * @param destinationIndex The index of the destination planet.
 * @return true if the move order was sent successfully, false otherwise.
 */
bool PlayerSendMoveOrder(const PlayerSelectionState *state,
    SOCKET socket,
    const SOCKADDR_IN *serverAddress,
    const Level *level,
    size_t destinationIndex) {

    // Basic validation of input parameters.
    if (state == NULL || level == NULL || serverAddress == NULL) {
        return false;
    }

    // Validate socket, selection count, and destination index.
    if (socket == INVALID_SOCKET || state->count == 0 || destinationIndex >= level->planetCount) {
        return false;
    }

    // Ensure there are selected planets to send a move order for.
    if (state->selectedPlanets == NULL || state->capacity == 0) {
        return false;
    }

    // We construct the move order packet dynamically
    // based on the number of selected origin planets.
    size_t originCount = state->count;
    size_t packetSize = sizeof(LevelMoveOrderPacket) + originCount * sizeof(int32_t);
    LevelMoveOrderPacket *packet = (LevelMoveOrderPacket *)malloc(packetSize);
    if (packet == NULL) {
        printf("Failed to allocate move order packet.\n");
        return false;
    }

    // Set up the header and fill in the origin planet indices.
    packet->type = LEVEL_PACKET_TYPE_MOVE_ORDER;
    packet->destinationPlanetIndex = (int32_t)destinationIndex;

    // Populate the origin planet indices from the selection state.
    int32_t *indices = packet->originPlanetIndices;

    // We iterate through the selection state and collect the indices
    // of the selected planets into the packet, also keeping track of how many we write
    // so we can set the originCount field correctly.
    size_t writeIndex = 0;
    for (size_t i = 0; i < state->capacity; ++i) {
        if (state->selectedPlanets[i]) {
            // if we somehow exceed the expected origin count,
            // we abort to avoid buffer overflows.
            if (writeIndex >= originCount) {
                free(packet);
                printf("Selection state mismatch detected.\n");
                return false;
            }
            indices[writeIndex++] = (int32_t)i;
        }
    }

    // Set the actual origin count in the packet header.
    packet->originCount = (uint32_t)writeIndex;

    // Actual packet size may be smaller than initially calculated
    // if there were discrepancies in selection state,
    // so we compute it based on how many origins we actually wrote.
    // That said, in practice writeIndex should always equal originCount here.
    size_t actualPacketSize = sizeof(LevelMoveOrderPacket) + writeIndex * sizeof(int32_t);

    // Send the packet to the server.
    int result = sendto(socket,
        (const char *)packet,
        (int)actualPacketSize,
        0,
        (const struct sockaddr *)serverAddress,
        (int)sizeof(*serverAddress));

    if (result == SOCKET_ERROR) {
        printf("sendto failed: %d\n", WSAGetLastError());
        free(packet);
        return false;
    }

    // We're done, and all that's left is to free and return.
    free(packet);
    return true;
}
