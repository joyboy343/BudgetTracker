import { createContext, useContext, useState, useEffect, useCallback, type ReactNode } from 'react'
import { getStoredUser, clearAuth } from '../api/client'
import { login as apiLogin, register as apiRegister, logout as apiLogout } from '../api/auth'
import type { User } from '../types'

interface AuthContextValue {
  user:     User | null
  loading:  boolean
  login:    (email: string, password: string) => Promise<void>
  register: (email: string, password: string) => Promise<void>
  logout:   () => Promise<void>
}

const AuthContext = createContext<AuthContextValue | null>(null)

export function AuthProvider({ children }: { children: ReactNode }) {
  const [user,    setUser]    = useState<User | null>(null)
  const [loading, setLoading] = useState(true)

  useEffect(() => {
    const stored = getStoredUser()
    if (stored) setUser(stored)
    setLoading(false)
  }, [])

  const login = useCallback(async (email: string, password: string) => {
    const tokens = await apiLogin(email, password)
    setUser({ id: tokens.user_id, email: tokens.email, created_at: '' })
  }, [])

  const register = useCallback(async (email: string, password: string) => {
    const tokens = await apiRegister(email, password)
    setUser({ id: tokens.user_id, email: tokens.email, created_at: '' })
  }, [])

  const logout = useCallback(async () => {
    await apiLogout()
    clearAuth()
    setUser(null)
  }, [])

  return (
    <AuthContext.Provider value={{ user, loading, login, register, logout }}>
      {children}
    </AuthContext.Provider>
  )
}

export function useAuth() {
  const ctx = useContext(AuthContext)
  if (!ctx) throw new Error('useAuth must be used within AuthProvider')
  return ctx
}
