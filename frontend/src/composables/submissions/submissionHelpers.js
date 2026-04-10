import { getSubmissionStatusLabel } from '@/utils/submissionStatus'

export const listLimit = 50
export const submissionPollingIntervalMs = 2000
export const pollingSubmissionStatuses = new Set(['queued', 'judging'])
export const submissionStatusOptions = [
  { value: '', label: '전체' },
  { value: 'accepted', label: '정답' },
  { value: 'wrong_answer', label: '오답' },
  { value: 'time_limit_exceeded', label: '시간 초과' },
  { value: 'memory_limit_exceeded', label: '메모리 초과' },
  { value: 'runtime_error', label: '런타임 에러' },
  { value: 'output_exceeded', label: '출력 초과' },
  { value: 'compile_error', label: '컴파일 에러' }
]

export const finishedSubmissionStatuses = new Set([
  'accepted',
  'wrong_answer',
  'time_limit_exceeded',
  'memory_limit_exceeded',
  'runtime_error',
  'output_exceeded',
  'compile_error'
])

export function normalizeProblemIdFilterInputValue(value){
  if (value === null || typeof value === 'undefined') {
    return ''
  }

  return String(value).trim()
}

export function normalizeUserIdFilterInputValue(value){
  if (value === null || typeof value === 'undefined') {
    return ''
  }

  return String(value).trim()
}

export function normalizeSubmissionMetric(value){
  if (value === null || typeof value === 'undefined' || value === '') {
    return null
  }

  const parsedValue = Number(value)
  return Number.isFinite(parsedValue) ? parsedValue : null
}

export function normalizeSubmittedAt(value){
  if (typeof value !== 'string' || !value.trim()) {
    return {
      timestamp: null,
      label: ''
    }
  }

  const trimmedValue = value.trim()
  const matchedTimestamp = trimmedValue.match(
    /^(\d{4}-\d{2}-\d{2})[ T](\d{2}:\d{2}:\d{2})(?:\.(\d{1,6}))?([+-]\d{2})(?::?(\d{2}))?$/
  )

  if (matchedTimestamp) {
    const [, datePart, timePart, fractionPart = '', offsetHour, offsetMinute = '00'] =
      matchedTimestamp
    const normalizedFraction = fractionPart
      ? `.${fractionPart.slice(0, 3).padEnd(3, '0')}`
      : ''
    const parsedTimestamp = Date.parse(
      `${datePart}T${timePart}${normalizedFraction}${offsetHour}:${offsetMinute}`
    )

    return {
      timestamp: Number.isNaN(parsedTimestamp) ? null : parsedTimestamp,
      label: `${datePart} ${timePart}`
    }
  }

  const parsedTimestamp = Date.parse(trimmedValue.replace(' ', 'T'))
  return {
    timestamp: Number.isNaN(parsedTimestamp) ? null : parsedTimestamp,
    label: trimmedValue
  }
}

export function formatRelativeSubmittedAt(nowTimestamp, timestamp){
  if (typeof timestamp !== 'number' || Number.isNaN(timestamp)) {
    return '-'
  }

  const elapsedSeconds = Math.max(1, Math.floor((nowTimestamp - timestamp) / 1000))

  if (elapsedSeconds < 60) {
    return `${elapsedSeconds}초 전`
  }

  const elapsedMinutes = Math.floor(elapsedSeconds / 60)
  if (elapsedMinutes < 60) {
    return `${elapsedMinutes}분 전`
  }

  const elapsedHours = Math.floor(elapsedMinutes / 60)
  if (elapsedHours < 24) {
    return `${elapsedHours}시간 전`
  }

  const elapsedDays = Math.floor(elapsedHours / 24)
  if (elapsedDays < 30) {
    return `${elapsedDays}일 전`
  }

  const elapsedMonths = Math.floor(elapsedDays / 30)
  if (elapsedMonths < 12) {
    return `${elapsedMonths}달 전`
  }

  const elapsedYears = Math.floor(elapsedDays / 365)
  return `${elapsedYears}년 전`
}

export function formatElapsedMs(countFormatter, value){
  if (typeof value !== 'number') {
    return '-'
  }

  return `${countFormatter.format(value)} ms`
}

export function formatMemory(countFormatter, value){
  if (typeof value !== 'number') {
    return '-'
  }

  return `${countFormatter.format(value)} KB`
}

export function normalizeSubmissionHistoryEntry(historyEntry, index){
  const normalizedCreatedAt = normalizeSubmittedAt(historyEntry?.created_at)
  const numericHistoryId = Number(historyEntry?.history_id)
  const fromStatus = typeof historyEntry?.from_status === 'string' && historyEntry.from_status
    ? historyEntry.from_status
    : null
  const toStatus = typeof historyEntry?.to_status === 'string' && historyEntry.to_status
    ? historyEntry.to_status
    : 'queued'
  const createdAtLabel = normalizedCreatedAt.label ||
    (typeof historyEntry?.created_at === 'string' && historyEntry.created_at.trim()
      ? historyEntry.created_at.trim()
      : '-')

  return {
    history_id: Number.isInteger(numericHistoryId) && numericHistoryId > 0
      ? numericHistoryId
      : null,
    history_key: Number.isInteger(numericHistoryId) && numericHistoryId > 0
      ? `history-${numericHistoryId}`
      : `history-fallback-${index}-${createdAtLabel}`,
    from_status: fromStatus,
    to_status: toStatus,
    reason: typeof historyEntry?.reason === 'string' ? historyEntry.reason.trim() : '',
    created_at_label: createdAtLabel
  }
}

export function formatHistoryTransition(historyEntry){
  if (!historyEntry?.from_status) {
    return getSubmissionStatusLabel('queued')
  }

  return `${getSubmissionStatusLabel(historyEntry.from_status)} -> ${getSubmissionStatusLabel(historyEntry.to_status)}`
}

export function fallbackCopyText(text){
  if (typeof document === 'undefined') {
    throw new Error('clipboard unavailable')
  }

  const helperTextArea = document.createElement('textarea')
  helperTextArea.value = text
  helperTextArea.setAttribute('readonly', '')
  helperTextArea.style.position = 'fixed'
  helperTextArea.style.opacity = '0'
  helperTextArea.style.pointerEvents = 'none'

  document.body.appendChild(helperTextArea)
  helperTextArea.select()
  helperTextArea.setSelectionRange(0, helperTextArea.value.length)

  const copySucceeded = document.execCommand('copy')
  document.body.removeChild(helperTextArea)

  if (!copySucceeded) {
    throw new Error('clipboard unavailable')
  }
}
