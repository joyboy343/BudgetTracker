import { useState, useEffect } from 'react'
import { X, RefreshCw } from 'lucide-react'
import { createTransaction, updateTransaction } from '../api/transactions'
import type { Transaction, Category, TransactionType } from '../types'
import type { ToastFn } from '../hooks/useToast'
import { format } from 'date-fns'

interface Props {
  open:        boolean
  onClose:     () => void
  categories:  Category[]
  editTxn?:    Transaction | null
  onSaved:     (t: Transaction) => void
  toast:       ToastFn
}

const INITIAL: {
  date: string; amount: string; type: TransactionType
  category_id: string; note: string; recurring: boolean; force: boolean
} = {
  date:        format(new Date(), 'yyyy-MM-dd'),
  amount:      '',
  type:        'expense',
  category_id: '',
  note:        '',
  recurring:   false,
  force:       false,
}

export function TransactionModal({ open, onClose, categories, editTxn, onSaved, toast }: Props) {
  const [form,    setForm]    = useState({ ...INITIAL })
  const [loading, setLoading] = useState(false)
  const [errors,  setErrors]  = useState<Record<string, string>>({})
  const [showForce, setShowForce] = useState(false)

  useEffect(() => {
    if (!open) return
    if (editTxn) {
      setForm({
        date:        editTxn.date,
        amount:      String(editTxn.amount),
        type:        editTxn.type,
        category_id: editTxn.category_id ? String(editTxn.category_id) : '',
        note:        editTxn.note,
        recurring:   editTxn.recurring,
        force:       false,
      })
    } else {
      setForm({ ...INITIAL })
    }
    setErrors({})
    setShowForce(false)
  }, [open, editTxn])

  const validate = () => {
    const e: Record<string, string> = {}
    if (!form.date)   e.date   = 'Date is required'
    if (!form.amount || isNaN(Number(form.amount)) || Number(form.amount) <= 0)
      e.amount = 'Valid positive amount required'
    return e
  }

  const submit = async () => {
    const e = validate()
    if (Object.keys(e).length) { setErrors(e); return }

    setLoading(true)
    try {
      const payload = {
        date:        form.date,
        amount:      Number(form.amount),
        type:        form.type,
        category_id: form.category_id ? Number(form.category_id) : null,
        note:        form.note,
        recurring:   form.recurring,
        force:       form.force,
      }

      const saved = editTxn
        ? await updateTransaction(editTxn.id, payload)
        : await createTransaction(payload)

      toast('success', editTxn ? 'Transaction updated' : 'Transaction added')
      onSaved(saved)
      onClose()
    } catch (err: unknown) {
      const data = (err as { response?: { data?: { error?: string; code?: number } } })?.response?.data
      if (data?.code === 409) {
        setShowForce(true)
        setErrors({ amount: data.error ?? 'Insufficient balance' })
      } else {
        toast('error', data?.error ?? 'Something went wrong')
      }
    } finally {
      setLoading(false)
    }
  }

  const handleForce = () => {
    setForm(f => ({ ...f, force: true }))
    setShowForce(false)
    setErrors({})
    submit()
  }

  if (!open) return null

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center p-4 backdrop">
      <div
        className="w-full max-w-md bg-surface-900 border border-slate-800 rounded-2xl shadow-2xl animate-scale-in"
        onClick={(e) => e.stopPropagation()}
      >
        {/* Header */}
        <div className="flex items-center justify-between px-6 py-4 border-b border-slate-800">
          <h2 className="font-display text-lg font-semibold text-white">
            {editTxn ? 'Edit Transaction' : 'Add Transaction'}
          </h2>
          <button onClick={onClose} className="btn-ghost p-1.5">
            <X size={18} />
          </button>
        </div>

        <div className="px-6 py-5 space-y-4">
          {/* Type toggle */}
          <div className="flex rounded-xl overflow-hidden border border-slate-700 p-1 gap-1">
            {(['expense', 'income'] as TransactionType[]).map((t) => (
              <button
                key={t}
                onClick={() => setForm(f => ({ ...f, type: t }))}
                className={`flex-1 py-2 rounded-lg text-sm font-medium transition-all duration-150 capitalize ${
                  form.type === t
                    ? t === 'expense'
                      ? 'bg-red-500/15 text-red-400 border border-red-500/20'
                      : 'bg-brand-500/15 text-brand-400 border border-brand-500/20'
                    : 'text-slate-500 hover:text-slate-300'
                }`}
              >
                {t}
              </button>
            ))}
          </div>

          {/* Amount */}
          <div>
            <label className="block text-xs font-medium text-slate-400 mb-1.5">Amount</label>
            <div className="relative">
              <span className="absolute left-3.5 top-1/2 -translate-y-1/2 text-slate-500 font-mono text-sm">$</span>
              <input
                type="number"
                step="0.01"
                min="0"
                placeholder="0.00"
                value={form.amount}
                onChange={(e) => setForm(f => ({ ...f, amount: e.target.value }))}
                className={`input pl-8 font-mono ${errors.amount ? 'border-red-500' : ''}`}
              />
            </div>
            {errors.amount && <p className="text-red-400 text-xs mt-1">{errors.amount}</p>}
            {showForce && (
              <div className="mt-2 p-3 bg-amber-500/10 border border-amber-500/20 rounded-lg">
                <p className="text-amber-400 text-xs mb-2">This will make your balance negative. Proceed anyway?</p>
                <button onClick={handleForce} className="text-xs btn bg-amber-500/20 text-amber-400 hover:bg-amber-500/30 py-1.5 px-3">
                  Override balance check
                </button>
              </div>
            )}
          </div>

          {/* Date */}
          <div>
            <label className="block text-xs font-medium text-slate-400 mb-1.5">Date</label>
            <input
              type="date"
              value={form.date}
              onChange={(e) => setForm(f => ({ ...f, date: e.target.value }))}
              className={`input ${errors.date ? 'border-red-500' : ''}`}
            />
            {errors.date && <p className="text-red-400 text-xs mt-1">{errors.date}</p>}
          </div>

          {/* Category */}
          <div>
            <label className="block text-xs font-medium text-slate-400 mb-1.5">Category</label>
            <select
              value={form.category_id}
              onChange={(e) => setForm(f => ({ ...f, category_id: e.target.value }))}
              className="select"
            >
              <option value="">No category</option>
              {categories.map((c) => (
                <option key={c.id} value={c.id}>{c.name}</option>
              ))}
            </select>
          </div>

          {/* Note */}
          <div>
            <label className="block text-xs font-medium text-slate-400 mb-1.5">Note</label>
            <input
              type="text"
              placeholder="What was this for?"
              value={form.note}
              maxLength={200}
              onChange={(e) => setForm(f => ({ ...f, note: e.target.value }))}
              className="input"
            />
          </div>

          {/* Recurring */}
          <label className="flex items-center gap-3 cursor-pointer group">
            <div
              onClick={() => setForm(f => ({ ...f, recurring: !f.recurring }))}
              className={`relative w-9 h-5 rounded-full transition-colors duration-200 ${
                form.recurring ? 'bg-brand-500' : 'bg-slate-700'
              }`}
            >
              <div className={`absolute top-0.5 w-4 h-4 rounded-full bg-white shadow transition-transform duration-200 ${
                form.recurring ? 'translate-x-4' : 'translate-x-0.5'
              }`} />
            </div>
            <div className="flex items-center gap-1.5 text-sm text-slate-400 group-hover:text-slate-300">
              <RefreshCw size={13} />
              Recurring monthly
            </div>
          </label>
        </div>

        {/* Footer */}
        <div className="flex gap-3 px-6 py-4 border-t border-slate-800">
          <button onClick={onClose} className="btn-ghost flex-1 justify-center">Cancel</button>
          <button
            onClick={submit}
            disabled={loading}
            className="btn-primary flex-1 justify-center"
          >
            {loading ? (
              <span className="w-4 h-4 border-2 border-white/30 border-t-white rounded-full animate-spin" />
            ) : (
              editTxn ? 'Update' : 'Add'
            )}
          </button>
        </div>
      </div>
    </div>
  )
}
