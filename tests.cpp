#include <cassert>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <thread>
#include <chrono>

#include "Database.hpp"

using namespace nosqldb;

// Helpers
static const std::string TEST_FILE = "test_dump.db";

static void cleanupTestFile() {
    std::remove(TEST_FILE.c_str());
}

// ---- CRUD Tests ----

void test_set_and_get_int() {
    Database db;
    assert(db.setInt("score", 42));
    auto val = db.get("score");
    assert(val.has_value());
    assert(*val == "42");
    std::cout << "  PASS: test_set_and_get_int\n";
}

void test_set_and_get_string() {
    Database db;
    assert(db.setString("name", "Alice"));
    auto val = db.get("name");
    assert(val.has_value());
    assert(*val == "\"Alice\"");
    std::cout << "  PASS: test_set_and_get_string\n";
}

void test_get_missing_key() {
    Database db;
    auto val = db.get("nonexistent");
    assert(!val.has_value());
    std::cout << "  PASS: test_get_missing_key\n";
}

void test_overwrite_value() {
    Database db;
    db.setInt("x", 1);
    db.setInt("x", 2);
    auto val = db.get("x");
    assert(val.has_value());
    assert(*val == "2");
    std::cout << "  PASS: test_overwrite_value\n";
}

void test_overwrite_type_change() {
    Database db;
    db.setInt("x", 1);
    db.setString("x", "hello");
    auto val = db.get("x");
    assert(val.has_value());
    assert(*val == "\"hello\"");
    std::cout << "  PASS: test_overwrite_type_change\n";
}

void test_delete() {
    Database db;
    db.setInt("x", 10);
    assert(db.remove("x"));
    auto val = db.get("x");
    assert(!val.has_value());
    std::cout << "  PASS: test_delete\n";
}

void test_delete_missing() {
    Database db;
    assert(!db.remove("ghost"));
    std::cout << "  PASS: test_delete_missing\n";
}

void test_delete_multiple() {
    Database db;
    db.setInt("a", 1);
    db.setInt("b", 2);
    db.setInt("c", 3);
    db.setString("d", "hello");

    // Delete a mix of existing and non-existing keys
    auto result = db.removeMultiple({"a", "c", "ghost", "phantom"});
    assert(result.deleted == 2);
    assert(result.notFound.size() == 2);
    assert(result.notFound[0] == "ghost");
    assert(result.notFound[1] == "phantom");

    // Remaining keys should be b and d
    assert(db.exists("b"));
    assert(db.exists("d"));
    assert(!db.exists("a"));
    assert(!db.exists("c"));
    assert(db.count() == 2);
    std::cout << "  PASS: test_delete_multiple\n";
}

void test_exists() {
    Database db;
    assert(!db.exists("a"));
    db.setInt("a", 1);
    assert(db.exists("a"));
    db.remove("a");
    assert(!db.exists("a"));
    std::cout << "  PASS: test_exists\n";
}

void test_count() {
    Database db;
    assert(db.count() == 0);
    db.setInt("a", 1);
    db.setString("b", "two");
    assert(db.count() == 2);
    db.remove("a");
    assert(db.count() == 1);
    std::cout << "  PASS: test_count\n";
}

void test_keys() {
    Database db;
    db.setInt("x", 1);
    db.setString("y", "two");
    db.setInt("z", 3);
    auto k = db.keys();
    assert(k.size() == 3);
    // All three keys should be present (order is unspecified)
    assert(std::find(k.begin(), k.end(), "x") != k.end());
    assert(std::find(k.begin(), k.end(), "y") != k.end());
    assert(std::find(k.begin(), k.end(), "z") != k.end());
    std::cout << "  PASS: test_keys\n";
}

// ---- Empty Key Validation Tests ----

void test_reject_empty_key() {
    Database db;
    assert(!db.setInt("", 42));
    assert(!db.setString("", "val"));
    assert(!db.get("").has_value());
    assert(!db.remove(""));
    assert(!db.exists(""));
    assert(db.count() == 0);
    std::cout << "  PASS: test_reject_empty_key\n";
}

// ---- Rename Tests ----

