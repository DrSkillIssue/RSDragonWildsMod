#include "fake/Fixture.hpp"

namespace
{
struct Containers : Fixture
{
    UObject* bucket;

    void SetUp() override
    {
        Fake::Object item_type(L"HeldContainerEquipmentData");
        Fake::field(reinterpret_cast<UStruct*>(item_type.pointer), L"Capacity", L"IntProperty", 0, 4);
        Fake::field(reinterpret_cast<UStruct*>(item_type.pointer), L"AmountDischargedPerUse", L"IntProperty", 4, 4);
        Fake::Object object(L"ITEM_Bucket_Compost", reinterpret_cast<UClass*>(item_type.pointer));
        bucket = object.pointer;
        Fake::records.at(bucket).collectable = true;
        member(bucket, L"Capacity").write<int>(L"IntProperty", 150);
        member(bucket, L"AmountDischargedPerUse").write<int>(L"IntProperty", 10);
        start();
        Fake::game.asset_loads = 0;
        settings.containers.items = { { "CompostBucket", 500, 50 } };
    }

    void load() { Fake::game.paths.emplace(known_containers[0].asset, bucket); }
    int capacity() { return member(bucket, L"Capacity").read<int>(L"IntProperty"); }
    int per_use() { return member(bucket, L"AmountDischargedPerUse").read<int>(L"IntProperty"); }
    int roots() { return Fake::records.at(bucket).roots; }
};

TEST_F(Containers, loads_roots_and_writes_the_values_on_configure)
{
    load();
    const auto adds = Fake::game.adds, widgets = Fake::game.constructions;
    configure();
    ASSERT_TRUE(capacity() == 500 && per_use() == 50 && roots() == 1 && Fake::game.asset_loads == 1);
    ASSERT_TRUE(Fake::game.adds == adds && Fake::game.constructions == widgets && Fake::game.log.empty());
    Fake::collect();
    ASSERT_TRUE(Fake::records.at(bucket).alive);
    Fake::time += 5000;
    idle(1000);
    ASSERT_TRUE(Fake::game.asset_loads == 1 && capacity() == 500);
}

TEST_F(Containers, applies_an_edit_and_logs_under_debug)
{
    load();
    configure();
    ASSERT_EQ(capacity(), 500);
    const auto adds = Fake::game.adds, widgets = Fake::game.constructions;
    settings.containers.items[0].capacity = 600;
    settings.debug = true;
    mod->configure(settings);
    Log::debug = false;
    ASSERT_TRUE(capacity() == 600 && roots() == 1 && Fake::game.asset_loads == 2);
    ASSERT_TRUE(Fake::logged("Container tweak applied: CompostBucket capacity 600, per use 50.") && Fake::logged("Configuration accepted.") && Fake::game.log.empty());
    ASSERT_TRUE(Fake::game.adds == adds && Fake::game.constructions == widgets);
    configure();
    ASSERT_TRUE(Fake::game.asset_loads == 2 && Fake::game.log.empty());
}

TEST_F(Containers, disabling_or_removing_un_roots_and_keeps_the_written_values)
{
    load();
    configure();
    ASSERT_TRUE(capacity() == 500 && roots() == 1);
    settings.containers.enabled = false;
    configure();
    ASSERT_TRUE(capacity() == 500 && roots() == 0 && Fake::game.log.empty());
    Fake::collect();
    ASSERT_FALSE(Fake::records.at(bucket).alive);
    Fake::records.at(bucket).alive = true;
    settings.containers.enabled = true;
    configure();
    ASSERT_TRUE(roots() == 1 && Fake::game.asset_loads == 2);
    settings.containers.items.clear();
    configure();
    ASSERT_TRUE(roots() == 0 && capacity() == 500 && Fake::game.log.empty());
}

TEST_F(Containers, logs_a_missing_item_once_and_does_not_retry)
{
    mod->configure(settings);
    ASSERT_TRUE(Fake::logged("Container item is not available: CompostBucket") && Fake::logged("Configuration accepted.") && Fake::game.log.empty());
    ASSERT_TRUE(capacity() == 150 && roots() == 0 && Fake::game.asset_loads == 1);
    load();
    Fake::time += 5000;
    idle(1000);
    configure();
    ASSERT_TRUE(capacity() == 150 && Fake::game.asset_loads == 1 && Fake::game.log.empty());
}

TEST_F(Containers, survives_the_controller_dying_and_un_roots_on_unload)
{
    load();
    configure();
    end_world();
    ASSERT_TRUE(roots() == 1 && Fake::logged("Spell bar released.") && Fake::game.log.empty());
    restart();
    tick();
    ASSERT_TRUE(roots() == 1 && capacity() == 500);
    mod.reset();
    ASSERT_EQ(roots(), 0);
}
}
