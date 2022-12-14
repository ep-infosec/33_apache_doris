// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include "olap/row_block.h"

#include <gtest/gtest.h>

#include <sstream>

#include "common/object_pool.h"
#include "exprs/expr.h"
#include "exprs/expr_context.h"
#include "olap/tablet.h"
#include "runtime/runtime_state.h"
#include "util/debug_util.h"

using std::vector;
using std::string;
using std::endl;
using std::stringstream;

namespace doris {

class TestRowBlock : public testing::Test {
public:
    TestRowBlock() {}
    void SetUp() {}
    void TearDown() {}
};

void init_tablet_schema(TabletSchemaSPtr tablet_schema) {
    TabletSchemaPB tablet_schema_pb;
    {
        // k1: bigint
        {
            ColumnPB* column_1 = tablet_schema_pb.add_column();
            column_1->set_unique_id(1);
            column_1->set_name("k1");
            column_1->set_type("BIGINT");
            column_1->set_is_key(true);
            column_1->set_length(8);
            column_1->set_aggregation("NONE");
        }
        // k2: char
        {
            ColumnPB* column_2 = tablet_schema_pb.add_column();
            column_2->set_unique_id(2);
            column_2->set_name("k2");
            column_2->set_type("CHAR");
            column_2->set_is_key(true);
            column_2->set_length(10);
            column_2->set_aggregation("NONE");
        }
        // k3: varchar
        {
            ColumnPB* column_3 = tablet_schema_pb.add_column();
            column_3->set_unique_id(3);
            column_3->set_name("k3");
            column_3->set_type("VARCHAR");
            column_3->set_is_key(true);
            column_3->set_length(20);
            column_3->set_aggregation("NONE");
        }
    }
    tablet_schema->init_from_pb(tablet_schema_pb);
}

TEST_F(TestRowBlock, init) {
    TabletSchemaSPtr tablet_schema = std::make_shared<TabletSchema>();
    init_tablet_schema(tablet_schema);
    {
        // has nullbyte
        RowBlock block(tablet_schema);
        RowBlockInfo block_info;
        block_info.row_num = 1024;
        block_info.null_supported = true;
        block.init(block_info);
        EXPECT_EQ(9 + 17 + 17, block._mem_row_bytes);
    }
    {
        // has nullbyte
        RowBlock block(tablet_schema);
        RowBlockInfo block_info;
        block_info.row_num = 1024;
        block_info.null_supported = false;
        block.init(block_info);
        EXPECT_EQ(9 + 17 + 17, block._mem_row_bytes);
    }
    {
        RowBlock block(tablet_schema);
        RowBlockInfo block_info;
        block_info.row_num = 1024;
        block_info.null_supported = true;
        block_info.column_ids.push_back(1);
        block.init(block_info);
        // null + sizeof(Slice)
        EXPECT_EQ(17, block._mem_row_bytes);
        EXPECT_EQ(std::numeric_limits<size_t>::max(), block._field_offset_in_memory[0]);
        EXPECT_EQ(0, block._field_offset_in_memory[1]);
        EXPECT_EQ(std::numeric_limits<size_t>::max(), block._field_offset_in_memory[2]);
    }
}

TEST_F(TestRowBlock, write_and_read) {
    TabletSchemaSPtr tablet_schema = std::make_shared<TabletSchema>();
    init_tablet_schema(tablet_schema);
    RowBlock block(tablet_schema);
    RowBlockInfo block_info;
    block_info.row_num = 1024;
    block_info.null_supported = true;
    block.init(block_info);

    RowCursor row;
    row.init(tablet_schema);
    for (int i = 0; i < 5; ++i) {
        block.get_row(i, &row);

        // bigint
        {
            int64_t val = i;
            row.set_not_null(0);
            row.set_field_content(0, (const char*)&val, block.mem_pool());
        }
        // char
        {
            char buf[10];
            memset(buf, 'a' + i, 10);
            Slice val(buf, 10);
            row.set_not_null(1);
            row.set_field_content(1, (const char*)&val, block.mem_pool());
        }
        // varchar
        {
            char buf[10];
            memset(buf, '0' + i, 10);
            Slice val(buf, 10);
            row.set_not_null(2);
            row.set_field_content(2, (const char*)&val, block.mem_pool());
        }
    }
    block.finalize(5);
    EXPECT_EQ(5, block.row_num());
}

TEST_F(TestRowBlock, write_and_read_without_nullbyte) {
    TabletSchemaSPtr tablet_schema = std::make_shared<TabletSchema>();
    init_tablet_schema(tablet_schema);
    RowBlock block(tablet_schema);
    RowBlockInfo block_info;
    block_info.row_num = 1024;
    block_info.null_supported = false;
    block.init(block_info);

    RowCursor row;
    row.init(tablet_schema);
    for (int i = 0; i < 5; ++i) {
        block.get_row(i, &row);

        // bigint
        {
            int64_t val = i;
            row.set_not_null(0);
            row.set_field_content(0, (const char*)&val, block.mem_pool());
        }
        // char
        {
            char buf[10];
            memset(buf, 'a' + i, 10);
            Slice val(buf, 10);
            row.set_not_null(1);
            row.set_field_content(1, (const char*)&val, block.mem_pool());
        }
        // varchar
        {
            char buf[10];
            memset(buf, '0' + i, 10);
            Slice val(buf, 10);
            row.set_not_null(2);
            row.set_field_content(2, (const char*)&val, block.mem_pool());
        }
    }
    block.finalize(5);
    EXPECT_EQ(5, block.row_num());
}

TEST_F(TestRowBlock, compress_failed) {
    TabletSchemaSPtr tablet_schema = std::make_shared<TabletSchema>();
    init_tablet_schema(tablet_schema);
    RowBlock block(tablet_schema);
    RowBlockInfo block_info;
    block_info.row_num = 1024;
    block_info.null_supported = true;
    block.init(block_info);

    RowCursor row;
    row.init(tablet_schema);
    for (int i = 0; i < 5; ++i) {
        block.get_row(i, &row);

        // bigint
        {
            int64_t val = i;
            row.set_field_content(0, (const char*)&val, block.mem_pool());
        }
        // char
        {
            char buf[10];
            memset(buf, 'a' + i, 10);
            Slice val(buf, 10);
            row.set_field_content(1, (const char*)&val, block.mem_pool());
        }
        // varchar
        {
            char buf[10];
            memset(buf, '0' + i, 10);
            Slice val(buf, 10);
            row.set_field_content(2, (const char*)&val, block.mem_pool());
        }
    }
    block.finalize(5);
    EXPECT_EQ(5, block.row_num());
}

TEST_F(TestRowBlock, decompress_failed) {
    TabletSchemaSPtr tablet_schema = std::make_shared<TabletSchema>();
    init_tablet_schema(tablet_schema);
    RowBlock block(tablet_schema);
    RowBlockInfo block_info;
    block_info.row_num = 1024;
    block_info.null_supported = true;
    block.init(block_info);

    RowCursor row;
    row.init(tablet_schema);
    for (int i = 0; i < 5; ++i) {
        block.get_row(i, &row);

        // bigint
        {
            int64_t val = i;
            row.set_field_content(0, (const char*)&val, block.mem_pool());
        }
        // char
        {
            char buf[10];
            memset(buf, 'a' + i, 10);
            Slice val(buf, 10);
            row.set_field_content(1, (const char*)&val, block.mem_pool());
        }
        // varchar
        {
            char buf[10];
            memset(buf, '0' + i, 10);
            Slice val(buf, 10);
            row.set_field_content(2, (const char*)&val, block.mem_pool());
        }
    }
    block.finalize(5);
    EXPECT_EQ(5, block.row_num());
}

TEST_F(TestRowBlock, clear) {
    TabletSchemaSPtr tablet_schema = std::make_shared<TabletSchema>();
    init_tablet_schema(tablet_schema);
    RowBlock block(tablet_schema);
    RowBlockInfo block_info;
    block_info.row_num = 1024;
    block_info.null_supported = true;
    block.init(block_info);

    block.finalize(5);
    EXPECT_EQ(5, block.row_num());
    EXPECT_EQ(1024, block.capacity());
    block.clear();
    EXPECT_EQ(1024, block.row_num());
}

TEST_F(TestRowBlock, pos_limit) {
    TabletSchemaSPtr tablet_schema = std::make_shared<TabletSchema>();
    init_tablet_schema(tablet_schema);
    RowBlock block(tablet_schema);
    RowBlockInfo block_info;
    block_info.row_num = 1024;
    block_info.null_supported = true;
    block.init(block_info);

    // assert init value
    EXPECT_EQ(0, block.pos());
    EXPECT_EQ(0, block.limit());
    EXPECT_FALSE(block.has_remaining());
    EXPECT_EQ(DEL_PARTIAL_SATISFIED, block.block_status());

    block.set_limit(100);
    EXPECT_EQ(100, block.limit());
    EXPECT_TRUE(block.has_remaining());
    EXPECT_EQ(100, block.remaining());

    block.set_pos(2);
    EXPECT_TRUE(block.has_remaining());
    EXPECT_EQ(98, block.remaining());

    block.pos_inc();
    EXPECT_TRUE(block.has_remaining());
    EXPECT_EQ(97, block.remaining());

    block.set_block_status(DEL_SATISFIED);
    EXPECT_EQ(DEL_SATISFIED, block.block_status());
}
} // namespace doris
