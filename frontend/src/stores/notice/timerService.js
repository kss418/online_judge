const hideTimerIds = new Map()

export function clearHideTimer(noticeId){
  const hideTimerId = hideTimerIds.get(noticeId)
  if (!hideTimerId) {
    return
  }

  window.clearTimeout(hideTimerId)
  hideTimerIds.delete(noticeId)
}

export function registerHideTimer(noticeId, hideTimerId){
  hideTimerIds.set(noticeId, hideTimerId)
}

export function clearAllHideTimers(){
  hideTimerIds.forEach((hideTimerId) => {
    window.clearTimeout(hideTimerId)
  })
  hideTimerIds.clear()
}
