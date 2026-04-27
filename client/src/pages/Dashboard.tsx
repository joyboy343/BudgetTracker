import { useState, useEffect, useCallback } from 'react'
import { ArrowUpRight, ArrowDownRight, TrendingUp, RefreshCw, Wallet, ChevronLeft, ChevronRight } from 'lucide-react'
import { format, addMonths, subMonths, parseISO } from 'date-fns'
import { getAccount } from '../api/index'
import { getMonthlyReport, getTrend } from '../api/index'
import { getTransactions } from '../api/transactions'
import type { Account, MonthlyReport, MonthlyTrend, Transaction, Category } from '../types'
import { MonthlyChart, TrendChart } from '../components/Charts'
import type { ToastFn } from '../hooks/useToast'

interface Props {
  categories:       Category[]
  onAddTransaction: () => void
  toast:            ToastFn
  refreshKey:       number
}

function fmt(n: number) {
  return n.toLocaleString('en-US', { minimumFractionDigits: 2, maximumFractionDigits: 2 })
}

export function Dashboard({ categories: _categories, onAddTransaction, toast, refreshKey }: Props) {
  const [selectedMonth, setSelectedMonth] = useState(() => format(new Date(), 'yyyy-MM'))
  const [account,   setAccount]   = useState<Account | null>(null)
  const [report,    setReport]    = useState<MonthlyReport | null>(null)
  const [trend,     setTrend]     = useState<MonthlyTrend[]>([])
  const [recent,    setRecent]    = useState<Transaction[]>([])
  const [loading,   setLoading]   = useState(true)

  const load = useCallback(async () => {
    setLoading(true)
    try {
      const [acc, rep, trd, txns] = await Promise.all([
        getAccount(),
        getMonthlyReport(selectedMonth),
        getTrend(6),
        getTransactions({ page: 1, size: 5 }),
      ])
      setAccount(acc)
      setReport(rep)
      setTrend(trd)
      setRecent(txns.items)
    } catch {
      toast('error', 'Failed to load dashboard data')
    } finally {
      setLoading(false)
    }
  }, [selectedMonth, toast, refreshKey])

  useEffect(() => { load() }, [load])

  const prevMonth = () => setSelectedMonth(m => format(subMonths(parseISO(m + '-01'), 1), 'yyyy-MM'))
  const nextMonth = () => setSelectedMonth(m => format(addMonths(parseISO(m + '-01'), 1), 'yyyy-MM'))
  const isCurrentMonth = selectedMonth === format(new Date(), 'yyyy-MM')

  if (loading) return <DashboardSkeleton />

  const savingsRate = report && report.total_income > 0
    ? ((report.total_income - report.total_expense) / report.total_income * 100).toFixed(0)
    : '0'

  return (
    <div className="space-y-6 animate-fade-in">
      {/* Month selector + Balance header */}
      <div className="flex flex-col sm:flex-row sm:items-center sm:justify-between gap-4">
        <div>
          <p className="stat-label mb-1">Current Balance</p>
          <div className="flex items-baseline gap-2">
            <span className="font-display text-4xl font-bold text-white amount tabular-nums">
              ${fmt(account?.balance ?? 0)}
            </span>
          </div>
        </div>

        <div className="flex items-center gap-2 bg-surface-900 border border-slate-800 rounded-xl px-3 py-2 self-start sm:self-auto">
          <button onClick={prevMonth} className="text-slate-400 hover:text-slate-100 p-1 rounded-lg hover:bg-slate-800 transition-colors">
            <ChevronLeft size={16} />
          </button>
          <span className="text-sm font-medium text-slate-200 w-20 text-center">
            {format(parseISO(selectedMonth + '-01'), 'MMM yyyy')}
          </span>
          <button
            onClick={nextMonth}
            disabled={isCurrentMonth}
            className="text-slate-400 hover:text-slate-100 p-1 rounded-lg hover:bg-slate-800 transition-colors disabled:opacity-30 disabled:cursor-not-allowed"
          >
            <ChevronRight size={16} />
          </button>
        </div>
      </div>

      {/* Stat cards */}
      <div className="grid grid-cols-2 lg:grid-cols-4 gap-4">
        <StatCard
          label="Income"
          value={`$${fmt(report?.total_income ?? 0)}`}
          icon={<ArrowUpRight size={16} />}
          color="text-brand-400"
          bg="bg-brand-500/10"
        />
        <StatCard
          label="Expenses"
          value={`$${fmt(report?.total_expense ?? 0)}`}
          icon={<ArrowDownRight size={16} />}
          color="text-red-400"
          bg="bg-red-500/10"
        />
        <StatCard
          label="Net Change"
          value={`${(report?.net_change ?? 0) >= 0 ? '+' : ''}$${fmt(report?.net_change ?? 0)}`}
          icon={<TrendingUp size={16} />}
          color={(report?.net_change ?? 0) >= 0 ? 'text-brand-400' : 'text-red-400'}
          bg={(report?.net_change ?? 0) >= 0 ? 'bg-brand-500/10' : 'bg-red-500/10'}
        />
        <StatCard
          label="Savings Rate"
          value={`${savingsRate}%`}
          icon={<Wallet size={16} />}
          color="text-blue-400"
          bg="bg-blue-500/10"
        />
      </div>

      {/* Charts row */}
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-4">
        <div className="card">
          <h3 className="text-sm font-semibold text-slate-300 mb-4">
            {format(parseISO(selectedMonth + '-01'), 'MMMM')} Overview
          </h3>
          {report ? <MonthlyChart report={report} /> : <EmptyChart />}
        </div>
        <div className="card">
          <h3 className="text-sm font-semibold text-slate-300 mb-4">6-Month Trend</h3>
          {trend.length > 0 ? <TrendChart trend={trend} /> : <EmptyChart />}
        </div>
      </div>

      {/* Budget progress + Recent transactions */}
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-4">
        {/* Budget progress */}
        <div className="card">
          <div className="flex items-center justify-between mb-4">
            <h3 className="text-sm font-semibold text-slate-300">Budget Status</h3>
            <span className="text-xs text-slate-500">{format(parseISO(selectedMonth + '-01'), 'MMM yyyy')}</span>
          </div>
          {report && report.by_category.length > 0 ? (
            <div className="space-y-3">
              {report.by_category
                .filter(c => c.budget > 0)
                .sort((a, b) => b.spent - a.spent)
                .slice(0, 6)
                .map(cat => {
                  const pct = Math.min(100, cat.budget > 0 ? (cat.spent / cat.budget) * 100 : 0)
                  return (
                    <div key={cat.category_id}>
                      <div className="flex items-center justify-between mb-1">
                        <div className="flex items-center gap-2">
                          <div className="w-2 h-2 rounded-full" style={{ background: cat.color }} />
                          <span className="text-xs font-medium text-slate-300">{cat.category}</span>
                        </div>
                        <div className="flex items-center gap-2">
                          <span className="text-xs amount text-slate-400">${fmt(cat.spent)}</span>
                          <span className="text-xs text-slate-600">/</span>
                          <span className="text-xs amount text-slate-500">${fmt(cat.budget)}</span>
                          {cat.status === 'over' && (
                            <span className="badge bg-red-500/10 text-red-400 border border-red-500/20 text-[10px]">over</span>
                          )}
                        </div>
                      </div>
                      <div className="progress-bar">
                        <div
                          className="progress-fill"
                          style={{
                            width: `${pct}%`,
                            background: cat.status === 'over'
                              ? 'rgb(239,68,68)'
                              : pct > 80 ? 'rgb(245,158,11)' : cat.color,
                          }}
                        />
                      </div>
                    </div>
                  )
                })}
            </div>
          ) : (
            <EmptyState icon={<Wallet size={20} />} text="No budget data for this month" />
          )}
        </div>

        {/* Recent transactions */}
        <div className="card">
          <div className="flex items-center justify-between mb-4">
            <h3 className="text-sm font-semibold text-slate-300">Recent Transactions</h3>
            <button
              onClick={onAddTransaction}
              className="text-xs text-brand-400 hover:text-brand-300 font-medium"
            >
              + Add
            </button>
          </div>
          {recent.length > 0 ? (
            <div className="space-y-2">
              {recent.map(t => (
                <div key={t.id} className="flex items-center gap-3 p-2.5 rounded-xl hover:bg-slate-800/50 transition-colors">
                  <div
                    className="w-8 h-8 rounded-xl flex items-center justify-center text-xs font-semibold flex-shrink-0"
                    style={{ background: (t.category_color || '#6366f1') + '22', color: t.category_color || '#6366f1' }}
                  >
                    {(t.category_name || '?')[0]?.toUpperCase()}
                  </div>
                  <div className="flex-1 min-w-0">
                    <p className="text-sm text-slate-200 truncate">{t.note || t.category_name || 'Uncategorized'}</p>
                    <p className="text-xs text-slate-500">{t.date} · {t.category_name || 'No category'}</p>
                  </div>
                  <span className={`text-sm font-semibold amount tabular-nums flex-shrink-0 ${
                    t.type === 'income' ? 'text-brand-400' : 'text-red-400'
                  }`}>
                    {t.type === 'income' ? '+' : '-'}${fmt(t.amount)}
                  </span>
                </div>
              ))}
            </div>
          ) : (
            <EmptyState icon={<RefreshCw size={20} />} text="No transactions yet" />
          )}
        </div>
      </div>
    </div>
  )
}

