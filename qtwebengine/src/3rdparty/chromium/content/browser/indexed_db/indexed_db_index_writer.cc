// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/indexed_db/indexed_db_index_writer.h"

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "content/browser/indexed_db/indexed_db_backing_store.h"
#include "content/browser/indexed_db/indexed_db_tracing.h"
#include "content/browser/indexed_db/indexed_db_transaction.h"
#include "content/common/indexed_db/indexed_db_key.h"
#include "content/common/indexed_db/indexed_db_key_path.h"
#include "content/common/indexed_db/indexed_db_key_range.h"

using base::ASCIIToUTF16;

namespace content {

IndexWriter::IndexWriter(
    const IndexedDBIndexMetadata& index_metadata)
    : index_metadata_(index_metadata) {}

IndexWriter::IndexWriter(
    const IndexedDBIndexMetadata& index_metadata,
    const IndexedDBDatabase::IndexKeys& index_keys)
    : index_metadata_(index_metadata), index_keys_(index_keys) {}

IndexWriter::~IndexWriter() {}

bool IndexWriter::VerifyIndexKeys(
    IndexedDBBackingStore* backing_store,
    IndexedDBBackingStore::Transaction* transaction,
    int64 database_id,
    int64 object_store_id,
    int64 index_id,
    bool* can_add_keys,
    const IndexedDBKey& primary_key,
    base::string16* error_message) const {
  *can_add_keys = false;
  DCHECK_EQ(index_id, index_keys_.first);
  for (size_t i = 0; i < index_keys_.second.size(); ++i) {
    bool ok = AddingKeyAllowed(backing_store,
                               transaction,
                               database_id,
                               object_store_id,
                               index_id,
                               (index_keys_.second)[i],
                               primary_key,
                               can_add_keys);
    if (!ok)
      return false;
    if (!*can_add_keys) {
      if (error_message) {
        *error_message = ASCIIToUTF16("Unable to add key to index '") +
                         index_metadata_.name +
                         ASCIIToUTF16("': at least one key does not satisfy "
                                      "the uniqueness requirements.");
      }
      return true;
    }
  }
  *can_add_keys = true;
  return true;
}

void IndexWriter::WriteIndexKeys(
    const IndexedDBBackingStore::RecordIdentifier& record_identifier,
    IndexedDBBackingStore* backing_store,
    IndexedDBBackingStore::Transaction* transaction,
    int64 database_id,
    int64 object_store_id) const {
  int64 index_id = index_metadata_.id;
  DCHECK_EQ(index_id, index_keys_.first);
  for (size_t i = 0; i < index_keys_.second.size(); ++i) {
    leveldb::Status s =
        backing_store->PutIndexDataForRecord(transaction,
                                             database_id,
                                             object_store_id,
                                             index_id,
                                             index_keys_.second[i],
                                             record_identifier);
    // This should have already been verified as a valid write during
    // verify_index_keys.
    DCHECK(s.ok());
  }
}

bool IndexWriter::AddingKeyAllowed(
    IndexedDBBackingStore* backing_store,
    IndexedDBBackingStore::Transaction* transaction,
    int64 database_id,
    int64 object_store_id,
    int64 index_id,
    const IndexedDBKey& index_key,
    const IndexedDBKey& primary_key,
    bool* allowed) const {
  *allowed = false;
  if (!index_metadata_.unique) {
    *allowed = true;
    return true;
  }

  scoped_ptr<IndexedDBKey> found_primary_key;
  bool found = false;
  leveldb::Status s = backing_store->KeyExistsInIndex(transaction,
                                                      database_id,
                                                      object_store_id,
                                                      index_id,
                                                      index_key,
                                                      &found_primary_key,
                                                      &found);
  if (!s.ok())
    return false;
  if (!found ||
      (primary_key.IsValid() && found_primary_key->Equals(primary_key)))
    *allowed = true;
  return true;
}

bool MakeIndexWriters(
    IndexedDBTransaction* transaction,
    IndexedDBBackingStore* backing_store,
    int64 database_id,
    const IndexedDBObjectStoreMetadata& object_store,
    const IndexedDBKey& primary_key,  // makes a copy
    bool key_was_generated,
    const std::vector<IndexedDBDatabase::IndexKeys>& index_keys,
    ScopedVector<IndexWriter>* index_writers,
    base::string16* error_message,
    bool* completed) {
  *completed = false;

  for (std::vector<IndexedDBDatabase::IndexKeys>::const_iterator it =
           index_keys.begin();
       it != index_keys.end();
       ++it) {
    IndexedDBObjectStoreMetadata::IndexMap::const_iterator found =
        object_store.indexes.find(it->first);
    if (found == object_store.indexes.end())
      continue;
    const IndexedDBIndexMetadata& index = found->second;
    IndexedDBDatabase::IndexKeys keys = *it;

    // If the object_store is using auto_increment, then any indexes with an
    // identical key_path need to also use the primary (generated) key as a key.
    if (key_was_generated && (index.key_path == object_store.key_path))
      keys.second.push_back(primary_key);

    scoped_ptr<IndexWriter> index_writer(new IndexWriter(index, keys));
    bool can_add_keys = false;
    bool backing_store_success =
        index_writer->VerifyIndexKeys(backing_store,
                                      transaction->BackingStoreTransaction(),
                                      database_id,
                                      object_store.id,
                                      index.id,
                                      &can_add_keys,
                                      primary_key,
                                      error_message);
    if (!backing_store_success)
      return false;
    if (!can_add_keys)
      return true;

    index_writers->push_back(index_writer.release());
  }

  *completed = true;
  return true;
}

}  // namespace content
