-- Categories for user 1
INSERT OR IGNORE INTO categories (name, color, icon, user_id) VALUES
  ('Salary',        '#22c55e', 'banknote',     1),
  ('Food',          '#f97316', 'utensils',     1),
  ('Rent',          '#6366f1', 'home',         1),
  ('Transport',     '#3b82f6', 'car',          1),
  ('Entertainment', '#ec4899', 'tv',           1),
  ('Healthcare',    '#14b8a6', 'heart-pulse',  1),
  ('Shopping',      '#f59e0b', 'shopping-bag', 1),
  ('Utilities',     '#78716c', 'zap',          1);

-- January 2026 transactions
INSERT INTO transactions (user_id, date, amount, type, category_id, note, recurring, created_at) VALUES
  (1, '2026-01-01', 3000.00, 'income',  (SELECT id FROM categories WHERE name='Salary'        AND user_id=1), 'Monthly salary',       1, '2026-01-01T00:00:00Z'),
  (1, '2026-01-02',  800.00, 'expense', (SELECT id FROM categories WHERE name='Rent'          AND user_id=1), 'January rent',         1, '2026-01-02T00:00:00Z'),
  (1, '2026-01-04',   85.50, 'expense', (SELECT id FROM categories WHERE name='Food'          AND user_id=1), 'Weekly groceries',     0, '2026-01-04T00:00:00Z'),
  (1, '2026-01-07',   32.00, 'expense', (SELECT id FROM categories WHERE name='Transport'     AND user_id=1), 'Monthly bus pass',     1, '2026-01-07T00:00:00Z'),
  (1, '2026-01-08',   45.00, 'expense', (SELECT id FROM categories WHERE name='Food'          AND user_id=1), 'Restaurant dinner',    0, '2026-01-08T00:00:00Z'),
  (1, '2026-01-10',  120.00, 'expense', (SELECT id FROM categories WHERE name='Utilities'     AND user_id=1), 'Electricity bill',     1, '2026-01-10T00:00:00Z'),
  (1, '2026-01-12',   92.00, 'expense', (SELECT id FROM categories WHERE name='Food'          AND user_id=1), 'Weekly groceries',     0, '2026-01-12T00:00:00Z'),
  (1, '2026-01-14',   58.00, 'expense', (SELECT id FROM categories WHERE name='Entertainment' AND user_id=1), 'Cinema + drinks',      0, '2026-01-14T00:00:00Z'),
  (1, '2026-01-16',  110.00, 'expense', (SELECT id FROM categories WHERE name='Transport'     AND user_id=1), 'Car fuel',             0, '2026-01-16T00:00:00Z'),
  (1, '2026-01-18',   78.00, 'expense', (SELECT id FROM categories WHERE name='Food'          AND user_id=1), 'Weekly groceries',     0, '2026-01-18T00:00:00Z'),
  (1, '2026-01-20',  200.00, 'expense', (SELECT id FROM categories WHERE name='Shopping'      AND user_id=1), 'Winter clothes',       0, '2026-01-20T00:00:00Z'),
  (1, '2026-01-22',   22.00, 'expense', (SELECT id FROM categories WHERE name='Entertainment' AND user_id=1), 'Streaming services',   1, '2026-01-22T00:00:00Z'),
  (1, '2026-01-25',   88.00, 'expense', (SELECT id FROM categories WHERE name='Food'          AND user_id=1), 'Weekly groceries',     0, '2026-01-25T00:00:00Z'),
  (1, '2026-01-28',   65.00, 'expense', (SELECT id FROM categories WHERE name='Shopping'      AND user_id=1), 'Home supplies',        0, '2026-01-28T00:00:00Z');

