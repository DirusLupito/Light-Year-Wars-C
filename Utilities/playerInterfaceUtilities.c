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
 * Sets the selection state of the specified planet.
 * @param state Pointer to the PlayerSelectionState to modify.
 * @param index Index of the planet.
 * @param selected true to select, false to deselect.
 * @return true if the operation succeeded, false otherwise.
 */
bool PlayerSelectionSet(PlayerSelectionState *state, size_t index, bool selected) {
    // Basic validation of input parameters.
    if (state == NULL || state->selectedPlanets == NULL || index >= state->capacity) {
        return false;
    }

    // If the planet is already in the desired state, nothing to do.
    bool current = state->selectedPlanets[index];
    if (current == selected) {
        return true;
    }

    // Otherwise, set the selection state and 
    // increment/decrement the count of selected planets
    // accordingly.
    state->selectedPlanets[index] = selected;
    if (selected) {
        state->count += 1;
    } else if (state->count > 0) {
        state->count -= 1;
    }
    return true;
}

/**
 * Selects all planets owned by the specified faction.
 * @param state Pointer to the PlayerSelectionState to update.
 * @param level Pointer to the current Level structure.
 * @param owner Pointer to the owning faction.
 * @param additive When true the owned planets are added to the existing selection.
 * @return true if at least one owned planet was processed, false otherwise.
 */
bool PlayerSelectionSelectOwned(PlayerSelectionState *state,
    const Level *level,
    const Faction *owner,
    bool additive) {

    // Basic validation of input parameters.
    if (state == NULL || level == NULL || owner == NULL) {
        return false;
    }

    // If there is no selection buffer, nothing can be selected.
    if (state->selectedPlanets == NULL || state->capacity == 0) {
        return false;
    }

    // If not additive, clear the existing selection first.
    if (!additive) {
        PlayerSelectionClear(state);
    }

    // Iterate through all planets and select those owned by the specified faction.
    // It's possible for this function to be called with planets selected 
    // which do not belong to the specified faction as for example the player might
    // have had some selected, then lost control of them with that same selection active.
    size_t processed = 0;

    // Limit the iteration to the smaller of the level's planet count
    // or the selection state's capacity to avoid out-of-bounds access.
    size_t limit = level->planetCount < state->capacity ? level->planetCount : state->capacity;
    for (size_t i = 0; i < limit; ++i) {
        if (level->planets[i].owner == owner) {
            PlayerSelectionSet(state, i, true);
            processed += 1;
        }
    }

    // Return true if at least one owned planet was processed.
    return processed > 0;
}

/**
 * Resizes the control group buffers to accommodate the provided planet count.
 * Existing data is cleared when the capacity changes.
 * @param groups Pointer to the PlayerControlGroups to reset.
 * @param planetCount Number of planets to support.
 * @return true on success, false otherwise.
 */
bool PlayerControlGroupsReset(PlayerControlGroups *groups, size_t planetCount) {
    // Basic validation of input pointer.
    if (groups == NULL) {
        return false;
    }

    // If planetCount is zero, free all existing buffers and set capacity to zero.
    if (planetCount == 0) {
        for (size_t i = 0; i < PLAYER_MAX_CONTROL_GROUPS; ++i) {
            free(groups->groups[i]);
            groups->groups[i] = NULL;
        }
        groups->capacity = 0;
        return true;
    }

    // Otherwise, if the capacity is changing, reallocate all buffers.
    if (groups->capacity != planetCount) {

        // For each control group, free any existing buffer.
        for (size_t i = 0; i < PLAYER_MAX_CONTROL_GROUPS; ++i) {
            free(groups->groups[i]);
            groups->groups[i] = NULL;
        }

        // For each control group, allocate a new buffer
        // sized to hold planetCount booleans.
        for (size_t i = 0; i < PLAYER_MAX_CONTROL_GROUPS; ++i) {
            groups->groups[i] = (bool *)calloc(planetCount, sizeof(bool));

            // If any allocation fails, free all previously allocated buffers
            // and set capacity to zero before returning failure.
            if (groups->groups[i] == NULL) {
                printf("Failed to allocate control group buffer :(.\n");
                for (size_t j = 0; j <= i; ++j) {
                    free(groups->groups[j]);
                    groups->groups[j] = NULL;
                }
                groups->capacity = 0;
                return false;
            }
        }

        // Mark the control groups as being able to track planetCount planets.
        groups->capacity = planetCount;
        return true;
    }

    // If the capacity is unchanged, simply clear all existing buffers.
    for (size_t i = 0; i < PLAYER_MAX_CONTROL_GROUPS; ++i) {
        if (groups->groups[i] != NULL) {
            memset(groups->groups[i], 0, planetCount * sizeof(bool));
        }
    }
    return true;
}

/**
 * Releases memory owned by the control groups structure.
 * @param groups Pointer to the PlayerControlGroups to free.
 */
