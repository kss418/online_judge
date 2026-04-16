import { useAsyncResource } from '@/composables/useAsyncResource'

export function useProfileUserIdAsyncResource({
  initialData,
  load,
  getErrorMessage
}){
  const resource = useAsyncResource({
    initialData,
    load,
    getErrorMessage
  })

  function reset(){
    resource.reset({
      preserveHasLoadedOnce: true,
      clearLastArgs: true
    })
  }

  async function loadByProfileUserId(profileUserId){
    const normalizedProfileUserId = Number(profileUserId)

    if (!Number.isInteger(normalizedProfileUserId) || normalizedProfileUserId <= 0) {
      reset()

      return {
        status: 'reset',
        data: resource.data.value
      }
    }

    return resource.run(normalizedProfileUserId, {
      resetDataOnError: true
    })
  }

  return {
    resource,
    reset,
    loadByProfileUserId
  }
}