-- February 2026 transactions
INSERT INTO transactions (user_id, date, amount, type, category_id, note, recurring, created_at) VALUES
  (1, '2026-02-01', 3000.00, 'income',  (SELECT id FROM categories WHERE name='Salary'        AND user_id=1), 'Monthly salary',       1, '2026-02-01T00:00:00Z'),
  (1, '2026-02-02',  800.00, 'expense', (SELECT id FROM categories WHERE name='Rent'          AND user_id=1), 'February rent',        1, '2026-02-02T00:00:00Z'),
  (1, '2026-02-04',   91.00, 'expense', (SELECT id FROM categories WHERE name='Food'          AND user_id=1), 'Weekly groceries',     0, '2026-02-04T00:00:00Z'),
  (1, '2026-02-06',   32.00, 'expense', (SELECT id FROM categories WHERE name='Transport'     AND user_id=1), 'Monthly bus pass',     1, '2026-02-06T00:00:00Z'),
  (1, '2026-02-08',   55.00, 'expense', (SELECT id FROM categories WHERE name='Food'          AND user_id=1), 'Restaurant lunch',     0, '2026-02-08T00:00:00Z'),
  (1, '2026-02-10',  110.00, 'expense', (SELECT id FROM categories WHERE name='Utilities'     AND user_id=1), 'Electricity bill',     1, '2026-02-10T00:00:00Z'),
  (1, '2026-02-11',  200.00, 'expense', (SELECT id FROM categories WHERE name='Healthcare'    AND user_id=1), 'Dentist appointment',  0, '2026-02-11T00:00:00Z'),
  (1, '2026-02-13',   84.00, 'expense', (SELECT id FROM categories WHERE name='Food'          AND user_id=1), 'Weekly groceries',     0, '2026-02-13T00:00:00Z'),
  (1, '2026-02-15',   95.00, 'expense', (SELECT id FROM categories WHERE name='Transport'     AND user_id=1), 'Car fuel',             0, '2026-02-15T00:00:00Z'),
  (1, '2026-02-17',   22.00, 'expense', (SELECT id FROM categories WHERE name='Entertainment' AND user_id=1), 'Streaming services',   1, '2026-02-17T00:00:00Z'),
  (1, '2026-02-19',  150.00, 'expense', (SELECT id FROM categories WHERE name='Shopping'      AND user_id=1), 'Shoes',                0, '2026-02-19T00:00:00Z'),
  (1, '2026-02-21',   79.00, 'expense', (SELECT id FROM categories WHERE name='Food'          AND user_id=1), 'Weekly groceries',     0, '2026-02-21T00:00:00Z'),
  (1, '2026-02-24',   42.00, 'expense', (SELECT id FROM categories WHERE name='Entertainment' AND user_id=1), 'Board game night',     0, '2026-02-24T00:00:00Z'),
  (1, '2026-02-26',   30.00, 'expense', (SELECT id FROM categories WHERE name='Healthcare'    AND user_id=1), 'Pharmacy',             0, '2026-02-26T00:00:00Z');

-- March 2026 transactions
INSERT INTO transactions (user_id, date, amount, type, category_id, note, recurring, created_at) VALUES
  (1, '2026-03-01', 3000.00, 'income',  (SELECT id FROM categories WHERE name='Salary'        AND user_id=1), 'Monthly salary',       1, '2026-03-01T00:00:00Z'),
  (1, '2026-03-02',  800.00, 'expense', (SELECT id FROM categories WHERE name='Rent'          AND user_id=1), 'March rent',           1, '2026-03-02T00:00:00Z'),
  (1, '2026-03-03',   95.00, 'expense', (SELECT id FROM categories WHERE name='Food'          AND user_id=1), 'Weekly groceries',     0, '2026-03-03T00:00:00Z'),
  (1, '2026-03-04',   32.00, 'expense', (SELECT id FROM categories WHERE name='Transport'     AND user_id=1), 'Monthly bus pass',     1, '2026-03-04T00:00:00Z'),
  (1, '2026-03-06',   48.00, 'expense', (SELECT id FROM categories WHERE name='Food'          AND user_id=1), 'Takeaway',             0, '2026-03-06T00:00:00Z'),
  (1, '2026-03-08',  115.00, 'expense', (SELECT id FROM categories WHERE name='Utilities'     AND user_id=1), 'Electricity bill',     1, '2026-03-08T00:00:00Z'),
  (1, '2026-03-09',  125.00, 'expense', (SELECT id FROM categories WHERE name='Transport'     AND user_id=1), 'Car fuel',             0, '2026-03-09T00:00:00Z'),
  (1, '2026-03-11',   88.00, 'expense', (SELECT id FROM categories WHERE name='Food'          AND user_id=1), 'Weekly groceries',     0, '2026-03-11T00:00:00Z'),
  (1, '2026-03-13',   22.00, 'expense', (SELECT id FROM categories WHERE name='Entertainment' AND user_id=1), 'Streaming services',   1, '2026-03-13T00:00:00Z'),
  (1, '2026-03-14',  180.00, 'expense', (SELECT id FROM categories WHERE name='Shopping'      AND user_id=1), 'Spring clothes',       0, '2026-03-14T00:00:00Z'),
  (1, '2026-03-15',   75.00, 'expense', (SELECT id FROM categories WHERE name='Food'          AND user_id=1), 'Weekly groceries',     0, '2026-03-15T00:00:00Z'),
  (1, '2026-03-17',  100.00, 'expense', (SELECT id FROM categories WHERE name='Entertainment' AND user_id=1), 'Concert tickets',      0, '2026-03-17T00:00:00Z'),
  (1, '2026-03-20',   60.00, 'expense', (SELECT id FROM categories WHERE name='Healthcare'    AND user_id=1), 'Gym membership',       1, '2026-03-20T00:00:00Z');