void PlayerControlGroupsFree(PlayerControlGroups *groups) {
    // Basic validation of input pointer.
    if (groups == NULL) {
        return;
    }

    // Free each control group buffer if it exists,
    for (size_t i = 0; i < PLAYER_MAX_CONTROL_GROUPS; ++i) {
        free(groups->groups[i]);
        groups->groups[i] = NULL;
    }

    // Reset capacity to zero.
    groups->capacity = 0;
}

/**
 * Replaces the specified control group with the current selection.
 * @param groups Pointer to the PlayerControlGroups to modify.
 * @param groupIndex Index of the control group (0-9).
 * @param selection Pointer to the current selection state.
 * @return true if the group was updated, false otherwise.
 */
bool PlayerControlGroupsOverwrite(PlayerControlGroups *groups,
    size_t groupIndex,
    const PlayerSelectionState *selection) {

    // Basic validation of input parameters.
    if (groups == NULL || selection == NULL || groupIndex >= PLAYER_MAX_CONTROL_GROUPS) {
        return false;
    }

    // If the control group buffer does not exist, cannot overwrite.
    if (groups->capacity == 0 || groups->groups[groupIndex] == NULL) {
        return false;
    }

    // Clear the existing control group selection by setting all booleans to false (0).
    memset(groups->groups[groupIndex], 0, groups->capacity * sizeof(bool));

    // If there is no selection buffer, nothing to add.
    // So the control group is now empty.
    if (selection->selectedPlanets == NULL || selection->capacity == 0) {
        return true;
    }

    // Iterate through the selection and copy selected planets into the control group.
    size_t limit = selection->capacity < groups->capacity ? selection->capacity : groups->capacity;
    for (size_t i = 0; i < limit; ++i) {
        if (selection->selectedPlanets[i]) {
            groups->groups[groupIndex][i] = true;
        }
    }
    return true;
}

/**
 * Adds the planets from the current selection into the specified control group.
 * @param groups Pointer to the PlayerControlGroups to modify.
 * @param groupIndex Index of the control group (0-9).
 * @param selection Pointer to the current selection state.
 * @return true if the group was updated, false otherwise.
 */
bool PlayerControlGroupsAdd(PlayerControlGroups *groups,
    size_t groupIndex,
    const PlayerSelectionState *selection) {

    // Basic validation of input parameters.
    if (groups == NULL || selection == NULL || groupIndex >= PLAYER_MAX_CONTROL_GROUPS) {
        return false;
    }

    // If the control group buffer does not exist, cannot add to it.
    if (groups->capacity == 0 || groups->groups[groupIndex] == NULL) {
        return false;
    }

    // If there is no selection buffer, nothing to add.
    if (selection->selectedPlanets == NULL || selection->capacity == 0) {
        return true;
    }

    // Iterate through the selection and copy selected planets into the control group.
    // A more efficient approach might find some way to efficiently extract out only planets
    // not already in the control group, and then iterate over only those, but this function
    // is hardly going to be a performance bottleneck in practice even with this simple approach.
    size_t limit = selection->capacity < groups->capacity ? selection->capacity : groups->capacity;
    for (size_t i = 0; i < limit; ++i) {
        if (selection->selectedPlanets[i]) {
            groups->groups[groupIndex][i] = true;
        }
    }
    return true;
}

/**
 * Applies the specified control group to the current selection, filtering by ownership.
 * @param groups Pointer to the PlayerControlGroups to read from.
 * @param groupIndex Index of the control group (0-9).
 * @param level Pointer to the current Level structure.
 * @param owner Pointer to the owning faction.
 * @param selection Pointer to the selection state to update.
 * @param additive When true the group's planets are added to the existing selection.
 * @return true if at least one planet was selected, false otherwise.
 */
bool PlayerControlGroupsApply(const PlayerControlGroups *groups,
    size_t groupIndex,
    const Level *level,
    const Faction *owner,
    PlayerSelectionState *selection,
    bool additive) {

    // Basic validation of input parameters.
    if (groups == NULL || level == NULL || owner == NULL || selection == NULL) {
        return false;
    }

    // If the control group buffer does not exist, cannot apply.
    if (groupIndex >= PLAYER_MAX_CONTROL_GROUPS) {
        return false;
    }

    if (groups->capacity == 0 || groups->groups[groupIndex] == NULL) {
        return false;
    }

    if (selection->selectedPlanets == NULL || selection->capacity == 0) {
        return false;
    }

    // If not additive, clear the existing selection first.
    if (!additive) {
        PlayerSelectionClear(selection);
    }

    // Iterate through the specified control group and add owned planets to the selection.
    size_t limit = level->planetCount < groups->capacity ? level->planetCount : groups->capacity;
    size_t selectedCount = 0;
    for (size_t i = 0; i < limit; ++i) {
        if (!groups->groups[groupIndex][i]) {
            continue;
        }
        if (level->planets[i].owner != owner) {
            continue;
        }
        if (PlayerSelectionSet(selection, i, true)) {
            selectedCount += 1;
        }
    }
    return selectedCount > 0;
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
