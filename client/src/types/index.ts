// ── Auth ──────────────────────────────────────────────────────────────────────
export interface AuthTokens {
  access_token:  string
  refresh_token: string
  expires_in:    number
  user_id:       number
  email:         string
}

export interface User {
  id:         number
  email:      string
  created_at: string
}

// ── Category ──────────────────────────────────────────────────────────────────
export interface Category {
  id:       number
  user_id:  number | null
  name:     string
  color:    string
  icon:     string
}

// ── Transaction ───────────────────────────────────────────────────────────────
export type TransactionType = 'expense' | 'income'

export interface Transaction {
  id:             number
  date:           string
  amount:         number
  type:           TransactionType
  category_id:    number | null
  category_name:  string
  category_color: string
  note:           string
  recurring:      boolean
  created_at:     string
}

export interface TransactionPage {
  items: Transaction[]
  total: number
  page:  number
  size:  number
  pages: number
}

export interface TransactionFilter {
  start?:    string
  end?:      string
  category?: number
  type?:     TransactionType | ''
  page?:     number
  size?:     number
}

// ── Budget ────────────────────────────────────────────────────────────────────
export interface Budget {
  id:             number
  year_month:     string
  category_id:    number
  category_name:  string
  category_color: string
  amount:         number
}

// ── Report ────────────────────────────────────────────────────────────────────
export interface CategorySummary {
  category_id: number
  category:    string
  color:       string
  budget:      number
  spent:       number
  income:      number
  status:      'ok' | 'over' | 'no_budget'
  over_by:     number
  remaining:   number
}

export interface MonthlyReport {
  year_month:    string
  total_income:  number
  total_expense: number
  net_change:    number
  ending_balance: number
  by_category:   CategorySummary[]
  top_expenses:  Transaction[]
}

export interface MonthlyTrend {
  year_month: string
  income:     number
  expense:    number
}

// ── Account ───────────────────────────────────────────────────────────────────
export interface Account {
  balance: number
}

// ── UI helpers ────────────────────────────────────────────────────────────────
export interface Toast {
  id:      string
  type:    'success' | 'error' | 'warning' | 'info'
  message: string
}

export interface ApiError {
  error:   string
  code:    number
  details?: Record<string, unknown>
}