-- Budgets for all 3 months
INSERT OR REPLACE INTO budgets (user_id, year_month, category_id, amount) VALUES
  (1, '2026-01', (SELECT id FROM categories WHERE name='Food'          AND user_id=1), 400.00),
  (1, '2026-01', (SELECT id FROM categories WHERE name='Rent'          AND user_id=1), 800.00),
  (1, '2026-01', (SELECT id FROM categories WHERE name='Transport'     AND user_id=1), 200.00),
  (1, '2026-01', (SELECT id FROM categories WHERE name='Entertainment' AND user_id=1), 100.00),
  (1, '2026-01', (SELECT id FROM categories WHERE name='Shopping'      AND user_id=1), 200.00),
  (1, '2026-01', (SELECT id FROM categories WHERE name='Utilities'     AND user_id=1), 130.00),
  (1, '2026-01', (SELECT id FROM categories WHERE name='Healthcare'    AND user_id=1), 100.00),

  (1, '2026-02', (SELECT id FROM categories WHERE name='Food'          AND user_id=1), 400.00),
  (1, '2026-02', (SELECT id FROM categories WHERE name='Rent'          AND user_id=1), 800.00),
  (1, '2026-02', (SELECT id FROM categories WHERE name='Transport'     AND user_id=1), 200.00),
  (1, '2026-02', (SELECT id FROM categories WHERE name='Entertainment' AND user_id=1), 100.00),
  (1, '2026-02', (SELECT id FROM categories WHERE name='Shopping'      AND user_id=1), 200.00),
  (1, '2026-02', (SELECT id FROM categories WHERE name='Utilities'     AND user_id=1), 130.00),
  (1, '2026-02', (SELECT id FROM categories WHERE name='Healthcare'    AND user_id=1), 100.00),

  (1, '2026-03', (SELECT id FROM categories WHERE name='Food'          AND user_id=1), 400.00),
  (1, '2026-03', (SELECT id FROM categories WHERE name='Rent'          AND user_id=1), 800.00),
  (1, '2026-03', (SELECT id FROM categories WHERE name='Transport'     AND user_id=1), 200.00),
  (1, '2026-03', (SELECT id FROM categories WHERE name='Entertainment' AND user_id=1), 100.00),
  (1, '2026-03', (SELECT id FROM categories WHERE name='Shopping'      AND user_id=1), 200.00),
  (1, '2026-03', (SELECT id FROM categories WHERE name='Utilities'     AND user_id=1), 130.00),
  (1, '2026-03', (SELECT id FROM categories WHERE name='Healthcare'    AND user_id=1), 100.00);

-- Update account balance (sum of all transactions)
UPDATE accounts SET balance = (
  SELECT COALESCE(SUM(CASE WHEN type='income' THEN amount ELSE -amount END), 0)
  FROM transactions WHERE user_id = 1
) WHERE user_id = 1;
