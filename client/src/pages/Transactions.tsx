import { useState, useEffect, useCallback } from 'react'
import { Search, Trash2, Edit2, Undo2, Upload, Download, ChevronLeft, ChevronRight } from 'lucide-react'
import { getTransactions, deleteTransaction, undoTransactions, importCSV } from '../api/transactions'
import { exportCSV } from '../api/index'
import type { Transaction, TransactionPage, Category, TransactionType } from '../types'
import type { ToastFn } from '../hooks/useToast'

interface Props {
  categories:  Category[]
  onEdit:      (t: Transaction) => void
  toast:       ToastFn
  refreshKey:  number
}

function fmt(n: number) {
  return n.toLocaleString('en-US', { minimumFractionDigits: 2, maximumFractionDigits: 2 })
}

export function Transactions({ categories, onEdit, toast, refreshKey }: Props) {
  const [data,       setData]       = useState<TransactionPage | null>(null)
  const [loading,    setLoading]    = useState(true)
  const [page,       setPage]       = useState(1)
  const [search,     setSearch]     = useState('')
  const [typeFilter, setTypeFilter] = useState<TransactionType | ''>('')
  const [catFilter,  setCatFilter]  = useState('')
  const [deleting,   setDeleting]   = useState<number | null>(null)

  const load = useCallback(async () => {
    setLoading(true)
    try {
      const result = await getTransactions({
        page,
        size: 20,
        type: typeFilter || undefined,
        category: catFilter ? Number(catFilter) : undefined,
      })
      setData(result)
    } catch {
      toast('error', 'Failed to load transactions')
    } finally {
      setLoading(false)
    }
  }, [page, typeFilter, catFilter, toast, refreshKey])

  useEffect(() => { load() }, [load])
  useEffect(() => { setPage(1) }, [typeFilter, catFilter])

  const handleDelete = async (id: number) => {
    if (!confirm('Delete this transaction?')) return
    setDeleting(id)
    try {
      await deleteTransaction(id)
      toast('success', 'Transaction deleted')
      load()
    } catch {
      toast('error', 'Failed to delete')
    } finally {
      setDeleting(null)
    }
  }

  const handleUndo = async () => {
    try {
      const r = await undoTransactions(1)
      toast('success', `Undid ${r.count} transaction`)
      load()
    } catch {
      toast('error', 'Nothing to undo')
    }
  }

  const handleExport = async () => {
    try {
      const blob = await exportCSV()
      const url  = URL.createObjectURL(blob)
      const a    = document.createElement('a')
      a.href     = url
      a.download = 'transactions.csv'
      a.click()
      URL.revokeObjectURL(url)
      toast('success', 'Export downloaded')
    } catch {
      toast('error', 'Export failed')
    }
  }

  const handleImport = async (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0]
    if (!file) return
    const text = await file.text()
    try {
      const result = await importCSV(text)
      toast('success', `Imported ${result.imported} rows${result.skipped ? `, skipped ${result.skipped}` : ''}`)
      load()
    } catch {
      toast('error', 'Import failed')
    }
    e.target.value = ''
  }

  // Client-side search filter
  const displayed = data?.items.filter(t =>
    !search || t.note.toLowerCase().includes(search.toLowerCase()) ||
    t.category_name.toLowerCase().includes(search.toLowerCase())
  ) ?? []

  return (
    <div className="space-y-4 animate-fade-in">
      {/* Header */}
      <div className="flex flex-col sm:flex-row sm:items-center gap-3">
        <div>
          <h1 className="font-display text-2xl font-bold text-white">Transactions</h1>
          <p className="text-sm text-slate-500">
            {data ? `${data.total} total` : '…'}
          </p>
        </div>
        <div className="sm:ml-auto flex items-center gap-2 flex-wrap">
          <button onClick={handleUndo} className="btn-ghost text-xs gap-1.5">
            <Undo2 size={13} /> Undo last
          </button>
          <label className="btn-ghost text-xs gap-1.5 cursor-pointer">
            <Upload size={13} /> Import CSV
            <input type="file" accept=".csv" className="hidden" onChange={handleImport} />
          </label>
          <button onClick={handleExport} className="btn-ghost text-xs gap-1.5">
            <Download size={13} /> Export
          </button>
        </div>
      </div>

      {/* Search & filter bar */}
      <div className="card-sm flex flex-col sm:flex-row gap-3">
        <div className="relative flex-1">
          <Search size={14} className="absolute left-3 top-1/2 -translate-y-1/2 text-slate-500" />
          <input
            type="text"
            placeholder="Search transactions…"
            value={search}
            onChange={e => setSearch(e.target.value)}
            className="input pl-9 py-2"
          />
        </div>
        <div className="flex gap-2">
          <select
            value={typeFilter}
            onChange={e => setTypeFilter(e.target.value as TransactionType | '')}
            className="select text-sm py-2"
          >
            <option value="">All types</option>
            <option value="income">Income</option>
            <option value="expense">Expense</option>
          </select>
          <select
            value={catFilter}
            onChange={e => setCatFilter(e.target.value)}
            className="select text-sm py-2"
          >
            <option value="">All categories</option>
            {categories.map(c => (
              <option key={c.id} value={c.id}>{c.name}</option>
            ))}
          </select>
          {(typeFilter || catFilter) && (
            <button
              onClick={() => { setTypeFilter(''); setCatFilter('') }}
              className="btn-ghost text-xs px-2.5"
            >
              Clear
            </button>
          )}
        </div>
      </div>

      {/* Table */}
      <div className="card p-0 overflow-hidden">
        {loading ? (
          <div className="p-6 space-y-3">
            {[...Array(8)].map((_, i) => (
              <div key={i} className="h-12 skeleton rounded-xl" />
            ))}
          </div>
        ) : displayed.length === 0 ? (
          <div className="flex flex-col items-center justify-center py-16 text-slate-600">
            <Search size={28} className="mb-2" />
            <p>No transactions found</p>
          </div>
        ) : (
          <div className="overflow-x-auto">
            <table className="w-full">
              <thead>
                <tr className="border-b border-slate-800">
                  {['Date', 'Description', 'Category', 'Amount', 'Type', ''].map(h => (
                    <th key={h} className="px-4 py-3 text-left text-xs font-medium text-slate-500 uppercase tracking-wide">
                      {h}
                    </th>
                  ))}
                </tr>
              </thead>
              <tbody>
                {displayed.map(t => (
                  <tr
                    key={t.id}
                    className="border-b border-slate-800/50 hover:bg-slate-800/30 transition-colors group"
                  >
                    <td className="px-4 py-3 text-sm text-slate-400 font-mono">{t.date}</td>
                    <td className="px-4 py-3">
                      <div className="flex items-center gap-2">
                        <p className="text-sm text-slate-200 truncate max-w-[200px]">
                          {t.note || '—'}
                        </p>
                        {t.recurring && (
                          <span className="badge bg-blue-500/10 text-blue-400 border border-blue-500/20 text-[10px]">
                            recurring
                          </span>
                        )}
                      </div>
                    </td>
                    <td className="px-4 py-3">
                      {t.category_name ? (
                        <span className="flex items-center gap-1.5">
                          <span
                            className="w-2 h-2 rounded-full flex-shrink-0"
                            style={{ background: t.category_color || '#6366f1' }}
                          />
                          <span className="text-xs text-slate-400">{t.category_name}</span>
                        </span>
                      ) : (
                        <span className="text-xs text-slate-600">—</span>
                      )}
                    </td>
                    <td className="px-4 py-3">
                      <span className={`text-sm font-semibold amount tabular-nums ${
                        t.type === 'income' ? 'text-brand-400' : 'text-red-400'
                      }`}>
                        {t.type === 'income' ? '+' : '-'}${fmt(t.amount)}
                      </span>
                    </td>
                    <td className="px-4 py-3">
                      <span className={`badge text-xs ${
                        t.type === 'income'
                          ? 'bg-brand-500/10 text-brand-400 border border-brand-500/20'
                          : 'bg-red-500/10   text-red-400   border border-red-500/20'
                      }`}>
                        {t.type}
                      </span>
                    </td>
                    <td className="px-4 py-3">
                      <div className="flex items-center gap-1 opacity-0 group-hover:opacity-100 transition-opacity">
                        <button
                          onClick={() => onEdit(t)}
                          className="p-1.5 rounded-lg text-slate-500 hover:text-slate-200 hover:bg-slate-700 transition-colors"
                          title="Edit"
                        >
                          <Edit2 size={13} />
                        </button>
                        <button
                          onClick={() => handleDelete(t.id)}
                          disabled={deleting === t.id}
                          className="p-1.5 rounded-lg text-slate-500 hover:text-red-400 hover:bg-red-500/10 transition-colors"
                          title="Delete"
                        >
                          <Trash2 size={13} />
                        </button>
                      </div>
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        )}

        {/* Pagination */}
        {data && data.pages > 1 && (
          <div className="flex items-center justify-between px-4 py-3 border-t border-slate-800">
            <p className="text-xs text-slate-500">
              Page {data.page} of {data.pages} · {data.total} results
            </p>
            <div className="flex items-center gap-1">
              <button
                disabled={page <= 1}
                onClick={() => setPage(p => p - 1)}
                className="btn-ghost py-1.5 px-2 disabled:opacity-30"
              >
                <ChevronLeft size={14} />
              </button>
              {Array.from({ length: Math.min(5, data.pages) }, (_, i) => {
                const p = Math.max(1, Math.min(data.pages - 4, page - 2)) + i
                return (
                  <button
                    key={p}
                    onClick={() => setPage(p)}
                    className={`w-8 h-8 rounded-lg text-xs font-medium transition-colors ${
                      p === page
                        ? 'bg-brand-500/20 text-brand-400 border border-brand-500/30'
                        : 'text-slate-500 hover:text-slate-200 hover:bg-slate-800'
                    }`}
                  >
                    {p}
                  </button>
                )
              })}
              <button
                disabled={page >= data.pages}
                onClick={() => setPage(p => p + 1)}
                className="btn-ghost py-1.5 px-2 disabled:opacity-30"
              >
                <ChevronRight size={14} />
              </button>
            </div>
          </div>
        )}
      </div>
    </div>
  )
}
