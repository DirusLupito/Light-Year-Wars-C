/**
 * Header file for Player Interface utility functions,
 * which holds helper functions for managing player interactions
 * with the game level, such as planet selection and sending orders.
 * @author abmize
 * @file playerInterfaceUtilities.h
 */
#ifndef _PLAYER_INTERFACE_UTILITIES_H_
#define _PLAYER_INTERFACE_UTILITIES_H_

#include <winsock2.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Objects/level.h"

// Structure to hold the player's current selection state.
// Contains a dynamic array of booleans indicating selected planets,
// as well as capacity and count of selected planets, which helps manage the selection.
typedef struct PlayerSelectionState {
    bool *selectedPlanets;
    size_t capacity;
    size_t count;
} PlayerSelectionState;

// Define the maximum number of control groups a player can have.
// Corresponds to the number keys 0-9.
#define PLAYER_MAX_CONTROL_GROUPS 10

// Structure to hold the player's control groups.
// Each control group is a dynamic array of booleans indicating selected planets.
// The capacity indicates how many planets can be tracked by each control group.
typedef struct PlayerControlGroups {
    bool *groups[PLAYER_MAX_CONTROL_GROUPS];
    size_t capacity;
} PlayerControlGroups;

/**
 * Resets the player's selection state to accommodate a new number of planets.
 * Allocates or reallocates the selection buffer as needed.
 * @param state Pointer to the PlayerSelectionState to reset.
 * @param planetCount The number of planets in the level.
 * @return true if the reset was successful, false otherwise.
 */
bool PlayerSelectionReset(PlayerSelectionState *state, size_t planetCount);

/**
 * Clears the player's current selection state,
 * deselecting all currently selected planets.
 * @param state Pointer to the PlayerSelectionState to clear.
 */
void PlayerSelectionClear(PlayerSelectionState *state);

/**
 * Toggles the selection state of a planet at the given index.
 * If additive is false, clears the current selection before toggling.
 * @param state Pointer to the PlayerSelectionState to modify.
 * @param index The index of the planet to toggle.
 * @param additive If true, adds/removes from the current selection;
 *                 if false, replaces the selection with just this planet.
 * @return true if the toggle was successful, false otherwise.
 */
bool PlayerSelectionToggle(PlayerSelectionState *state, size_t index, bool additive);

/**
 * Frees the resources associated with the player's selection state.
 * @param state Pointer to the PlayerSelectionState to free.
 */
void PlayerSelectionFree(PlayerSelectionState *state);

/**
 * Sets the selection state of the specified planet.
 * @param state Pointer to the PlayerSelectionState to modify.
 * @param index Index of the planet.
 * @param selected true to select, false to deselect.
 * @return true if the operation succeeded, false otherwise.
 */
bool PlayerSelectionSet(PlayerSelectionState *state, size_t index, bool selected);

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
    bool additive);

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
    size_t destinationIndex);

/**
 * Resizes the control group buffers to accommodate the provided planet count.
 * Existing data is cleared when the capacity changes.
 * @param groups Pointer to the PlayerControlGroups to reset.
 * @param planetCount Number of planets to support.
 * @return true on success, false otherwise.
 */
bool PlayerControlGroupsReset(PlayerControlGroups *groups, size_t planetCount);

/**
 * Releases memory owned by the control groups structure.
 * @param groups Pointer to the PlayerControlGroups to free.
 */
void PlayerControlGroupsFree(PlayerControlGroups *groups);

/**
 * Replaces the specified control group with the current selection.
 * @param groups Pointer to the PlayerControlGroups to modify.
 * @param groupIndex Index of the control group (0-9).
 * @param selection Pointer to the current selection state.
 * @return true if the group was updated, false otherwise.
 */
bool PlayerControlGroupsOverwrite(PlayerControlGroups *groups,
    size_t groupIndex,
    const PlayerSelectionState *selection);

/**
 * Adds the planets from the current selection into the specified control group.
 * @param groups Pointer to the PlayerControlGroups to modify.
 * @param groupIndex Index of the control group (0-9).
 * @param selection Pointer to the current selection state.
 * @return true if the group was updated, false otherwise.
 */
bool PlayerControlGroupsAdd(PlayerControlGroups *groups,
    size_t groupIndex,
    const PlayerSelectionState *selection);

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
    bool additive);

#endif /* _PLAYER_INTERFACE_UTILITIES_H_ */
