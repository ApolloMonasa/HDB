/**
 * test/table/tuple_test.cpp —— Lab 2 Task 1 判卷用例（纯内存，不依赖缓冲池）
 */
#include "gtest/gtest.h"
#include "hdb/storage/table/column.h"
#include "hdb/storage/table/schema.h"
#include "hdb/storage/table/tuple.h"
#include "hdb/type/value.h"
#include "hdb/type/value_factory.h"

namespace hdb::test {

namespace {
Schema MakeTestSchema() {
    return Schema({Column("id", TypeId::INTEGER),
                   Column("name", TypeId::VARCHAR, 20),
                   Column("gpa", TypeId::FLOAT)});
}
}  // namespace

TEST(TupleTest, ConstructAndGetValue) {
    Schema schema = MakeTestSchema();
    Tuple tuple({ValueFactory::CreateInteger(42), ValueFactory::CreateVarchar("hdu"),
                 ValueFactory::CreateFloat(3.25)},
                schema);

    EXPECT_EQ(tuple.GetValue(schema, 0).GetAsInteger(), 42);
    EXPECT_EQ(tuple.GetValue(schema, 1).GetAsVarchar(), "hdu");
    EXPECT_DOUBLE_EQ(tuple.GetValue(schema, 2).GetAsFloat(), 3.25);
}

TEST(TupleTest, SerializeDeserializeRoundtrip) {
    Schema schema = MakeTestSchema();
    Tuple tuple({ValueFactory::CreateInteger(7), ValueFactory::CreateVarchar("database"),
                 ValueFactory::CreateFloat(95.5)},
                schema);

    std::vector<char> buf(256);
    uint32_t written = tuple.SerializeTo(buf.data());
    // 定长布局：bitmap(1) + INT(4) + VARCHAR(20)槽(4+20) + FLOAT(8) = 37
    EXPECT_EQ(written, 1 + 4 + (4 + 20) + 8);

    Tuple back;
    back.DeserializeFrom(buf.data(), schema, RID(3, 1));
    EXPECT_EQ(back.GetRid(), RID(3, 1));
    EXPECT_EQ(back.GetValue(schema, 0).GetAsInteger(), 7);
    EXPECT_EQ(back.GetValue(schema, 1).GetAsVarchar(), "database");
    EXPECT_DOUBLE_EQ(back.GetValue(schema, 2).GetAsFloat(), 95.5);
}

TEST(TupleTest, NullValueRoundtrip) {
    Schema schema({Column("x", TypeId::INTEGER)});
    Tuple tuple({ValueFactory::CreateNull()}, schema);

    std::vector<char> buf(16);
    tuple.SerializeTo(buf.data());
    Tuple back;
    back.DeserializeFrom(buf.data(), schema, RID(0, 0));
    EXPECT_TRUE(back.GetValue(schema, 0).IsNull());
}

}  // namespace hdb::test