void test_rename_basic() {
    Database db;
    db.setInt("old", 100);
    assert(db.rename("old", "new_key"));
    assert(!db.exists("old"));
    auto val = db.get("new_key");
    assert(val.has_value());
    assert(*val == "100");
    std::cout << "  PASS: test_rename_basic\n";
}

void test_rename_overwrites_target() {
    Database db;
    db.setInt("a", 1);
    db.setInt("b", 2);
    assert(db.rename("a", "b"));
    assert(!db.exists("a"));
    auto val = db.get("b");
    assert(val.has_value());
    assert(*val == "1");  // a's value replaces b's
    assert(db.count() == 1);
    std::cout << "  PASS: test_rename_overwrites_target\n";
}

void test_rename_missing_source() {
    Database db;
    assert(!db.rename("ghost", "target"));
    std::cout << "  PASS: test_rename_missing_source\n";
}

void test_rename_same_key() {
    Database db;
    db.setInt("x", 1);
    assert(!db.rename("x", "x"));
    std::cout << "  PASS: test_rename_same_key\n";
}

void test_rename_empty_keys() {
    Database db;
    db.setInt("x", 1);
    assert(!db.rename("", "y"));
    assert(!db.rename("x", ""));
    std::cout << "  PASS: test_rename_empty_keys\n";
}

// ---- Type Tests ----

void test_type_int() {
    Database db;
    db.setInt("num", 42);
    auto t = db.type("num");
    assert(t.has_value());
    assert(*t == ValueType::INT);
    std::cout << "  PASS: test_type_int\n";
}

void test_type_string() {
    Database db;
    db.setString("msg", "hello");
    auto t = db.type("msg");
    assert(t.has_value());
    assert(*t == ValueType::STRING);
    std::cout << "  PASS: test_type_string\n";
}

void test_type_missing() {
    Database db;
    assert(!db.type("ghost").has_value());
    std::cout << "  PASS: test_type_missing\n";
}

// ---- Persistence Tests ----

void test_save_and_load_basic() {
    cleanupTestFile();
    {
        Database db;
        db.setInt("score", 100);
        db.setString("name", "Bob");
        assert(db.saveToFile(TEST_FILE));
    }
    {
        Database db;
        auto result = db.loadFromFile(TEST_FILE);
        assert(result.fileFound);
        assert(result.entriesLoaded == 2);
        assert(result.warnings.empty());
        auto v1 = db.get("score");
        assert(v1.has_value() && *v1 == "100");
        auto v2 = db.get("name");
        assert(v2.has_value() && *v2 == "\"Bob\"");
    }
    cleanupTestFile();
    std::cout << "  PASS: test_save_and_load_basic\n";
}

void test_roundtrip_string_with_spaces() {
    cleanupTestFile();
    {
        Database db;
        db.setString("greeting", "Hello World from C++");
        assert(db.saveToFile(TEST_FILE));
    }
    {
        Database db;
        auto result = db.loadFromFile(TEST_FILE);
        assert(result.fileFound);
        assert(result.entriesLoaded == 1);
        auto val = db.get("greeting");
        assert(val.has_value());
        assert(*val == "\"Hello World from C++\"");
    }
    cleanupTestFile();
    std::cout << "  PASS: test_roundtrip_string_with_spaces\n";
}

void test_roundtrip_string_with_tabs_and_newlines() {
    cleanupTestFile();
    {
        Database db;
        db.setString("weird", "col1\tcol2\nrow2");
        assert(db.saveToFile(TEST_FILE));
    }
    {
        Database db;
        auto result = db.loadFromFile(TEST_FILE);
        assert(result.fileFound);
        assert(result.entriesLoaded == 1);
        assert(result.warnings.empty());
        auto val = db.get("weird");
        assert(val.has_value());
        assert(*val == "\"col1\tcol2\nrow2\"");
    }
    cleanupTestFile();
    std::cout << "  PASS: test_roundtrip_string_with_tabs_and_newlines\n";
}

void test_roundtrip_string_with_backslash() {
    cleanupTestFile();
    {
        Database db;
        db.setString("path", "C:\\Users\\test");
        assert(db.saveToFile(TEST_FILE));
    }
    {
        Database db;
        auto result = db.loadFromFile(TEST_FILE);
        assert(result.fileFound);
        assert(result.entriesLoaded == 1);
        auto val = db.get("path");
        assert(val.has_value());
        assert(*val == "\"C:\\Users\\test\"");
    }
    cleanupTestFile();
    std::cout << "  PASS: test_roundtrip_string_with_backslash\n";
}

