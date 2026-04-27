-- Seed data for development/demo
-- Run after migrations: sqlite3 budget.db < seed/seed_data.sql

-- Demo user: demo@budget.app / password: demo1234
INSERT OR IGNORE INTO users(email, password_hash, created_at) VALUES
  ('demo@budget.app',
   '8a7b3c1d2e4f5a6b:a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1d2e3f4a5b6c7d8e9f0a1b2',
   '2026-01-01T00:00:00Z');

-- Ensure account exists
INSERT OR IGNORE INTO accounts(user_id, balance) VALUES (1, 0);

-- Categories for user 1
INSERT OR IGNORE INTO categories(user_id, name, color, icon) VALUES
  (1, 'Salary',        '#22c55e', 'banknote'),
  (1, 'Food',          '#f97316', 'utensils'),
  (1, 'Rent',          '#6366f1', 'home'),
  (1, 'Transport',     '#3b82f6', 'car'),
  (1, 'Entertainment', '#ec4899', 'tv'),
  (1, 'Healthcare',    '#14b8a6', 'heart-pulse'),
  (1, 'Shopping',      '#a855f7', 'shopping-bag'),
  (1, 'Utilities',     '#78716c', 'zap');

-- Budgets for Jan-Mar 2026
INSERT OR IGNORE INTO budgets(user_id, year_month, category_id, amount) VALUES
  (1, '2026-01', (SELECT id FROM categories WHERE user_id=1 AND name='Food'),          400.00),
  (1, '2026-01', (SELECT id FROM categories WHERE user_id=1 AND name='Rent'),         1200.00),
  (1, '2026-01', (SELECT id FROM categories WHERE user_id=1 AND name='Transport'),      150.00),
  (1, '2026-01', (SELECT id FROM categories WHERE user_id=1 AND name='Entertainment'),  100.00),
  (1, '2026-01', (SELECT id FROM categories WHERE user_id=1 AND name='Healthcare'),      80.00),
  (1, '2026-01', (SELECT id FROM categories WHERE user_id=1 AND name='Shopping'),       200.00),
  (1, '2026-01', (SELECT id FROM categories WHERE user_id=1 AND name='Utilities'),      120.00),

  (1, '2026-02', (SELECT id FROM categories WHERE user_id=1 AND name='Food'),          400.00),
  (1, '2026-02', (SELECT id FROM categories WHERE user_id=1 AND name='Rent'),         1200.00),
  (1, '2026-02', (SELECT id FROM categories WHERE user_id=1 AND name='Transport'),      150.00),
  (1, '2026-02', (SELECT id FROM categories WHERE user_id=1 AND name='Entertainment'),  100.00),
  (1, '2026-02', (SELECT id FROM categories WHERE user_id=1 AND name='Healthcare'),      80.00),
  (1, '2026-02', (SELECT id FROM categories WHERE user_id=1 AND name='Shopping'),       200.00),
  (1, '2026-02', (SELECT id FROM categories WHERE user_id=1 AND name='Utilities'),      120.00),

  (1, '2026-03', (SELECT id FROM categories WHERE user_id=1 AND name='Food'),          400.00),
  (1, '2026-03', (SELECT id FROM categories WHERE user_id=1 AND name='Rent'),         1200.00),
  (1, '2026-03', (SELECT id FROM categories WHERE user_id=1 AND name='Transport'),      150.00),
  (1, '2026-03', (SELECT id FROM categories WHERE user_id=1 AND name='Entertainment'),  100.00),
  (1, '2026-03', (SELECT id FROM categories WHERE user_id=1 AND name='Healthcare'),      80.00),
  (1, '2026-03', (SELECT id FROM categories WHERE user_id=1 AND name='Shopping'),       200.00),
  (1, '2026-03', (SELECT id FROM categories WHERE user_id=1 AND name='Utilities'),      120.00);

