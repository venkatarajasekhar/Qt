// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_BROWSER_DATABASE_DATABASES_TABLE_H_
#define WEBKIT_BROWSER_DATABASE_DATABASES_TABLE_H_

#include <vector>

#include "base/strings/string16.h"
#include "webkit/browser/webkit_storage_browser_export.h"

namespace sql {
class Connection;
}

namespace webkit_database {

struct WEBKIT_STORAGE_BROWSER_EXPORT_PRIVATE DatabaseDetails {
  DatabaseDetails();
  ~DatabaseDetails();

  std::string origin_identifier;
  base::string16 database_name;
  base::string16 description;
  int64 estimated_size;
};

class WEBKIT_STORAGE_BROWSER_EXPORT_PRIVATE DatabasesTable {
 public:
  explicit DatabasesTable(sql::Connection* db) : db_(db) { }

  bool Init();
  int64 GetDatabaseID(const std::string& origin_identifier,
                      const base::string16& database_name);
  bool GetDatabaseDetails(const std::string& origin_identifier,
                          const base::string16& database_name,
                          DatabaseDetails* details);
  bool InsertDatabaseDetails(const DatabaseDetails& details);
  bool UpdateDatabaseDetails(const DatabaseDetails& details);
  bool DeleteDatabaseDetails(const std::string& origin_identifier,
                             const base::string16& database_name);
  bool GetAllOriginIdentifiers(std::vector<std::string>* origin_identifiers);
  bool GetAllDatabaseDetailsForOriginIdentifier(
      const std::string& origin_identifier,
      std::vector<DatabaseDetails>* details);
  bool DeleteOriginIdentifier(const std::string& origin_identifier);
 private:
  sql::Connection* db_;
};

}  // namespace webkit_database

#endif  // WEBKIT_BROWSER_DATABASE_DATABASES_TABLE_H_
