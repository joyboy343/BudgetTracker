import {
  Chart as ChartJS,
  CategoryScale, LinearScale, BarElement, PointElement,
  LineElement, Title, Tooltip, Legend, Filler,
  type ChartOptions
} from 'chart.js'
import { Bar, Line } from 'react-chartjs-2'
import type { MonthlyReport, MonthlyTrend } from '../types'
import type { CategorySummary } from '../types'

ChartJS.register(
  CategoryScale, LinearScale, BarElement, PointElement,
  LineElement, Title, Tooltip, Legend, Filler
)

const chartFont = { family: "'DM Sans', sans-serif", size: 11 }

// ── Monthly Income vs Expense Bar Chart ───────────────────────────────────────

interface MonthlyChartProps { report: MonthlyReport }

export function MonthlyChart({ report }: MonthlyChartProps) {
  const options: ChartOptions<'bar'> = {
    responsive: true,
    maintainAspectRatio: false,
    plugins: {
      legend: {
        position: 'top',
        labels: { color: '#94a3b8', font: chartFont, boxWidth: 10, padding: 16 },
      },
      tooltip: {
        backgroundColor: '#1e293b',
        borderColor: '#334155',
        borderWidth: 1,
        titleColor: '#f1f5f9',
        bodyColor: '#94a3b8',
        titleFont: { ...chartFont, weight: 'bold' },
        callbacks: {
          label: (ctx) => ` $${(ctx.raw as number)?.toLocaleString('en-US', { minimumFractionDigits: 2 })}`,
        },
      },
    },
    scales: {
      x: {
        grid:  { color: 'rgba(51,65,85,0.4)' },
        ticks: { color: '#64748b', font: chartFont },
      },
      y: {
        grid:  { color: 'rgba(51,65,85,0.4)' },
        ticks: {
          color: '#64748b',
          font:   chartFont,
          callback: (v) => `$${Number(v).toLocaleString()}`,
        },
      },
    },
  }

  const data = {
    labels: ['Income', 'Expenses'],
    datasets: [{
      label: report.year_month,
      data:  [report.total_income, report.total_expense],
      backgroundColor: ['rgba(16,185,129,0.7)', 'rgba(239,68,68,0.7)'],
      borderColor:     ['rgb(16,185,129)',       'rgb(239,68,68)'],
      borderWidth: 1,
      borderRadius: 6,
    }],
  }

  return (
    <div className="h-48">
      <Bar data={data} options={options} />
    </div>
  )
}

// ── Category Spending Bar Chart ───────────────────────────────────────────────

interface CategoryChartProps { categories: CategorySummary[] }

export function CategorySpendChart({ categories }: CategoryChartProps) {
  const top = categories.filter(c => c.spent > 0).slice(0, 8)

  const options: ChartOptions<'bar'> = {
    responsive: true,
    maintainAspectRatio: false,
    indexAxis: 'y',
    plugins: {
      legend: { display: false },
      tooltip: {
        backgroundColor: '#1e293b',
        borderColor: '#334155',
        borderWidth: 1,
        titleColor: '#f1f5f9',
        bodyColor: '#94a3b8',
        callbacks: {
          label: (ctx) => ` $${(ctx.raw as number)?.toLocaleString('en-US', { minimumFractionDigits: 2 })}`,
        },
      },
    },
    scales: {
      x: {
        grid:  { color: 'rgba(51,65,85,0.4)' },
        ticks: { color: '#64748b', font: chartFont, callback: (v) => `$${v}` },
      },
      y: {
        grid:  { display: false },
        ticks: { color: '#94a3b8', font: { ...chartFont, size: 12 } },
      },
    },
  }

  const data = {
    labels: top.map(c => c.category),
    datasets: [{
      data:            top.map(c => c.spent),
      backgroundColor: top.map(c => c.color + 'bb'),
      borderColor:     top.map(c => c.color),
      borderWidth: 1,
      borderRadius: 4,
    }],
  }

  return (
    <div style={{ height: `${Math.max(top.length * 36, 120)}px` }}>
      <Bar data={data} options={options} />
    </div>
  )
}

// ── 6-Month Trend Line Chart ──────────────────────────────────────────────────

interface TrendChartProps { trend: MonthlyTrend[] }

export function TrendChart({ trend }: TrendChartProps) {
  const options: ChartOptions<'line'> = {
    responsive: true,
    maintainAspectRatio: false,
    plugins: {
      legend: {
        position: 'top',
        labels: { color: '#94a3b8', font: chartFont, boxWidth: 10, padding: 16 },
      },
      tooltip: {
        backgroundColor: '#1e293b',
        borderColor: '#334155',
        borderWidth: 1,
        titleColor: '#f1f5f9',
        bodyColor: '#94a3b8',
        callbacks: {
          label: (ctx) => ` $${(ctx.raw as number)?.toLocaleString('en-US', { minimumFractionDigits: 2 })}`,
        },
      },
    },
    scales: {
      x: {
        grid:  { color: 'rgba(51,65,85,0.4)' },
        ticks: { color: '#64748b', font: chartFont },
      },
      y: {
        grid:  { color: 'rgba(51,65,85,0.4)' },
        ticks: {
          color: '#64748b',
          font:   chartFont,
          callback: (v) => `$${Number(v).toLocaleString()}`,
        },
      },
    },
    interaction: { mode: 'index', intersect: false },
    elements: { line: { tension: 0.4 } },
  }

  const data = {
    labels: trend.map(t => t.year_month),
    datasets: [
      {
        label: 'Income',
        data:  trend.map(t => t.income),
        borderColor:     'rgb(16,185,129)',
        backgroundColor: 'rgba(16,185,129,0.1)',
        fill: true,
        pointBackgroundColor: 'rgb(16,185,129)',
        pointRadius: 4,
      },
      {
        label: 'Expenses',
        data:  trend.map(t => t.expense),
        borderColor:     'rgb(239,68,68)',
        backgroundColor: 'rgba(239,68,68,0.1)',
        fill: true,
        pointBackgroundColor: 'rgb(239,68,68)',
        pointRadius: 4,
      },
    ],
  }

  return (
    <div className="h-56">
      <Line data={data} options={options} />
    </div>
  )
}