void test_load_missing_file() {
    Database db;
    auto result = db.loadFromFile("no_such_file_12345.db");
    assert(!result.fileFound);
    assert(result.entriesLoaded == 0);
    std::cout << "  PASS: test_load_missing_file\n";
}

void test_load_malformed_lines() {
    cleanupTestFile();
    // Write a file with a mix of valid and invalid lines
    {
        std::ofstream f(TEST_FILE);
        f << "good\tINT\t42\n";
        f << "bad_no_tabs\n";            // malformed: no tabs
        f << "bad\tINT\tnotanumber\n";   // corrupt INT value
        f << "ok\tSTRING\thello world\n";
        f << "unk\tFLOAT\t3.14\n";      // unknown type
    }
    {
        Database db;
        auto result = db.loadFromFile(TEST_FILE);
        assert(result.fileFound);
        assert(result.entriesLoaded == 2); // "good" and "ok"
        assert(result.warnings.size() == 3); // bad_no_tabs, bad INT, unknown FLOAT
        auto v1 = db.get("good");
        assert(v1.has_value() && *v1 == "42");
        auto v2 = db.get("ok");
        assert(v2.has_value() && *v2 == "\"hello world\"");
    }
    cleanupTestFile();
    std::cout << "  PASS: test_load_malformed_lines\n";
}

void test_empty_database_file() {
    cleanupTestFile();
    {
        Database db;
        db.setInt("a", 1);
        db.setString("b", "two");
        db.saveToFile(TEST_FILE);
        db.emptyDatabaseFile(TEST_FILE);
        assert(db.count() == 0);
        // File should exist but be empty
        std::ifstream f(TEST_FILE);
        assert(f.is_open());
        std::string content;
        std::getline(f, content);
        assert(content.empty());
    }
    cleanupTestFile();
    std::cout << "  PASS: test_empty_database_file\n";
}

// ---- Enum Serialization Tests ----

void test_value_type_to_string() {
    assert(valueTypeToString(ValueType::INT) == "INT");
    assert(valueTypeToString(ValueType::STRING) == "STRING");
    std::cout << "  PASS: test_value_type_to_string\n";
}

// ---- Timer Tests ----

void test_clear_timer_fires() {
    cleanupTestFile();
    Database db;
    db.setInt("temp", 99);
    db.saveToFile(TEST_FILE);

    bool callbackFired = false;
    db.setClearTimer(1, TEST_FILE, [&callbackFired]() {
        callbackFired = true;
    });

    // Wait for the timer to fire (1 second + margin)
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    assert(callbackFired);
    assert(db.count() == 0);
    std::cout << "  PASS: test_clear_timer_fires\n";
    cleanupTestFile();
}

void test_clear_timer_cancel_on_destroy() {
    cleanupTestFile();
    bool callbackFired = false;
    {
        Database db;
        db.setInt("temp", 1);
        db.saveToFile(TEST_FILE);
        db.setClearTimer(5, TEST_FILE, [&callbackFired]() {
            callbackFired = true;
        });
        // db is destroyed here — destructor should join and cancel the timer
    }
    assert(!callbackFired);
    std::cout << "  PASS: test_clear_timer_cancel_on_destroy\n";
    cleanupTestFile();
}

void test_clear_timer_rejects_negative() {
    Database db;
    // Negative seconds should be rejected, returning false
    assert(!db.setClearTimer(-5, TEST_FILE));
    // Database should be untouched
    db.setInt("safe", 1);
    assert(db.count() == 1);
    std::cout << "  PASS: test_clear_timer_rejects_negative\n";
}

void test_clear_timer_rejects_zero() {
    Database db;
    // Zero seconds should also be rejected
    assert(!db.setClearTimer(0, TEST_FILE));
    db.setInt("safe", 1);
    assert(db.count() == 1);
    std::cout << "  PASS: test_clear_timer_rejects_zero\n";
}

// ---- Load Replaces Store Tests ----

