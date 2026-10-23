#pragma once

#include <sqlite3.h>
#include <string>

namespace lms {

class SqliteDatabase {
public:
    explicit SqliteDatabase(const std::string& filePath);
    ~SqliteDatabase();

    SqliteDatabase(const SqliteDatabase&) = delete;
    SqliteDatabase& operator=(const SqliteDatabase&) = delete;

    bool foreignKeysEnabled() const;
    void initializeSchemaFromFile(const std::string& schemaFile);

private:
    sqlite3* connection_ = nullptr;
};

} // namespace lms
