/*-------------------------------------------------------------------------
 *
 * tile_group.h
 * file description
 *
 * Copyright(c) 2015, CMU
 *
 * /n-store/src/storage/tile_group.h
 *
 *-------------------------------------------------------------------------
 */

#pragma once

#include "backend/catalog/manager.h"
#include "backend/storage/tile.h"
#include "backend/storage/tile_group_header.h"

#include <cassert>

namespace peloton {
namespace storage {

//===--------------------------------------------------------------------===//
// Tile Group
//===--------------------------------------------------------------------===//

class AbstractTable;
class TileGroupIterator;

/**
 * Represents a group of tiles logically horizontally contiguous.
 *
 * < <Tile 1> <Tile 2> .. <Tile n> >
 *
 * Look at TileGroupHeader for MVCC implementation.
 *
 * TileGroups are only instantiated via TileGroupFactory.
 */
class TileGroup {
    friend class Tile;
    friend class TileGroupFactory;

    TileGroup() = delete;
    TileGroup(TileGroup const&) = delete;

public:

    // Tile group constructor
    TileGroup(TileGroupHeader *tile_group_header,
              AbstractTable *table,
              AbstractBackend *backend,
              const std::vector<catalog::Schema>& schemas,
              int tuple_count);

    ~TileGroup() {

        // clean up tiles
        for(auto tile : tiles) {
            delete tile;
        }

        // clean up tile group header
        delete tile_group_header;
    }

    //===--------------------------------------------------------------------===//
    // Operations
    //===--------------------------------------------------------------------===//

    // insert tuple at next available slot in tile if a slot exists
    oid_t InsertTuple(txn_id_t transaction_id, const Tuple *tuple);

    // reclaim tuple at given slot
    void ReclaimTuple(oid_t tuple_slot_id);

    // returns tuple at given slot in tile if it exists
    Tuple* SelectTuple(oid_t tile_offset, oid_t tuple_slot_id);

    // returns tuple at given slot if it exists
    Tuple *SelectTuple(oid_t tuple_slot_id);

    // delete tuple at given slot if it is not already locked
    bool DeleteTuple(txn_id_t transaction_id, oid_t tuple_slot_id);

    //===--------------------------------------------------------------------===//
    // Transaction Processing
    //===--------------------------------------------------------------------===//

    // commit the inserted tuple
    void CommitInsertedTuple(oid_t tuple_slot_id, cid_t commit_id);

    // commit the deleted tuple
    void CommitDeletedTuple(oid_t tuple_slot_id, txn_id_t transaction_id, cid_t commit_id);

    // abort the inserted tuple
    void AbortInsertedTuple(oid_t tuple_slot_id);

    // abort the deleted tuple
    void AbortDeletedTuple(oid_t tuple_slot_id);

    //===--------------------------------------------------------------------===//
    // Utilities
    //===--------------------------------------------------------------------===//

    // Get a string representation of this tile group
    friend std::ostream& operator<<(std::ostream& os, const TileGroup& tile_group);

    oid_t GetNextTupleSlot() const {
        return tile_group_header->GetNextTupleSlot();
    }

    oid_t GetActiveTupleCount() const {
        return tile_group_header->GetActiveTupleCount();
    }

    oid_t GetAllocatedTupleCount() const {
        return num_tuple_slots;
    }

    TileGroupHeader *GetHeader() const {
        return tile_group_header;
    }

    unsigned int NumTiles() const {
        return tiles.size();
    }

    // Get the tile at given offset in the tile group
    Tile *GetTile(const oid_t tile_itr) const;

    oid_t GetTileId(const oid_t tile_id) const {
        assert(tiles[tile_id]);
        return tiles[tile_id]->GetTileId();
    }

    Pool *GetTilePool(const oid_t tile_id) const {
        Tile *tile = GetTile(tile_id);

        if(tile != nullptr)
            return tile->GetPool();

        return nullptr;
    }

    oid_t GetTileGroupId() const {
        return tile_group_id;
    }

    void SetTileGroupId(oid_t tile_group_id_) {
        tile_group_id = tile_group_id_;
    }

    AbstractBackend* GetBackend() const {
        return backend;
    }

    std::vector<catalog::Schema> &GetTileSchemas() {
        return tile_schemas;
    }

    size_t GetTileCount() const {
        return tile_count;
    }

    void LocateTileAndColumn(oid_t column_id, oid_t &tile_offset, oid_t &tile_column_id);

    oid_t GetTileIdFromColumnId(oid_t column_id);

    oid_t GetTileColumnId(oid_t column_id);

    Value GetValue(oid_t tuple_id, oid_t column_id);

protected:

    //===--------------------------------------------------------------------===//
    // Data members
    //===--------------------------------------------------------------------===//

    // Catalog information
    oid_t database_id;  // TODO REMOVE
    oid_t table_id;     // TODO REMOVE
    oid_t tile_group_id;

    // backend
    AbstractBackend* backend;   // TODO REMOVE(?)

    // mapping to tile schemas
    std::vector<catalog::Schema> tile_schemas;

    // set of tiles
    std::vector<Tile*> tiles;

    // associated tile group
    TileGroupHeader* tile_group_header;

    // associated table
    AbstractTable* table; // TODO: Remove this! It is a waste of space!!

    // number of tuple slots allocated
    oid_t num_tuple_slots;

    // number of tiles
    oid_t tile_count;

    std::mutex tile_group_mutex;
};


} // End storage namespace
} // End peloton namespace