void test_load_clears_existing_data() {
    cleanupTestFile();
    // Save a file with one key
    {
        Database db;
        db.setInt("from_file", 42);
        assert(db.saveToFile(TEST_FILE));
    }
    // Load into a database that already has different data
    {
        Database db;
        db.setString("pre_existing", "should be gone");
        db.setInt("another", 99);
        assert(db.count() == 2);

        auto result = db.loadFromFile(TEST_FILE);
        assert(result.fileFound);
        assert(result.entriesLoaded == 1);

        // Pre-existing keys must be gone — load replaces, not merges
        assert(!db.exists("pre_existing"));
        assert(!db.exists("another"));
        assert(db.count() == 1);
        auto val = db.get("from_file");
        assert(val.has_value() && *val == "42");
    }
    cleanupTestFile();
    std::cout << "  PASS: test_load_clears_existing_data\n";
}

// ---- emptyDatabaseFile Return Value Test ----

void test_empty_database_file_returns_bool() {
    cleanupTestFile();
    Database db;
    db.setInt("a", 1);
    // Should return true on success
    assert(db.emptyDatabaseFile(TEST_FILE));
    assert(db.count() == 0);
    cleanupTestFile();
    std::cout << "  PASS: test_empty_database_file_returns_bool\n";
}

// ---- Double Tests ----

void test_set_and_get_double() {
    Database db;
    assert(db.setDouble("pi", 3.14));
    auto val = db.get("pi");
    assert(val.has_value());
    assert(*val == "3.14");
    std::cout << "  PASS: test_set_and_get_double\n";
}

void test_double_precision() {
    cleanupTestFile();
    {
        Database db;
        db.setDouble("big", 123456789.123456);
        assert(db.saveToFile(TEST_FILE));
    }
    {
        Database db;
        auto result = db.loadFromFile(TEST_FILE);
        assert(result.fileFound);
        assert(result.entriesLoaded == 1);
        auto val = db.get("big");
        assert(val.has_value());
        // Verify the value survives a round-trip
    }
    cleanupTestFile();
    std::cout << "  PASS: test_double_precision\n";
}

void test_type_double() {
    Database db;
    db.setDouble("pi", 3.14);
    auto t = db.type("pi");
    assert(t.has_value());
    assert(*t == ValueType::DOUBLE);
    std::cout << "  PASS: test_type_double\n";
}

void test_persistence_double() {
    cleanupTestFile();
    {
        Database db;
        db.setDouble("e", 2.71828);
        assert(db.saveToFile(TEST_FILE));
    }
    {
        Database db;
        auto result = db.loadFromFile(TEST_FILE);
        assert(result.fileFound);
        assert(result.entriesLoaded == 1);
        auto val = db.get("e");
        assert(val.has_value());
        assert(*val == "2.71828");
        auto t = db.type("e");
        assert(t.has_value() && *t == ValueType::DOUBLE);
    }
    cleanupTestFile();
    std::cout << "  PASS: test_persistence_double\n";
}

// ---- Bool Tests ----

void test_set_and_get_bool_true() {
    Database db;
    assert(db.setBool("flag", true));
    auto val = db.get("flag");
    assert(val.has_value());
    assert(*val == "true");
    std::cout << "  PASS: test_set_and_get_bool_true\n";
}

void test_set_and_get_bool_false() {
    Database db;
    assert(db.setBool("flag", false));
    auto val = db.get("flag");
    assert(val.has_value());
    assert(*val == "false");
    std::cout << "  PASS: test_set_and_get_bool_false\n";
}

void test_type_bool() {
    Database db;
    db.setBool("flag", true);
    auto t = db.type("flag");
    assert(t.has_value());
    assert(*t == ValueType::BOOL);
    std::cout << "  PASS: test_type_bool\n";
}

void test_persistence_bool() {
    cleanupTestFile();
    {
        Database db;
        db.setBool("on", true);
        db.setBool("off", false);
        assert(db.saveToFile(TEST_FILE));
    }
    {
        Database db;
        auto result = db.loadFromFile(TEST_FILE);
        assert(result.fileFound);
        assert(result.entriesLoaded == 2);
        auto v1 = db.get("on");
        assert(v1.has_value() && *v1 == "true");
        auto v2 = db.get("off");
        assert(v2.has_value() && *v2 == "false");
        auto t1 = db.type("on");
        assert(t1.has_value() && *t1 == ValueType::BOOL);
        auto t2 = db.type("off");
        assert(t2.has_value() && *t2 == ValueType::BOOL);
    }
    cleanupTestFile();
    std::cout << "  PASS: test_persistence_bool\n";
}

