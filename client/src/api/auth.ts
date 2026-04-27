// ── auth.ts ───────────────────────────────────────────────────────────────────
import { api, setAuth, clearAuth } from './client'
import type { AuthTokens, User } from '../types'

export async function register(email: string, password: string): Promise<AuthTokens> {
  const { data } = await api.post<AuthTokens>('/auth/register', { email, password })
  setAuth(data.access_token, data.refresh_token, { id: data.user_id, email: data.email })
  return data
}

export async function login(email: string, password: string): Promise<AuthTokens> {
  const { data } = await api.post<AuthTokens>('/auth/login', { email, password })
  setAuth(data.access_token, data.refresh_token, { id: data.user_id, email: data.email })
  return data
}

export async function logout(): Promise<void> {
  try { await api.post('/auth/logout') } catch { /* ignore */ }
  clearAuth()
}

export async function getMe(): Promise<User> {
  const { data } = await api.get<User>('/auth/me')
  return data
}
