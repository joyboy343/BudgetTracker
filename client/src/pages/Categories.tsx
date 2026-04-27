import { useState, useEffect, useCallback } from 'react'
import { Plus, Trash2, Edit2, Check, X, ChevronLeft, ChevronRight } from 'lucide-react'
import { format, addMonths, subMonths, parseISO } from 'date-fns'
import { getCategories, createCategory, updateCategory, deleteCategory, getBudgets, upsertBudget, deleteBudget } from '../api/index'
import type { Category, Budget } from '../types'
import type { ToastFn } from '../hooks/useToast'

interface Props { toast: ToastFn; refreshKey: number }

const ICON_OPTIONS = ['tag','utensils','home','car','tv','heart-pulse','shopping-bag','zap',
  'plane','coffee','book','gamepad-2','music','dumbbell','banknote','gift','camera','laptop']

const COLOR_OPTIONS = [
  '#ef4444','#f97316','#f59e0b','#22c55e','#10b981','#14b8a6',
  '#3b82f6','#6366f1','#8b5cf6','#ec4899','#78716c','#94a3b8',
]

function fmt(n: number) {
  return n.toLocaleString('en-US', { minimumFractionDigits: 2, maximumFractionDigits: 2 })
}

export function CategoriesPage({ toast, refreshKey }: Props) {
  const [categories, setCategories]   = useState<Category[]>([])
  const [budgets,    setBudgets]       = useState<Budget[]>([])
  const [loading,    setLoading]       = useState(true)
  const [selectedMonth, setSelectedMonth] = useState(() => format(new Date(), 'yyyy-MM'))

  // New category form
  const [showNewCat,  setShowNewCat]  = useState(false)
  const [newCatName,  setNewCatName]  = useState('')
  const [newCatColor, setNewCatColor] = useState(COLOR_OPTIONS[5])
  const [newCatIcon,  setNewCatIcon]  = useState('tag')
  const [savingCat,   setSavingCat]   = useState(false)

  // Edit category inline
  const [editCatId,   setEditCatId]   = useState<number | null>(null)
  const [editCatName, setEditCatName] = useState('')

  // Budget editing
  const [editBudgetCatId, setEditBudgetCatId] = useState<number | null>(null)
  const [editBudgetAmt,   setEditBudgetAmt]   = useState('')

  const load = useCallback(async () => {
    setLoading(true)
    try {
      const [cats, buds] = await Promise.all([
        getCategories(),
        getBudgets(selectedMonth),
      ])
      setCategories(cats)
      setBudgets(buds)
    } catch {
      toast('error', 'Failed to load data')
    } finally {
      setLoading(false)
    }
  }, [selectedMonth, toast, refreshKey])

  useEffect(() => { load() }, [load])

  const handleCreateCat = async () => {
    if (!newCatName.trim()) return
    setSavingCat(true)
    try {
      await createCategory(newCatName.trim(), newCatColor, newCatIcon)
      toast('success', 'Category created')
      setNewCatName('')
      setShowNewCat(false)
      load()
    } catch {
      toast('error', 'Name already exists or failed')
    } finally {
      setSavingCat(false)
    }
  }

  const handleSaveCatName = async (cat: Category) => {
    if (!editCatName.trim()) { setEditCatId(null); return }
    try {
      await updateCategory(cat.id, editCatName.trim(), cat.color, cat.icon)
      toast('success', 'Category renamed')
      setEditCatId(null)
      load()
    } catch {
      toast('error', 'Failed to rename')
    }
  }

  const handleDeleteCat = async (id: number) => {
    if (!confirm('Delete this category? Transactions will lose this category.')) return
    try {
      await deleteCategory(id)
      toast('success', 'Category deleted')
      load()
    } catch {
      toast('error', 'Cannot delete category')
    }
  }

  const handleSaveBudget = async (catId: number) => {
    const amt = parseFloat(editBudgetAmt)
    if (isNaN(amt) || amt <= 0) { setEditBudgetCatId(null); return }
    try {
      await upsertBudget(selectedMonth, catId, amt)
      toast('success', 'Budget saved')
      setEditBudgetCatId(null)
      load()
    } catch {
      toast('error', 'Failed to save budget')
    }
  }

  const handleDeleteBudget = async (id: number) => {
    try {
      await deleteBudget(id)
      toast('success', 'Budget removed')
      load()
    } catch {
      toast('error', 'Failed to remove budget')
    }
  }

  const getBudgetFor = (catId: number) => budgets.find(b => b.category_id === catId)
  const prevMonth = () => setSelectedMonth(m => format(subMonths(parseISO(m + '-01'), 1), 'yyyy-MM'))
  const nextMonth = () => setSelectedMonth(m => format(addMonths(parseISO(m + '-01'), 1), 'yyyy-MM'))

  return (
    <div className="space-y-6 animate-fade-in">
      <div>
        <h1 className="font-display text-2xl font-bold text-white">Categories & Budgets</h1>
        <p className="text-sm text-slate-500 mt-1">Manage spending categories and set monthly limits</p>
      </div>

      {/* Month selector for budgets */}
      <div className="flex items-center gap-2 bg-surface-900 border border-slate-800 rounded-xl px-3 py-2 self-start w-fit">
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

      <div className="grid grid-cols-1 lg:grid-cols-2 gap-4">
        {/* Categories column */}
        <div className="card">
          <div className="flex items-center justify-between mb-4">
            <h2 className="text-sm font-semibold text-slate-300">Categories</h2>
            <button onClick={() => setShowNewCat(v => !v)} className="btn-ghost text-xs gap-1">
              <Plus size={13} /> New
            </button>
          </div>

          {/* New category form */}
          {showNewCat && (
            <div className="mb-4 p-4 bg-slate-800/50 rounded-xl border border-slate-700 space-y-3">
              <input
                type="text"
                placeholder="Category name"
                value={newCatName}
                onChange={e => setNewCatName(e.target.value)}
                onKeyDown={e => e.key === 'Enter' && handleCreateCat()}
                className="input"
                autoFocus
              />
              <div>
                <p className="text-xs text-slate-500 mb-2">Color</p>
                <div className="flex flex-wrap gap-2">
                  {COLOR_OPTIONS.map(c => (
                    <button
                      key={c}
                      onClick={() => setNewCatColor(c)}
                      className={`w-6 h-6 rounded-full border-2 transition-transform ${
                        newCatColor === c ? 'border-white scale-110' : 'border-transparent'
                      }`}
                      style={{ background: c }}
                    />
                  ))}
                </div>
              </div>
              <div>
                <p className="text-xs text-slate-500 mb-2">Icon</p>
                <div className="flex flex-wrap gap-1">
                  {ICON_OPTIONS.map(ic => (
                    <button
                      key={ic}
                      onClick={() => setNewCatIcon(ic)}
                      className={`px-2 py-1 rounded-lg text-xs transition-colors ${
                        newCatIcon === ic
                          ? 'bg-brand-500/20 text-brand-400 border border-brand-500/30'
                          : 'text-slate-500 hover:text-slate-300 hover:bg-slate-700'
                      }`}
                    >
                      {ic}
                    </button>
                  ))}
                </div>
              </div>
              <div className="flex gap-2 pt-1">
                <button onClick={() => setShowNewCat(false)} className="btn-ghost flex-1 justify-center text-sm py-2">Cancel</button>
                <button onClick={handleCreateCat} disabled={savingCat || !newCatName.trim()} className="btn-primary flex-1 justify-center text-sm py-2">
                  {savingCat ? <span className="w-3.5 h-3.5 border-2 border-white/30 border-t-white rounded-full animate-spin" /> : 'Create'}
                </button>
              </div>
            </div>
          )}

          {loading ? (
            <div className="space-y-2">
              {[...Array(6)].map((_, i) => <div key={i} className="h-10 skeleton rounded-xl" />)}
            </div>
          ) : categories.length === 0 ? (
            <p className="text-sm text-slate-600 text-center py-8">No categories yet</p>
          ) : (
            <div className="space-y-1">
              {categories.map(cat => (
                <div key={cat.id} className="flex items-center gap-3 px-3 py-2.5 rounded-xl hover:bg-slate-800/50 group transition-colors">
                  <div
                    className="w-7 h-7 rounded-lg flex items-center justify-center flex-shrink-0 text-xs font-semibold"
                    style={{ background: cat.color + '22', color: cat.color }}
                  >
                    {cat.name[0].toUpperCase()}
                  </div>

                  {editCatId === cat.id ? (
                    <input
                      type="text"
                      value={editCatName}
                      onChange={e => setEditCatName(e.target.value)}
                      onKeyDown={e => {
                        if (e.key === 'Enter') handleSaveCatName(cat)
                        if (e.key === 'Escape') setEditCatId(null)
                      }}
                      className="input py-1 text-sm flex-1"
                      autoFocus
                    />
                  ) : (
                    <span className="text-sm text-slate-200 flex-1">{cat.name}</span>
                  )}

                  <div className="flex items-center gap-1 opacity-0 group-hover:opacity-100 transition-opacity">
                    {editCatId === cat.id ? (
                      <>
                        <button onClick={() => handleSaveCatName(cat)} className="p-1.5 text-brand-400 hover:bg-brand-500/10 rounded-lg">
                          <Check size={13} />
                        </button>
                        <button onClick={() => setEditCatId(null)} className="p-1.5 text-slate-500 hover:bg-slate-700 rounded-lg">
                          <X size={13} />
                        </button>
                      </>
                    ) : (
                      <>
                        <button
                          onClick={() => { setEditCatId(cat.id); setEditCatName(cat.name) }}
                          className="p-1.5 text-slate-500 hover:text-slate-200 hover:bg-slate-700 rounded-lg transition-colors"
                        >
                          <Edit2 size={13} />
                        </button>
                        {cat.user_id !== null && (
                          <button
                            onClick={() => handleDeleteCat(cat.id)}
                            className="p-1.5 text-slate-500 hover:text-red-400 hover:bg-red-500/10 rounded-lg transition-colors"
                          >
                            <Trash2 size={13} />
                          </button>
                        )}
                      </>
                    )}
                  </div>
                </div>
              ))}
            </div>
          )}
        </div>

        {/* Budgets column */}
        <div className="card">
          <h2 className="text-sm font-semibold text-slate-300 mb-4">
            Monthly Budgets — {format(parseISO(selectedMonth + '-01'), 'MMMM yyyy')}
          </h2>
          {loading ? (
            <div className="space-y-2">
              {[...Array(6)].map((_, i) => <div key={i} className="h-12 skeleton rounded-xl" />)}
            </div>
          ) : categories.length === 0 ? (
            <p className="text-sm text-slate-600 text-center py-8">Create categories first</p>
          ) : (
            <div className="space-y-2">
              {categories.map(cat => {
                const budget = getBudgetFor(cat.id)
                const isEditing = editBudgetCatId === cat.id

                return (
                  <div key={cat.id} className="flex items-center gap-3 px-3 py-3 rounded-xl hover:bg-slate-800/30 group transition-colors">
                    <div
                      className="w-2 h-2 rounded-full flex-shrink-0"
                      style={{ background: cat.color }}
                    />
                    <span className="text-sm text-slate-300 flex-1">{cat.name}</span>

                    {isEditing ? (
                      <div className="flex items-center gap-2">
                        <div className="relative">
                          <span className="absolute left-2.5 top-1/2 -translate-y-1/2 text-slate-500 text-xs">$</span>
                          <input
                            type="number"
                            step="0.01"
                            min="0"
                            value={editBudgetAmt}
                            onChange={e => setEditBudgetAmt(e.target.value)}
                            onKeyDown={e => {
                              if (e.key === 'Enter') handleSaveBudget(cat.id)
                              if (e.key === 'Escape') setEditBudgetCatId(null)
                            }}
                            className="input w-28 py-1.5 pl-6 text-sm"
                            autoFocus
                          />
                        </div>
                        <button onClick={() => handleSaveBudget(cat.id)} className="p-1.5 text-brand-400 hover:bg-brand-500/10 rounded-lg">
                          <Check size={13} />
                        </button>
                        <button onClick={() => setEditBudgetCatId(null)} className="p-1.5 text-slate-500 hover:bg-slate-700 rounded-lg">
                          <X size={13} />
                        </button>
                      </div>
                    ) : (
                      <div className="flex items-center gap-2">
                        {budget ? (
                          <>
                            <span className="text-sm amount text-slate-400 tabular-nums">${fmt(budget.amount)}</span>
                            <div className="flex items-center gap-1 opacity-0 group-hover:opacity-100 transition-opacity">
                              <button
                                onClick={() => { setEditBudgetCatId(cat.id); setEditBudgetAmt(String(budget.amount)) }}
                                className="p-1.5 text-slate-500 hover:text-slate-200 hover:bg-slate-700 rounded-lg transition-colors"
                              >
                                <Edit2 size={13} />
                              </button>
                              <button
                                onClick={() => handleDeleteBudget(budget.id)}
                                className="p-1.5 text-slate-500 hover:text-red-400 hover:bg-red-500/10 rounded-lg transition-colors"
                              >
                                <Trash2 size={13} />
                              </button>
                            </div>
                          </>
                        ) : (
                          <button
                            onClick={() => { setEditBudgetCatId(cat.id); setEditBudgetAmt('') }}
                            className="opacity-0 group-hover:opacity-100 text-xs text-brand-400 hover:text-brand-300 flex items-center gap-1 transition-opacity"
                          >
                            <Plus size={12} /> Set budget
                          </button>
                        )}
                      </div>
                    )}
                  </div>
                )
              })}
            </div>
          )}
        </div>
      </div>
    </div>
  )
}