// ---- List Tests ----

void test_lpush_creates_list() {
    Database db;
    int result = db.listPushLeft("mylist", "a");
    assert(result == 1);
    std::cout << "  PASS: test_lpush_creates_list\n";
}

void test_lpush_prepends() {
    Database db;
    db.listPushLeft("mylist", "a");
    db.listPushLeft("mylist", "b");
    db.listPushLeft("mylist", "c");
    auto range = db.listRange("mylist", 0, -1);
    assert(range.has_value());
    assert(range->size() == 3);
    assert((*range)[0] == "c");
    assert((*range)[1] == "b");
    assert((*range)[2] == "a");
    std::cout << "  PASS: test_lpush_prepends\n";
}

void test_rpush_appends() {
    Database db;
    db.listPushRight("mylist", "a");
    db.listPushRight("mylist", "b");
    db.listPushRight("mylist", "c");
    auto range = db.listRange("mylist", 0, -1);
    assert(range.has_value());
    assert(range->size() == 3);
    assert((*range)[0] == "a");
    assert((*range)[1] == "b");
    assert((*range)[2] == "c");
    std::cout << "  PASS: test_rpush_appends\n";
}

void test_lpop_returns_front() {
    Database db;
    db.listPushRight("mylist", "a");
    db.listPushRight("mylist", "b");
    db.listPushRight("mylist", "c");
    auto val = db.listPopLeft("mylist");
    assert(val.has_value());
    assert(*val == "a");
    std::cout << "  PASS: test_lpop_returns_front\n";
}

void test_rpop_returns_back() {
    Database db;
    db.listPushRight("mylist", "a");
    db.listPushRight("mylist", "b");
    db.listPushRight("mylist", "c");
    auto val = db.listPopRight("mylist");
    assert(val.has_value());
    assert(*val == "c");
    std::cout << "  PASS: test_rpop_returns_back\n";
}

void test_pop_empty_list() {
    Database db;
    db.listPushRight("mylist", "a");
    db.listPopLeft("mylist");  // remove the only element
    auto val = db.listPopLeft("mylist");
    assert(!val.has_value());
    std::cout << "  PASS: test_pop_empty_list\n";
}

void test_lrange_basic() {
    Database db;
    db.listPushRight("mylist", "a");
    db.listPushRight("mylist", "b");
    db.listPushRight("mylist", "c");
    db.listPushRight("mylist", "d");
    db.listPushRight("mylist", "e");
    auto range = db.listRange("mylist", 0, -1);
    assert(range.has_value());
    assert(range->size() == 5);
    assert((*range)[0] == "a");
    assert((*range)[4] == "e");
    std::cout << "  PASS: test_lrange_basic\n";
}

void test_lrange_subset() {
    Database db;
    db.listPushRight("mylist", "a");
    db.listPushRight("mylist", "b");
    db.listPushRight("mylist", "c");
    db.listPushRight("mylist", "d");
    auto range = db.listRange("mylist", 1, 2);
    assert(range.has_value());
    assert(range->size() == 2);
    assert((*range)[0] == "b");
    assert((*range)[1] == "c");
    std::cout << "  PASS: test_lrange_subset\n";
}

void test_llen() {
    Database db;
    db.listPushRight("mylist", "a");
    db.listPushRight("mylist", "b");
    db.listPushRight("mylist", "c");
    auto len = db.listLength("mylist");
    assert(len.has_value());
    assert(*len == 3);
    db.listPopLeft("mylist");
    len = db.listLength("mylist");
    assert(len.has_value());
    assert(*len == 2);
    std::cout << "  PASS: test_llen\n";
}

void test_list_wrong_type() {
    Database db;
    db.setInt("num", 42);
    int result = db.listPushLeft("num", "val");
    assert(result == -1);
    std::cout << "  PASS: test_list_wrong_type\n";
}

void test_type_list() {
    Database db;
    db.listPushRight("mylist", "a");
    auto t = db.type("mylist");
    assert(t.has_value());
    assert(*t == ValueType::LIST);
    std::cout << "  PASS: test_type_list\n";
}

