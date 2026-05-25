//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Builders/Isaac/IsaacDungeonModel.h"


void UIsaacDungeonModel::RemoveStylingFromRoom(int32 RoomId) {
    for (FIsaacRoom& Room : Rooms) {
        if (Room.Id == RoomId) {
            for (FIsaacRoomTile& Tile : Room.Layout.Tiles) {
                Tile.tileType = EIsaacRoomTileType::Floor;
            }
            break;
        }
    }
}

void UIsaacDungeonModel::Reset() {
	Super::Reset();
    Rooms.Reset();
    Doors.Reset();
}

