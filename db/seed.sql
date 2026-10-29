-- BorrowHub Seed Data
-- 1 Admin, 3 Students, 6 Items (2 Book, 2 Electronic, 2 Equipment), 1 Chat Thread
-- All deposits respect the type-based minimums defined in Blueprint 3.4

-- 1 Admin and 3 Students
-- Passwords are demo Argon2id / hash values for 'Password123!'
INSERT OR IGNORE INTO users (id, email, prn, password_hash, name, phone, department, year, division, role, verified, blocked, reputation, created_at)
VALUES
(1, 'admin@campus.edu', NULL, '$argon2id$v=19$m=65536,t=2,p=1$c29tZXNhbHRmb3JkZW1v$5u2kFkI6r+6xO2hB1b1eC6p8p8W0A7f+Y5q9v2kFkI4', 'Campus Administrator', '+919876543210', 'Administration', NULL, NULL, 'ADMIN', 1, 0, 100.0, 1730000000),
(2, 'aarav@campus.edu', 'PRN2024001', '$argon2id$v=19$m=65536,t=2,p=1$c29tZXNhbHRmb3JkZW1v$5u2kFkI6r+6xO2hB1b1eC6p8p8W0A7f+Y5q9v2kFkI4', 'Aarav Patel', '+919811122233', 'Computer Science', 3, 'A', 'STUDENT', 1, 0, 62.0, 1730000100),
(3, 'priya@campus.edu', 'PRN2024002', '$argon2id$v=19$m=65536,t=2,p=1$c29tZXNhbHRmb3JkZW1v$5u2kFkI6r+6xO2hB1b1eC6p8p8W0A7f+Y5q9v2kFkI4', 'Priya Sharma', '+919822233344', 'Electronics', 2, 'B', 'STUDENT', 1, 0, 62.0, 1730000200),
(4, 'rohan@campus.edu', 'PRN2024003', '$argon2id$v=19$m=65536,t=2,p=1$c29tZXNhbHRmb3JkZW1v$5u2kFkI6r+6xO2hB1b1eC6p8p8W0A7f+Y5q9v2kFkI4', 'Rohan Verma', '+919833344455', 'Mechanical', 4, 'A', 'STUDENT', 1, 0, 62.0, 1730000300);

-- 6 Items: 2 Books (min 100 INR = 10000 paise), 2 Electronics (min 500 INR = 50000 paise), 2 Equipments (min 200 INR = 20000 paise)
INSERT OR IGNORE INTO items (id, owner_id, type, title, category, item_condition, location, deposit_paise, avail_from, avail_to, attrs_json, status, created_at)
VALUES
(1, 2, 'BOOK', 'Introduction to Algorithms (CLRS 3rd Ed)', 'Academics', 'GOOD', 'Hostel 3, Room 214', 15000, 1730000000, 1761500000, '{"author":"Cormen, Leiserson, Rivest, Stein","isbn":"9780262033848"}', 'AVAILABLE', 1730000400),
(2, 3, 'BOOK', 'Design Patterns: Elements of Reusable Object-Oriented Software', 'Computer Science', 'GOOD', 'Library Study Hall', 12000, 1730000000, 1761500000, '{"author":"Gang of Four","isbn":"9780201633610"}', 'AVAILABLE', 1730000500),
(3, 2, 'ELECTRONIC', 'Arduino Mega 2560 Sensor & Starter Kit', 'Robotics Lab', 'GOOD', 'Tech Hub Lab 2', 65000, 1730000000, 1761500000, '{"brand":"Arduino","model":"Mega 2560 R3"}', 'AVAILABLE', 1730000600),
(4, 4, 'ELECTRONIC', 'Texas Instruments TI-84 Plus CE Graphing Calculator', 'Calculators', 'GOOD', 'Hostel 1, Room 108', 55000, 1730000000, 1761500000, '{"brand":"Texas Instruments","model":"TI-84 Plus CE"}', 'AVAILABLE', 1730000700),
(5, 3, 'EQUIPMENT', 'Digital Multimeter with Auto-Ranging & Probes', 'Lab Equipment', 'GOOD', 'EE Department Locker 12', 25000, 1730000000, 1761500000, '{"equipmentType":"Multimeter","brand":"Mastech"}', 'AVAILABLE', 1730000800),
(6, 4, 'EQUIPMENT', 'Rotring Architectural Drafting & Drafting Board Set', 'Architecture', 'GOOD', 'Design Studio B', 30000, 1730000000, 1761500000, '{"equipmentType":"Drafting Board","brand":"Rotring"}', 'AVAILABLE', 1730000900);

-- 1 Initial Chat Thread between Priya (borrower) and Aarav (owner of Arduino Kit item 3)
INSERT OR IGNORE INTO chat_threads (id, item_id, borrower_id)
VALUES (1, 3, 3);

-- Initial messages in thread
INSERT OR IGNORE INTO messages (id, thread_id, sender_id, body, created_at)
VALUES
(1, 1, 3, 'Hi Aarav, is the Arduino Mega kit available for the robotics hackathon this weekend?', 1730001000),
(2, 1, 2, 'Hey Priya! Yes it is. It includes the ultrasonic sensors and servo motors as well.', 1730001050);
