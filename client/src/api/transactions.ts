import { api } from './client'
import type { Transaction, TransactionPage, TransactionFilter } from '../types'

export async function getTransactions(filter: TransactionFilter = {}): Promise<TransactionPage> {
  const params = new URLSearchParams()
  if (filter.start)    params.set('start',    filter.start)
  if (filter.end)      params.set('end',      filter.end)
  if (filter.category) params.set('category', String(filter.category))
  if (filter.type)     params.set('type',     filter.type)
  if (filter.page)     params.set('page',     String(filter.page))
  if (filter.size)     params.set('size',     String(filter.size))
  const { data } = await api.get<TransactionPage>(`/transactions?${params}`)
  return data
}

export async function getTransaction(id: number): Promise<Transaction> {
  const { data } = await api.get<Transaction>(`/transactions/${id}`)
  return data
}

export interface CreateTransactionInput {
  date:        string
  amount:      number
  type:        'expense' | 'income'
  category_id: number | null
  note:        string
  recurring:   boolean
  force?:      boolean
}

export async function createTransaction(input: CreateTransactionInput): Promise<Transaction> {
  const params = input.force ? '?force=true' : ''
  const { data } = await api.post<Transaction>(`/transactions${params}`, input)
  return data
}

export async function updateTransaction(id: number, input: Partial<CreateTransactionInput>): Promise<Transaction> {
  const { data } = await api.put<Transaction>(`/transactions/${id}`, input)
  return data
}

export async function deleteTransaction(id: number): Promise<void> {
  await api.delete(`/transactions/${id}`)
}

export async function undoTransactions(lastN = 1): Promise<{ removed_ids: number[]; count: number }> {
  const { data } = await api.post('/transactions/undo', { last_n: lastN })
  return data
}

export async function importCSV(csvText: string): Promise<{ imported: number; skipped: number; errors: string[] }> {
  const { data } = await api.post('/transactions/import', csvText, {
    headers: { 'Content-Type': 'text/csv' },
  })
  return data
}
