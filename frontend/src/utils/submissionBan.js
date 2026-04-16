import { formatRemainingDuration, formatTimestamp } from '@/utils/dateTime'

export function createSubmissionBanState(){
  return {
    submission_banned_until: null,
    submission_banned_until_timestamp: null,
    submission_banned_until_label: ''
  }
}

export function isSubmissionBanActive(nowTimestamp, submissionBanTimestamp){
  return (
    typeof nowTimestamp === 'number' &&
    !Number.isNaN(nowTimestamp) &&
    typeof submissionBanTimestamp === 'number' &&
    !Number.isNaN(submissionBanTimestamp) &&
    submissionBanTimestamp > nowTimestamp
  )
}

export function getSubmissionBanPhase({
  nowTimestamp,
  submissionBanTimestamp,
  isLoading = false,
  hasError = false
}){
  if (isLoading) {
    return 'loading'
  }

  if (hasError) {
    return 'error'
  }

  if (isSubmissionBanActive(nowTimestamp, submissionBanTimestamp)) {
    return 'active'
  }

  return 'none'
}

export function formatSubmissionBanRemaining(
  nowTimestamp,
  submissionBanTimestamp,
  fallback = ''
){
  if (!isSubmissionBanActive(nowTimestamp, submissionBanTimestamp)) {
    return fallback
  }

  return formatRemainingDuration(nowTimestamp, submissionBanTimestamp)
}

export function formatSubmissionBanUntil(submissionBannedUntil, fallback = '-'){
  if (typeof submissionBannedUntil !== 'string' || !submissionBannedUntil) {
    return fallback
  }

  return formatTimestamp(submissionBannedUntil)
}
