/*
 * Copyright 2011 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SFNTLY_CPP_SRC_SFNTLY_TABLE_GENERIC_TABLE_BUILDER_H_
#define SFNTLY_CPP_SRC_SFNTLY_TABLE_GENERIC_TABLE_BUILDER_H_

#include "sfntly/table/table_based_table_builder.h"

namespace sfntly {

// A table builder to do the minimal table building for an unknown table type.
class GenericTableBuilder : public TableBasedTableBuilder,
                            public RefCounted<GenericTableBuilder> {
 public:
  virtual ~GenericTableBuilder();

  virtual CALLER_ATTACH FontDataTable* SubBuildTable(ReadableFontData* data);

  static CALLER_ATTACH GenericTableBuilder*
      CreateBuilder(Header* header, WritableFontData* data);

 private:
  GenericTableBuilder(Header* header, WritableFontData* data);
  GenericTableBuilder(Header* header, ReadableFontData* data);
};

}  // namespace sfntly

#endif  // SFNTLY_CPP_SRC_SFNTLY_TABLE_BYTE_ARRAY_TABLE_BUILDER_H_
