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

#endif /* _PLAYER_INTERFACE_UTILITIES_H_ */
