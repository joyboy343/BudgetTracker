// ── categories.ts ─────────────────────────────────────────────────────────────
import { api } from './client'
import type { Category, Budget, MonthlyReport, MonthlyTrend, Account } from '../types'

export async function getCategories(): Promise<Category[]> {
  const { data } = await api.get<Category[]>('/categories')
  return data
}

export async function createCategory(name: string, color: string, icon: string): Promise<Category> {
  const { data } = await api.post<Category>('/categories', { name, color, icon })
  return data
}

export async function updateCategory(id: number, name: string, color: string, icon: string): Promise<Category> {
  const { data } = await api.put<Category>(`/categories/${id}`, { name, color, icon })
  return data
}

export async function deleteCategory(id: number): Promise<void> {
  await api.delete(`/categories/${id}`)
}

// ── budgets.ts ────────────────────────────────────────────────────────────────
export async function getBudgets(yearMonth?: string): Promise<Budget[]> {
  const params = yearMonth ? `?year_month=${yearMonth}` : ''
  const { data } = await api.get<Budget[]>(`/budgets${params}`)
  return data
}

export async function upsertBudget(yearMonth: string, categoryId: number, amount: number): Promise<Budget> {
  const { data } = await api.post<Budget>('/budgets', {
    year_month:  yearMonth,
    category_id: categoryId,
    amount,
  })
  return data
}

export async function deleteBudget(id: number): Promise<void> {
  await api.delete(`/budgets/${id}`)
}

// ── reports.ts ────────────────────────────────────────────────────────────────
export async function getMonthlyReport(yearMonth: string): Promise<MonthlyReport> {
  const { data } = await api.get<MonthlyReport>(`/reports/monthly?year_month=${yearMonth}`)
  return data
}

export async function getTrend(months = 6): Promise<MonthlyTrend[]> {
  const { data } = await api.get<MonthlyTrend[]>(`/reports/trend?months=${months}`)
  return data
}

export async function exportCSV(yearMonth?: string): Promise<Blob> {
  const params = yearMonth ? `?year_month=${yearMonth}&format=csv` : '?format=csv'
  const { data } = await api.get(`/reports/export${params}`, { responseType: 'blob' })
  return data
}

// ── account.ts ────────────────────────────────────────────────────────────────
export async function getAccount(): Promise<Account> {
  const { data } = await api.get<Account>('/account')
  return data
}

export async function fundAccount(amount: number, note = 'Account funding'): Promise<Account> {
  const { data } = await api.post<Account>('/account/fund', { amount, note })
  return data
}
