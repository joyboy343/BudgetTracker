import { NavLink, useNavigate } from 'react-router-dom'
import {
  LayoutDashboard, ArrowUpDown, Tag, BarChart3,
  LogOut, TrendingUp, Plus
} from 'lucide-react'
import { useAuth } from '../context/AuthContext'
import { clsx } from 'clsx'

interface Props {
  onAddTransaction: () => void
}

const navItems = [
  { to: '/',             icon: LayoutDashboard, label: 'Dashboard'    },
  { to: '/transactions', icon: ArrowUpDown,     label: 'Transactions' },
  { to: '/categories',   icon: Tag,             label: 'Budgets'      },
  { to: '/reports',      icon: BarChart3,       label: 'Reports'      },
]

export function Sidebar({ onAddTransaction }: Props) {
  const { user, logout } = useAuth()
  const navigate = useNavigate()

  const handleLogout = async () => {
    await logout()
    navigate('/login')
  }

  return (
    <aside
      className="fixed inset-y-0 left-0 z-30 flex flex-col"
      style={{ width: 'var(--sidebar-w)' }}
    >
      {/* Logo */}
      <div className="flex items-center gap-2.5 px-6 py-6 border-b border-slate-800">
        <div className="w-8 h-8 rounded-lg bg-brand-500 flex items-center justify-center">
          <TrendingUp size={16} className="text-white" />
        </div>
        <span className="font-display text-lg font-700 text-white tracking-tight">Ledger</span>
      </div>

      {/* Add Transaction CTA */}
      <div className="px-4 pt-4">
        <button
          onClick={onAddTransaction}
          className="btn-primary w-full justify-center py-3 font-semibold"
        >
          <Plus size={16} />
          Add Transaction
        </button>
      </div>

      {/* Nav links */}
      <nav className="flex-1 px-3 py-4 space-y-1">
        {navItems.map(({ to, icon: Icon, label }) => (
          <NavLink
            key={to}
            to={to}
            end={to === '/'}
            className={({ isActive }) =>
              clsx(
                'flex items-center gap-3 px-3 py-2.5 rounded-xl text-sm font-medium transition-all duration-150',
                isActive
                  ? 'bg-brand-500/15 text-brand-400 border border-brand-500/20'
                  : 'text-slate-400 hover:text-slate-100 hover:bg-slate-800'
              )
            }
          >
            <Icon size={17} />
            {label}
          </NavLink>
        ))}
      </nav>

      {/* User / Logout */}
      <div className="px-4 pb-4 border-t border-slate-800 pt-4">
        <div className="flex items-center gap-3 px-3 py-2 rounded-xl">
          <div className="w-8 h-8 rounded-full bg-brand-500/20 border border-brand-500/30 flex items-center justify-center text-brand-400 text-xs font-semibold">
            {user?.email?.[0]?.toUpperCase() ?? 'U'}
          </div>
          <div className="flex-1 min-w-0">
            <p className="text-xs font-medium text-slate-300 truncate">{user?.email}</p>
          </div>
          <button
            onClick={handleLogout}
            title="Logout"
            className="text-slate-500 hover:text-red-400 transition-colors"
          >
            <LogOut size={15} />
          </button>
        </div>
      </div>
    </aside>
  )
}
