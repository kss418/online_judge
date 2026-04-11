import {
  computed,
  onMounted,
  onUnmounted,
  ref,
  unref,
  watch
} from 'vue'

function resolveMaybeReactive(value){
  if (typeof value === 'function') {
    return value()
  }

  return unref(value)
}

export function usePollingController({
  task,
  enabled,
  intervalMs,
  pauseWhenHidden = true,
  runImmediately = false
}){
  const isInFlight = ref(false)
  const isDocumentVisible = ref(typeof document === 'undefined' ? true : !document.hidden)
  const isRunning = ref(false)
  const canRun = computed(() =>
    Boolean(resolveMaybeReactive(enabled)) &&
    (!pauseWhenHidden || isDocumentVisible.value)
  )

  let pollingTimer = null
  let isMounted = false
  let isStoppedManually = false
  let wasRunnable = false

  function resolveIntervalMs(){
    const nextIntervalMs = Number(resolveMaybeReactive(intervalMs))
    return Number.isFinite(nextIntervalMs) && nextIntervalMs >= 0
      ? nextIntervalMs
      : 0
  }

  function clearScheduledPoll(){
    if (pollingTimer) {
      clearTimeout(pollingTimer)
      pollingTimer = null
    }
  }

  function updateRunningState(){
    isRunning.value = isMounted && !isStoppedManually && (
      isInFlight.value ||
      Boolean(pollingTimer) ||
      canRun.value
    )
  }

  function updateDocumentVisibility(){
    if (typeof document === 'undefined') {
      isDocumentVisible.value = true
      return
    }

    isDocumentVisible.value = !document.hidden
  }

  async function executeTask(){
    if (
      !isMounted ||
      isStoppedManually ||
      isInFlight.value ||
      !canRun.value
    ) {
      updateRunningState()
      return
    }

    isInFlight.value = true
    updateRunningState()

    try {
      await task()
    } finally {
      isInFlight.value = false
      sync()
    }
  }

  function scheduleNextPoll(){
    if (
      !isMounted ||
      isStoppedManually ||
      isInFlight.value ||
      pollingTimer ||
      !canRun.value ||
      typeof window === 'undefined'
    ) {
      updateRunningState()
      return
    }

    pollingTimer = window.setTimeout(() => {
      pollingTimer = null
      void executeTask()
    }, resolveIntervalMs())

    updateRunningState()
  }

  function stop(){
    isStoppedManually = true
    wasRunnable = false
    clearScheduledPoll()
    updateRunningState()
  }

  function start(){
    isStoppedManually = false
    sync()
  }

  function sync(){
    const runnable = isMounted && !isStoppedManually && canRun.value
    const becameRunnable = runnable && !wasRunnable

    clearScheduledPoll()

    if (!runnable) {
      wasRunnable = false
      updateRunningState()
      return
    }

    wasRunnable = true

    if (isInFlight.value) {
      updateRunningState()
      return
    }

    if (becameRunnable && runImmediately) {
      void executeTask()
      return
    }

    scheduleNextPoll()
  }

  function runNow(){
    if (!isMounted) {
      return
    }

    isStoppedManually = false
    clearScheduledPoll()
    void executeTask()
  }

  function handleDocumentVisibilityChange(){
    updateDocumentVisibility()
    sync()
  }

  watch(
    [canRun, () => resolveIntervalMs()],
    () => {
      sync()
    }
  )

  onMounted(() => {
    isMounted = true
    updateDocumentVisibility()

    if (typeof document !== 'undefined') {
      document.addEventListener('visibilitychange', handleDocumentVisibilityChange)
    }

    sync()
  })

  onUnmounted(() => {
    isMounted = false
    wasRunnable = false
    clearScheduledPoll()

    if (typeof document !== 'undefined') {
      document.removeEventListener('visibilitychange', handleDocumentVisibilityChange)
    }

    updateRunningState()
  })

  return {
    isRunning,
    isInFlight,
    isDocumentVisible,
    start,
    stop,
    sync,
    runNow
  }
}