-- January 2026 transactions
INSERT INTO transactions(user_id, date, amount, type, category_id, note, recurring, created_at) VALUES
  (1,'2026-01-01',3200.00,'income', (SELECT id FROM categories WHERE user_id=1 AND name='Salary'),      'January salary', 1, '2026-01-01T09:00:00Z'),
  (1,'2026-01-01',1200.00,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Rent'),        'Monthly rent',   1, '2026-01-01T09:01:00Z'),
  (1,'2026-01-03',  65.20,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Food'),        'Weekly groceries',0,'2026-01-03T11:00:00Z'),
  (1,'2026-01-05',  45.00,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Transport'),   'Monthly bus pass',1,'2026-01-05T08:00:00Z'),
  (1,'2026-01-07',  28.50,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Food'),        'Dinner out',     0, '2026-01-07T19:00:00Z'),
  (1,'2026-01-09',  55.00,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Utilities'),   'Electric bill',  0, '2026-01-09T10:00:00Z'),
  (1,'2026-01-10',  72.30,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Food'),        'Groceries',      0, '2026-01-10T12:00:00Z'),
  (1,'2026-01-12',  38.00,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Entertainment'),'Cinema tickets', 0,'2026-01-12T15:00:00Z'),
  (1,'2026-01-14', 120.00,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Shopping'),    'Winter jacket',  0, '2026-01-14T14:00:00Z'),
  (1,'2026-01-15',  35.00,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Utilities'),   'Water bill',     0, '2026-01-15T10:00:00Z'),
  (1,'2026-01-17',  22.80,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Food'),        'Lunch cafe',     0, '2026-01-17T13:00:00Z'),
  (1,'2026-01-19',  60.00,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Healthcare'),  'Dentist',        0, '2026-01-19T10:00:00Z'),
  (1,'2026-01-21',  88.40,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Food'),        'Groceries + wine',0,'2026-01-21T16:00:00Z'),
  (1,'2026-01-24',  25.00,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Entertainment'),'Streaming month',1,'2026-01-24T10:00:00Z'),
  (1,'2026-01-26',  45.80,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Shopping'),    'Home supplies',  0, '2026-01-26T11:00:00Z'),
  (1,'2026-01-28',  30.00,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Transport'),   'Taxi rides',     0, '2026-01-28T20:00:00Z');

-- February 2026 transactions
INSERT INTO transactions(user_id, date, amount, type, category_id, note, recurring, created_at) VALUES
  (1,'2026-02-01',3200.00,'income', (SELECT id FROM categories WHERE user_id=1 AND name='Salary'),      'February salary',1, '2026-02-01T09:00:00Z'),
  (1,'2026-02-01',1200.00,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Rent'),        'Monthly rent',   1, '2026-02-01T09:01:00Z'),
  (1,'2026-02-03',  78.50,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Food'),        'Groceries',      0, '2026-02-03T11:00:00Z'),
  (1,'2026-02-05',  45.00,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Transport'),   'Bus pass',       1, '2026-02-05T08:00:00Z'),
  (1,'2026-02-07',  52.00,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Utilities'),   'Electric',       0, '2026-02-07T10:00:00Z'),
  (1,'2026-02-08', 200.00,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Shopping'),    'Shoes & clothes',0,'2026-02-08T14:00:00Z'),
  (1,'2026-02-10',  33.60,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Food'),        'Takeout',        0, '2026-02-10T19:30:00Z'),
  (1,'2026-02-12',  49.99,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Entertainment'),'Concert tickets',0,'2026-02-12T10:00:00Z'),
  (1,'2026-02-14',  85.00,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Food'),        "Valentine's dinner",0,'2026-02-14T20:00:00Z'),
  (1,'2026-02-16', 500.00,'income', NULL,                                                                'Freelance work', 0,'2026-02-16T09:00:00Z'),
  (1,'2026-02-17',  65.40,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Food'),        'Groceries',      0, '2026-02-17T12:00:00Z'),
  (1,'2026-02-19',  30.00,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Healthcare'),  'Pharmacy',       0, '2026-02-19T16:00:00Z'),
  (1,'2026-02-21',  25.00,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Entertainment'),'Streaming',      1,'2026-02-21T10:00:00Z'),
  (1,'2026-02-24',  28.00,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Transport'),   'Taxi',           0, '2026-02-24T22:00:00Z'),
  (1,'2026-02-25',  75.30,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Food'),        'Groceries',      0, '2026-02-25T11:00:00Z'),
  (1,'2026-02-27',  30.00,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Utilities'),   'Internet',       1, '2026-02-27T10:00:00Z');

-- March 2026 transactions
INSERT INTO transactions(user_id, date, amount, type, category_id, note, recurring, created_at) VALUES
  (1,'2026-03-01',3200.00,'income', (SELECT id FROM categories WHERE user_id=1 AND name='Salary'),      'March salary',   1, '2026-03-01T09:00:00Z'),
  (1,'2026-03-01',1200.00,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Rent'),        'Monthly rent',   1, '2026-03-01T09:01:00Z'),
  (1,'2026-03-03',  55.20,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Food'),        'Groceries',      0, '2026-03-03T11:00:00Z'),
  (1,'2026-03-05',  45.00,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Transport'),   'Bus pass',       1, '2026-03-05T08:00:00Z'),
  (1,'2026-03-07',  42.00,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Utilities'),   'Electric',       0, '2026-03-07T10:00:00Z'),
  (1,'2026-03-08',  89.99,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Shopping'),    'Books & gear',   0, '2026-03-08T14:00:00Z'),
  (1,'2026-03-10',  38.50,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Food'),        'Lunch out',      0, '2026-03-10T13:00:00Z'),
  (1,'2026-03-12',  25.00,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Entertainment'),'Streaming',      1,'2026-03-12T10:00:00Z'),
  (1,'2026-03-14',  62.30,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Food'),        'Groceries',      0, '2026-03-14T11:00:00Z'),
  (1,'2026-03-16', 350.00,'income', NULL,                                                                'Side project',   0,'2026-03-16T09:00:00Z'),
  (1,'2026-03-17',  50.00,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Healthcare'),  'GP visit',       0, '2026-03-17T10:00:00Z'),
  (1,'2026-03-19',  33.00,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Transport'),   'Taxi',           0, '2026-03-19T21:00:00Z'),
  (1,'2026-03-21',  71.80,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Food'),        'Dinner party food',0,'2026-03-21T15:00:00Z'),
  (1,'2026-03-24',  30.00,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Utilities'),   'Internet',       1, '2026-03-24T10:00:00Z'),
  (1,'2026-03-25',  66.40,'expense',(SELECT id FROM categories WHERE user_id=1 AND name='Food'),        'Groceries',      0, '2026-03-25T12:00:00Z');

-- Update balance (sum of all transactions)
UPDATE accounts SET balance = (
  SELECT COALESCE(
    SUM(CASE WHEN type='income' THEN amount ELSE -amount END), 0
  )
  FROM transactions WHERE user_id=1
) WHERE user_id=1;

SELECT printf('Seed complete. Balance: %.2f', balance) FROM accounts WHERE user_id=1;