function StatCard({ label, value, icon, color, bg }: {
  label: string; value: string; icon: React.ReactNode; color: string; bg: string
}) {
  return (
    <div className="card-sm">
      <div className="flex items-center justify-between mb-3">
        <p className="stat-label">{label}</p>
        <div className={`w-7 h-7 rounded-lg ${bg} ${color} flex items-center justify-center`}>
          {icon}
        </div>
      </div>
      <p className={`text-xl font-bold amount tabular-nums ${color}`}>{value}</p>
    </div>
  )
}

function EmptyChart() {
  return (
    <div className="h-48 flex items-center justify-center">
      <p className="text-sm text-slate-600">No data for this period</p>
    </div>
  )
}

function EmptyState({ icon, text }: { icon: React.ReactNode; text: string }) {
  return (
    <div className="flex flex-col items-center justify-center py-8 gap-2 text-slate-600">
      {icon}
      <p className="text-sm">{text}</p>
    </div>
  )
}

function DashboardSkeleton() {
  return (
    <div className="space-y-6 animate-pulse">
      <div className="h-12 skeleton w-48" />
      <div className="grid grid-cols-2 lg:grid-cols-4 gap-4">
        {[...Array(4)].map((_, i) => <div key={i} className="h-24 skeleton rounded-2xl" />)}
      </div>
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-4">
        {[...Array(2)].map((_, i) => <div key={i} className="h-64 skeleton rounded-2xl" />)}
      </div>
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-4">
        {[...Array(2)].map((_, i) => <div key={i} className="h-64 skeleton rounded-2xl" />)}
      </div>
    </div>
  )
}
