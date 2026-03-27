import { reactive, readonly } from 'vue'

const noticeState = reactive({
  items: []
})

let nextNoticeId = 1
const hideTimerIds = new Map()

function clearHideTimer(noticeId){
  const hideTimerId = hideTimerIds.get(noticeId)
  if (!hideTimerId) {
    return
  }

  window.clearTimeout(hideTimerId)
  hideTimerIds.delete(noticeId)
}

function clearNotice(noticeId){
  if (typeof noticeId === 'number') {
    clearHideTimer(noticeId)
    noticeState.items = noticeState.items.filter((item) => item.id !== noticeId)
    return
  }

  noticeState.items.forEach((item) => {
    clearHideTimer(item.id)
  })
  noticeState.items = []
}

function showNotice(message, options = {}){
  const normalizedMessage = typeof message === 'string' ? message.trim() : ''
  if (!normalizedMessage) {
    return
  }

  const noticeId = nextNoticeId++
  const notice = {
    id: noticeId,
    message: normalizedMessage,
    tone: options.tone || 'neutral'
  }

  noticeState.items.unshift(notice)

  const duration = Number(options.duration ?? 4000)
  if (duration > 0) {
    const hideTimerId = window.setTimeout(() => {
      clearNotice(noticeId)
    }, duration)
    hideTimerIds.set(noticeId, hideTimerId)
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

export function useNotice(){
  return {
    noticeState: readonly(noticeState),
    showNotice,
    showSuccessNotice,
    showErrorNotice,
    clearNotice
  }
}
