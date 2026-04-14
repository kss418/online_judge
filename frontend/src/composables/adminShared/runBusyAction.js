export async function runBusyAction({
  busySection,
  section,
  clearFeedback,
  run,
  onError,
  onFinally
}){
  busySection.value = section
  clearFeedback?.()

  try {
    return await run()
  } catch (error) {
    onError?.(error)
    return null
  } finally {
    busySection.value = ''
    onFinally?.()
  }
}
