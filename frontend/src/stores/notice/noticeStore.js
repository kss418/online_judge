import {
  allocateNoticeId,
  clearNoticeItems,
  pushNotice,
  removeNotice,
  state
} from '@/stores/notice/state'
import {
  clearAllHideTimers,
  clearHideTimer,
  registerHideTimer
} from '@/stores/notice/timerService'

function clearNotice(noticeId){
  if (typeof noticeId === 'number') {
    clearHideTimer(noticeId)
    removeNotice(noticeId)
    return
  }

  clearAllHideTimers()
  clearNoticeItems()
}

function showNotice(message, options = {}){
  const normalizedMessage = typeof message === 'string' ? message.trim() : ''
  if (!normalizedMessage) {
    return
  }

  const noticeId = allocateNoticeId()
  const notice = {
    id: noticeId,
    message: normalizedMessage,
    tone: options.tone || 'neutral'
  }

  pushNotice(notice)

  const duration = Number(options.duration ?? 4000)
  if (duration > 0) {
    const hideTimerId = window.setTimeout(() => {
      clearNotice(noticeId)
    }, duration)
    registerHideTimer(noticeId, hideTimerId)
  }

  return noticeId
}

function showSuccessNotice(message, options = {}){
  return showNotice(message, {
    ...options,
    tone: 'success'
  })
}

function showErrorNotice(message, options = {}){
  return showNotice(message, {
    ...options,
    tone: 'danger'
  })
}

export const noticeStore = {
  state,
  showNotice,
  showSuccessNotice,
  showErrorNotice,
  clearNotice
}
