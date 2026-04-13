import { getUserWrongProblems } from '@/api/user'
import { useAsyncResource } from '@/composables/useAsyncResource'
import { formatApiError } from '@/utils/apiError'

export function useMyWrongProblemsResource({
  authState
}){
  const wrongProblemsResource = useAsyncResource({
    initialData: [],
    async load(profileUserId){
      const payload = await getUserWrongProblems(
        profileUserId,
        authState.token || undefined
      )

      return Array.isArray(payload.wrong_problems) ? payload.wrong_problems : []
    },
    getErrorMessage(error){
      return formatApiError(error, {
        fallback: '틀린 문제 목록을 불러오지 못했습니다.'
      })
    }
  })

  function resetWrongProblems(){
    wrongProblemsResource.reset({
      preserveHasLoadedOnce: true,
      clearLastArgs: true
    })
  }

  async function loadWrongProblems(profileUserId){
    const normalizedProfileUserId = Number(profileUserId)

    if (!Number.isInteger(normalizedProfileUserId) || normalizedProfileUserId <= 0) {
      resetWrongProblems()

      return {
        status: 'reset',
        data: wrongProblemsResource.data.value
      }
    }

    return wrongProblemsResource.run(normalizedProfileUserId, {
      resetDataOnError: true
    })
  }

  return {
    wrongProblems: wrongProblemsResource.data,
    isWrongProblemsLoading: wrongProblemsResource.isLoading,
    wrongProblemsErrorMessage: wrongProblemsResource.errorMessage,
    resetWrongProblems,
    loadWrongProblems
  }
}
