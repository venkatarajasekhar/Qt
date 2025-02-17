// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/bind.h"
#include "base/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/test/error_callback_support.h"
#include "sql/test/scoped_error_ignorer.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/sqlite/sqlite3.h"

namespace {

class SQLStatementTest : public testing::Test {
 public:
  virtual void SetUp() {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    ASSERT_TRUE(db_.Open(temp_dir_.path().AppendASCII("SQLStatementTest.db")));
  }

  virtual void TearDown() {
    db_.Close();
  }

  sql::Connection& db() { return db_; }

 private:
  base::ScopedTempDir temp_dir_;
  sql::Connection db_;
};

}  // namespace

TEST_F(SQLStatementTest, Assign) {
  sql::Statement s;
  EXPECT_FALSE(s.is_valid());

  s.Assign(db().GetUniqueStatement("CREATE TABLE foo (a, b)"));
  EXPECT_TRUE(s.is_valid());
}

TEST_F(SQLStatementTest, Run) {
  ASSERT_TRUE(db().Execute("CREATE TABLE foo (a, b)"));
  ASSERT_TRUE(db().Execute("INSERT INTO foo (a, b) VALUES (3, 12)"));

  sql::Statement s(db().GetUniqueStatement("SELECT b FROM foo WHERE a=?"));
  EXPECT_FALSE(s.Succeeded());

  // Stepping it won't work since we haven't bound the value.
  EXPECT_FALSE(s.Step());

  // Run should fail since this produces output, and we should use Step(). This
  // gets a bit wonky since sqlite says this is OK so succeeded is set.
  s.Reset(true);
  s.BindInt(0, 3);
  EXPECT_FALSE(s.Run());
  EXPECT_EQ(SQLITE_ROW, db().GetErrorCode());
  EXPECT_TRUE(s.Succeeded());

  // Resetting it should put it back to the previous state (not runnable).
  s.Reset(true);
  EXPECT_FALSE(s.Succeeded());

  // Binding and stepping should produce one row.
  s.BindInt(0, 3);
  EXPECT_TRUE(s.Step());
  EXPECT_TRUE(s.Succeeded());
  EXPECT_EQ(12, s.ColumnInt(0));
  EXPECT_FALSE(s.Step());
  EXPECT_TRUE(s.Succeeded());
}

// Error callback called for error running a statement.
TEST_F(SQLStatementTest, ErrorCallback) {
  ASSERT_TRUE(db().Execute("CREATE TABLE foo (a INTEGER PRIMARY KEY, b)"));

  int error = SQLITE_OK;
  sql::ScopedErrorCallback sec(
      &db(), base::Bind(&sql::CaptureErrorCallback, &error));

  // Insert in the foo table the primary key. It is an error to insert
  // something other than an number. This error causes the error callback
  // handler to be called with SQLITE_MISMATCH as error code.
  sql::Statement s(db().GetUniqueStatement("INSERT INTO foo (a) VALUES (?)"));
  EXPECT_TRUE(s.is_valid());
  s.BindCString(0, "bad bad");
  EXPECT_FALSE(s.Run());
  EXPECT_EQ(SQLITE_MISMATCH, error);
}

// Error ignorer works for error running a statement.
TEST_F(SQLStatementTest, ScopedIgnoreError) {
  ASSERT_TRUE(db().Execute("CREATE TABLE foo (a INTEGER PRIMARY KEY, b)"));

  sql::Statement s(db().GetUniqueStatement("INSERT INTO foo (a) VALUES (?)"));
  EXPECT_TRUE(s.is_valid());

  sql::ScopedErrorIgnorer ignore_errors;
  ignore_errors.IgnoreError(SQLITE_MISMATCH);
  s.BindCString(0, "bad bad");
  ASSERT_FALSE(s.Run());
  ASSERT_TRUE(ignore_errors.CheckIgnoredErrors());
}

TEST_F(SQLStatementTest, Reset) {
  ASSERT_TRUE(db().Execute("CREATE TABLE foo (a, b)"));
  ASSERT_TRUE(db().Execute("INSERT INTO foo (a, b) VALUES (3, 12)"));
  ASSERT_TRUE(db().Execute("INSERT INTO foo (a, b) VALUES (4, 13)"));

  sql::Statement s(db().GetUniqueStatement(
      "SELECT b FROM foo WHERE a = ? "));
  s.BindInt(0, 3);
  ASSERT_TRUE(s.Step());
  EXPECT_EQ(12, s.ColumnInt(0));
  ASSERT_FALSE(s.Step());

  s.Reset(false);
  // Verify that we can get all rows again.
  ASSERT_TRUE(s.Step());
  EXPECT_EQ(12, s.ColumnInt(0));
  EXPECT_FALSE(s.Step());

  s.Reset(true);
  ASSERT_FALSE(s.Step());
}
