import { useState, useCallback, useEffect } from 'react'
import { BrowserRouter, Routes, Route, Navigate } from 'react-router-dom'
import { AuthProvider, useAuth } from './context/AuthContext'
import { Sidebar } from './components/Sidebar'
import { ToastContainer } from './components/Toast'
import { TransactionModal } from './components/TransactionModal'
import { Login } from './pages/Login'
import { Register } from './pages/Register'
import { Dashboard } from './pages/Dashboard'
import { Transactions } from './pages/Transactions'
import { CategoriesPage } from './pages/Categories'
import { Reports } from './pages/Reports'
import { useToast } from './hooks/useToast'
import { getCategories } from './api/index'
import type { Category, Transaction } from './types'

function ProtectedLayout() {
  const { user, loading } = useAuth()
  const { toasts, toast, dismiss } = useToast()

  const [modalOpen,    setModalOpen]    = useState(false)
  const [editTxn,      setEditTxn]      = useState<Transaction | null>(null)
  const [categories,   setCategories]   = useState<Category[]>([])
  const [refreshKey,   setRefreshKey]   = useState(0)

  const loadCategories = useCallback(async () => {
    try {
      const cats = await getCategories()
      setCategories(cats)
    } catch {
      // silent fail
    }
  }, [])

  useEffect(() => {
    if (user) loadCategories()
  }, [user, loadCategories, refreshKey])

  const handleSaved = useCallback(() => {
    setRefreshKey(k => k + 1)
    loadCategories()
  }, [loadCategories])

  const handleEdit = useCallback((t: Transaction) => {
    setEditTxn(t)
    setModalOpen(true)
  }, [])

  const openAdd = useCallback(() => {
    setEditTxn(null)
    setModalOpen(true)
  }, [])

  if (loading) {
    return (
      <div className="min-h-screen bg-surface-950 flex items-center justify-center">
        <div className="w-8 h-8 border-2 border-brand-500/30 border-t-brand-500 rounded-full animate-spin" />
      </div>
    )
  }

  if (!user) return <Navigate to="/login" replace />

  return (
    <div className="min-h-screen bg-surface-950 flex">
      <Sidebar onAddTransaction={openAdd} />

      {/* Main content */}
      <main
        className="flex-1 min-w-0 p-6 overflow-y-auto"
        style={{ marginLeft: 'var(--sidebar-w)' }}
      >
        <div className="max-w-6xl mx-auto">
          <Routes>
            <Route path="/" element={
              <Dashboard
                categories={categories}
                onAddTransaction={openAdd}
                toast={toast}
                refreshKey={refreshKey}
              />
            } />
            <Route path="/transactions" element={
              <Transactions
                categories={categories}
                onEdit={handleEdit}
                toast={toast}
                refreshKey={refreshKey}
              />
            } />
            <Route path="/categories" element={
              <CategoriesPage toast={toast} refreshKey={refreshKey} />
            } />
            <Route path="/reports" element={
              <Reports toast={toast} />
            } />
            <Route path="*" element={<Navigate to="/" replace />} />
          </Routes>
        </div>
      </main>

      {/* Global transaction modal */}
      <TransactionModal
        open={modalOpen}
        onClose={() => { setModalOpen(false); setEditTxn(null) }}
        categories={categories}
        editTxn={editTxn}
        onSaved={handleSaved}
        toast={toast}
      />

      <ToastContainer toasts={toasts} dismiss={dismiss} />
    </div>
  )
}

export default function App() {
  const { toasts, toast, dismiss } = useToast()

  return (
    <BrowserRouter>
      <AuthProvider>
        <Routes>
          <Route path="/login"    element={<><Login    toast={toast} /><ToastContainer toasts={toasts} dismiss={dismiss} /></>} />
          <Route path="/register" element={<><Register toast={toast} /><ToastContainer toasts={toasts} dismiss={dismiss} /></>} />
          <Route path="/*"        element={<ProtectedLayout />} />
        </Routes>
      </AuthProvider>
    </BrowserRouter>
  )
}