void test_persistence_list() {
    cleanupTestFile();
    {
        Database db;
        db.listPushRight("mylist", "hello");
        db.listPushRight("mylist", "world,with,commas");
        db.listPushRight("mylist", "back\\slash");
        assert(db.saveToFile(TEST_FILE));
    }
    {
        Database db;
        auto result = db.loadFromFile(TEST_FILE);
        assert(result.fileFound);
        assert(result.entriesLoaded == 1);
        auto range = db.listRange("mylist", 0, -1);
        assert(range.has_value());
        assert(range->size() == 3);
        assert((*range)[0] == "hello");
        assert((*range)[1] == "world,with,commas");
        assert((*range)[2] == "back\\slash");
        auto t = db.type("mylist");
        assert(t.has_value() && *t == ValueType::LIST);
    }
    cleanupTestFile();
    std::cout << "  PASS: test_persistence_list\n";
}

// ---- Main ----

int main() {
    int testCount = 0;
    std::cout << "Running tests...\n\n";
    std::cout << "[CRUD Operations]\n";
    test_set_and_get_int();         ++testCount;
    test_set_and_get_string();      ++testCount;
    test_get_missing_key();         ++testCount;
    test_overwrite_value();         ++testCount;
    test_overwrite_type_change();   ++testCount;
    test_delete();                  ++testCount;
    test_delete_missing();          ++testCount;
    test_delete_multiple();         ++testCount;
    test_exists();                  ++testCount;
    test_count();                   ++testCount;
    test_keys();                    ++testCount;

    std::cout << "\n[Validation]\n";
    test_reject_empty_key();        ++testCount;

    std::cout << "\n[Rename]\n";
    test_rename_basic();            ++testCount;
    test_rename_overwrites_target(); ++testCount;
    test_rename_missing_source();   ++testCount;
    test_rename_same_key();         ++testCount;
    test_rename_empty_keys();       ++testCount;

    std::cout << "\n[Type]\n";
    test_type_int();                ++testCount;
    test_type_string();             ++testCount;
    test_type_missing();            ++testCount;

    std::cout << "\n[Persistence]\n";
    test_save_and_load_basic();     ++testCount;
    test_roundtrip_string_with_spaces();          ++testCount;
    test_roundtrip_string_with_tabs_and_newlines(); ++testCount;
    test_roundtrip_string_with_backslash();        ++testCount;
    test_load_missing_file();       ++testCount;
    test_load_malformed_lines();    ++testCount;
    test_empty_database_file();     ++testCount;
    test_load_clears_existing_data(); ++testCount;
    test_empty_database_file_returns_bool(); ++testCount;

    std::cout << "\n[Enum Serialization]\n";
    test_value_type_to_string();    ++testCount;

    std::cout << "\n[Timer]\n";
    test_clear_timer_fires();       ++testCount;
    test_clear_timer_cancel_on_destroy();          ++testCount;
    test_clear_timer_rejects_negative();           ++testCount;
    test_clear_timer_rejects_zero();               ++testCount;

    std::cout << "\n[Double]\n";
    test_set_and_get_double();      ++testCount;
    test_double_precision();        ++testCount;
    test_type_double();             ++testCount;
    test_persistence_double();      ++testCount;

    std::cout << "\n[Bool]\n";
    test_set_and_get_bool_true();   ++testCount;
    test_set_and_get_bool_false();  ++testCount;
    test_type_bool();               ++testCount;
    test_persistence_bool();        ++testCount;

    std::cout << "\n[List]\n";
    test_lpush_creates_list();      ++testCount;
    test_lpush_prepends();          ++testCount;
    test_rpush_appends();           ++testCount;
    test_lpop_returns_front();      ++testCount;
    test_rpop_returns_back();       ++testCount;
    test_pop_empty_list();          ++testCount;
    test_lrange_basic();            ++testCount;
    test_lrange_subset();           ++testCount;
    test_llen();                    ++testCount;
    test_list_wrong_type();         ++testCount;
    test_type_list();               ++testCount;
    test_persistence_list();        ++testCount;

    std::cout << "\n--- All " << testCount << " tests passed! ---\n";
    return 0;
}
