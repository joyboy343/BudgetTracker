import { useState, useEffect, useCallback } from 'react'
import { Download, TrendingUp, TrendingDown, DollarSign, ChevronLeft, ChevronRight } from 'lucide-react'
import { format, addMonths, subMonths, parseISO } from 'date-fns'
import { getMonthlyReport, getTrend, exportCSV } from '../api/index'
import type { MonthlyReport, MonthlyTrend } from '../types'
import { CategorySpendChart, TrendChart } from '../components/Charts'
import type { ToastFn } from '../hooks/useToast'

interface Props { toast: ToastFn }

function fmt(n: number) {
  return n.toLocaleString('en-US', { minimumFractionDigits: 2, maximumFractionDigits: 2 })
}

export function Reports({ toast }: Props) {
  const [selectedMonth, setSelectedMonth] = useState(() => format(new Date(), 'yyyy-MM'))
  const [report,  setReport]  = useState<MonthlyReport | null>(null)
  const [trend,   setTrend]   = useState<MonthlyTrend[]>([])
  const [loading, setLoading] = useState(true)

  const load = useCallback(async () => {
    setLoading(true)
    try {
      const [rep, trd] = await Promise.all([
        getMonthlyReport(selectedMonth),
        getTrend(6),
      ])
      setReport(rep)
      setTrend(trd)
    } catch {
      toast('error', 'Failed to load report')
    } finally {
      setLoading(false)
    }
  }, [selectedMonth, toast])

  useEffect(() => { load() }, [load])

  const prevMonth = () => setSelectedMonth(m => format(subMonths(parseISO(m + '-01'), 1), 'yyyy-MM'))
  const nextMonth = () => setSelectedMonth(m => format(addMonths(parseISO(m + '-01'), 1), 'yyyy-MM'))

  const handleExport = async () => {
    try {
      const blob = await exportCSV(selectedMonth)
      const url  = URL.createObjectURL(blob)
      const a    = document.createElement('a')
      a.href     = url
      a.download = `transactions_${selectedMonth}.csv`
      a.click()
      URL.revokeObjectURL(url)
      toast('success', 'Report downloaded')
    } catch {
      toast('error', 'Export failed')
    }
  }

  return (
    <div className="space-y-6 animate-fade-in">
      {/* Header */}
      <div className="flex flex-col sm:flex-row sm:items-center gap-4">
        <div>
          <h1 className="font-display text-2xl font-bold text-white">Reports</h1>
          <p className="text-sm text-slate-500 mt-1">Monthly summaries and spending insights</p>
        </div>
        <div className="sm:ml-auto flex items-center gap-3 flex-wrap">
          {/* Month selector */}
          <div className="flex items-center gap-2 bg-surface-900 border border-slate-800 rounded-xl px-3 py-2">
            <button onClick={prevMonth} className="text-slate-400 hover:text-slate-100 p-1 rounded-lg hover:bg-slate-800 transition-colors">
              <ChevronLeft size={16} />
            </button>
            <span className="text-sm font-medium text-slate-200 w-24 text-center">
              {format(parseISO(selectedMonth + '-01'), 'MMM yyyy')}
            </span>
            <button onClick={nextMonth} className="text-slate-400 hover:text-slate-100 p-1 rounded-lg hover:bg-slate-800 transition-colors">
              <ChevronRight size={16} />
            </button>
          </div>
          <button onClick={handleExport} className="btn-ghost text-sm gap-2">
            <Download size={14} /> Export CSV
          </button>
        </div>
      </div>

      {loading ? (
        <div className="space-y-4">
          <div className="grid grid-cols-3 gap-4">
            {[...Array(3)].map((_, i) => <div key={i} className="h-24 skeleton rounded-2xl" />)}
          </div>
          <div className="h-72 skeleton rounded-2xl" />
          <div className="h-72 skeleton rounded-2xl" />
        </div>
      ) : !report ? null : (
        <>
          {/* Summary cards */}
          <div className="grid grid-cols-1 sm:grid-cols-3 gap-4">
            <div className="card-sm">
              <div className="flex items-center gap-2 mb-3">
                <div className="w-7 h-7 rounded-lg bg-brand-500/10 text-brand-400 flex items-center justify-center">
                  <TrendingUp size={14} />
                </div>
                <p className="stat-label">Total Income</p>
              </div>
              <p className="text-2xl font-bold text-brand-400 amount tabular-nums">${fmt(report.total_income)}</p>
            </div>
            <div className="card-sm">
              <div className="flex items-center gap-2 mb-3">
                <div className="w-7 h-7 rounded-lg bg-red-500/10 text-red-400 flex items-center justify-center">
                  <TrendingDown size={14} />
                </div>
                <p className="stat-label">Total Expenses</p>
              </div>
              <p className="text-2xl font-bold text-red-400 amount tabular-nums">${fmt(report.total_expense)}</p>
            </div>
            <div className="card-sm">
              <div className="flex items-center gap-2 mb-3">
                <div className={`w-7 h-7 rounded-lg flex items-center justify-center ${
                  report.net_change >= 0 ? 'bg-brand-500/10 text-brand-400' : 'bg-red-500/10 text-red-400'
                }`}>
                  <DollarSign size={14} />
                </div>
                <p className="stat-label">Net Change</p>
              </div>
              <p className={`text-2xl font-bold amount tabular-nums ${report.net_change >= 0 ? 'text-brand-400' : 'text-red-400'}`}>
                {report.net_change >= 0 ? '+' : ''}${fmt(report.net_change)}
              </p>
            </div>
          </div>

          {/* 6-month trend */}
          {trend.length > 0 && (
            <div className="card">
              <h3 className="text-sm font-semibold text-slate-300 mb-4">6-Month Trend</h3>
              <TrendChart trend={trend} />
            </div>
          )}

          {/* Category breakdown */}
          <div className="grid grid-cols-1 lg:grid-cols-2 gap-4">
            {/* Chart */}
            {report.by_category.length > 0 && (
              <div className="card">
                <h3 className="text-sm font-semibold text-slate-300 mb-4">Spending by Category</h3>
                <CategorySpendChart categories={report.by_category} />
              </div>
            )}

            {/* Budget vs actual table */}
            <div className="card">
              <h3 className="text-sm font-semibold text-slate-300 mb-4">Budget vs Actual</h3>
              {report.by_category.length === 0 ? (
                <p className="text-sm text-slate-600 text-center py-8">No transactions this month</p>
              ) : (
                <div className="space-y-3">
                  {report.by_category.map(cat => (
                    <div key={cat.category_id}>
                      <div className="flex items-center justify-between mb-1.5">
                        <div className="flex items-center gap-2">
                          <div className="w-2 h-2 rounded-full" style={{ background: cat.color }} />
                          <span className="text-sm text-slate-300">{cat.category}</span>
                          {cat.status === 'over' && (
                            <span className="badge bg-red-500/10 text-red-400 border border-red-500/20 text-[10px]">
                              over by ${fmt(cat.over_by)}
                            </span>
                          )}
                          {cat.status === 'no_budget' && (
                            <span className="badge bg-slate-700 text-slate-400 text-[10px]">
                              no budget
                            </span>
                          )}
                        </div>
                        <div className="text-right">
                          <span className="text-sm amount tabular-nums text-slate-300">${fmt(cat.spent)}</span>
                          {cat.budget > 0 && (
                            <span className="text-xs text-slate-600 ml-1.5">/ ${fmt(cat.budget)}</span>
                          )}
                        </div>
                      </div>
                      {cat.budget > 0 && (
                        <div className="progress-bar">
                          <div
                            className="progress-fill"
                            style={{
                              width: `${Math.min(100, (cat.spent / cat.budget) * 100)}%`,
                              background: cat.status === 'over' ? 'rgb(239,68,68)'
                                : (cat.spent / cat.budget) > 0.8 ? 'rgb(245,158,11)'
                                : cat.color,
                            }}
                          />
                        </div>
                      )}
                    </div>
                  ))}
                </div>
              )}
            </div>
          </div>

          {/* Top expenses */}
          {report.top_expenses.length > 0 && (
            <div className="card">
              <h3 className="text-sm font-semibold text-slate-300 mb-4">Top Expenses</h3>
              <div className="overflow-x-auto">
                <table className="w-full">
                  <thead>
                    <tr className="border-b border-slate-800">
                      {['Date', 'Description', 'Category', 'Amount'].map(h => (
                        <th key={h} className="px-4 py-2 text-left text-xs text-slate-500 uppercase tracking-wide">{h}</th>
                      ))}
                    </tr>
                  </thead>
                  <tbody>
                    {report.top_expenses.map((t, i) => (
                      <tr key={t.id} className="border-b border-slate-800/50">
                        <td className="px-4 py-2.5 text-xs text-slate-500 font-mono">{t.date}</td>
                        <td className="px-4 py-2.5">
                          <div className="flex items-center gap-2">
                            <span className="w-5 h-5 rounded-full bg-slate-800 text-slate-500 flex items-center justify-center text-[10px] font-bold">
                              {i + 1}
                            </span>
                            <span className="text-sm text-slate-200">{t.note || '—'}</span>
                          </div>
                        </td>
                        <td className="px-4 py-2.5">
                          {t.category_name ? (
                            <span className="flex items-center gap-1.5">
                              <span className="w-2 h-2 rounded-full" style={{ background: t.category_color || '#6366f1' }} />
                              <span className="text-xs text-slate-400">{t.category_name}</span>
                            </span>
                          ) : <span className="text-xs text-slate-600">—</span>}
                        </td>
                        <td className="px-4 py-2.5">
                          <span className="text-sm font-semibold text-red-400 amount tabular-nums">
                            ${fmt(t.amount)}
                          </span>
                        </td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              </div>
            </div>
          )}
        </>
      )}
    </div>
  )
}
