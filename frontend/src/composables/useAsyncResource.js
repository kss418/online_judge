import { ref } from 'vue'
import { formatApiError } from '@/utils/apiError'

function resolveInitialData(initialData){
  return typeof initialData === 'function'
    ? initialData()
    : initialData
}

export function useAsyncResource({ initialData, load, getErrorMessage }){
  const data = ref(resolveInitialData(initialData))
  const errorMessage = ref('')
  const isLoading = ref(false)
  const hasLoadedOnce = ref(false)

  let latestRequestId = 0
  let lastArgs

  function invalidate(){
    latestRequestId += 1
    isLoading.value = false
  }

  function reset(options = {}){
    invalidate()
    data.value = resolveInitialData(initialData)
    errorMessage.value = ''

    if (!options.preserveHasLoadedOnce) {
      hasLoadedOnce.value = false
    }

    if (options.clearLastArgs) {
      lastArgs = undefined
    }
  }

  function mutate(nextValueOrUpdater){
    data.value = typeof nextValueOrUpdater === 'function'
      ? nextValueOrUpdater(data.value)
      : nextValueOrUpdater
  }

  async function run(args, options = {}){
    const {
      background = false,
      clearErrorOnRun = true,
      resetDataOnRun = false,
      resetDataOnError = false,
      setErrorOnError = true
    } = options
    const requestId = ++latestRequestId

    lastArgs = args

    if (!background) {
      isLoading.value = true
    }

    if (clearErrorOnRun) {
      errorMessage.value = ''
    }

    if (!background && resetDataOnRun) {
      data.value = resolveInitialData(initialData)
    }

    try {
      const nextData = await load(args)

      if (requestId !== latestRequestId) {
        return {
          status: 'stale',
          data: data.value
        }
      }

      data.value = nextData
      hasLoadedOnce.value = true

      return {
        status: 'success',
        data: nextData
      }
    } catch (error) {
      if (requestId !== latestRequestId) {
        return {
          status: 'stale',
          error
        }
      }

      if (setErrorOnError) {
        errorMessage.value = typeof getErrorMessage === 'function'
          ? getErrorMessage(error)
          : formatApiError(error)
      }

      if (resetDataOnError) {
        data.value = resolveInitialData(initialData)
      }

      hasLoadedOnce.value = true

      return {
        status: 'error',
        error
      }
    } finally {
      if (!background && requestId === latestRequestId) {
        isLoading.value = false
      }
    }
  }

  async function refresh(options = {}){
    return run(lastArgs, options)
  }

  return {
    data,
    errorMessage,
    isLoading,
    hasLoadedOnce,
    run,
    refresh,
    reset,
    invalidate,
    mutate
  }
}
