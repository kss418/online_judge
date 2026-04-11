import {
  finishedSubmissionStatuses,
  getSubmissionStatusLabel,
  submissionPollingStatuses
} from '@/utils/submissionStatus'

export const listLimit = 50
export const submissionPollingIntervalMs = 2000
export { finishedSubmissionStatuses }
export const pollingSubmissionStatuses = submissionPollingStatuses

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
