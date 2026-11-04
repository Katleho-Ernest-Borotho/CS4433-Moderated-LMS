-- v1.1 additive migration; mirrored in SqliteDatabase.cpp
CREATE TABLE IF NOT EXISTS class_deadline_extensions (
 class_extension_id INTEGER PRIMARY KEY,
 assignment_id INTEGER NOT NULL UNIQUE REFERENCES assignments(assignment_id) ON DELETE RESTRICT,
 lecturer_id INTEGER NOT NULL REFERENCES users(user_id) ON DELETE RESTRICT,
 new_deadline_utc TEXT NOT NULL,
 created_at_utc TEXT NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_class_extensions_lecturer ON class_deadline_extensions(lecturer_id);
