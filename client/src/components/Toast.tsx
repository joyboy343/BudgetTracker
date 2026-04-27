import { CheckCircle, XCircle, AlertTriangle, Info, X } from 'lucide-react'
import type { Toast } from '../types'

interface Props {
  toasts:  Toast[]
  dismiss: (id: string) => void
}

const icons = {
  success: <CheckCircle  size={16} className="text-brand-400" />,
  error:   <XCircle      size={16} className="text-red-400"   />,
  warning: <AlertTriangle size={16} className="text-amber-400" />,
  info:    <Info         size={16} className="text-blue-400"  />,
}

const colors = {
  success: 'border-brand-500/30 bg-brand-500/10',
  error:   'border-red-500/30   bg-red-500/10',
  warning: 'border-amber-500/30 bg-amber-500/10',
  info:    'border-blue-500/30  bg-blue-500/10',
}

export function ToastContainer({ toasts, dismiss }: Props) {
  return (
    <div className="fixed bottom-6 right-6 z-50 flex flex-col gap-2 pointer-events-none">
      {toasts.map((t) => (
        <div
          key={t.id}
          className={`
            flex items-center gap-3 pl-4 pr-3 py-3 rounded-xl border text-sm
            shadow-2xl backdrop-blur pointer-events-auto
            animate-slide-up min-w-[280px] max-w-[380px]
            ${colors[t.type]}
          `}
        >
          {icons[t.type]}
          <span className="flex-1 text-slate-200">{t.message}</span>
          <button
            onClick={() => dismiss(t.id)}
            className="text-slate-500 hover:text-slate-300 transition-colors"
          >
            <X size={14} />
          </button>
        </div>
      ))}
    </div>
  )
}
