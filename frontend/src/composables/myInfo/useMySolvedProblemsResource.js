import { getUserSolvedProblems } from '@/api/user'
import { useAsyncResource } from '@/composables/useAsyncResource'
import { formatApiError } from '@/utils/apiError'

export function useMySolvedProblemsResource({
  authState
}){
  const solvedProblemsResource = useAsyncResource({
    initialData: [],
    async load(profileUserId){
      const payload = await getUserSolvedProblems(
        profileUserId,
        authState.token || undefined
      )

      return Array.isArray(payload.solved_problems) ? payload.solved_problems : []
    },
    getErrorMessage(error){
      return formatApiError(error, {
        fallback: '푼 문제 목록을 불러오지 못했습니다.'
      })
    }
  })

  function resetSolvedProblems(){
    solvedProblemsResource.reset({
      preserveHasLoadedOnce: true,
      clearLastArgs: true
    })
  }

  async function loadSolvedProblems(profileUserId){
    const normalizedProfileUserId = Number(profileUserId)

    if (!Number.isInteger(normalizedProfileUserId) || normalizedProfileUserId <= 0) {
      resetSolvedProblems()

      return {
        status: 'reset',
        data: solvedProblemsResource.data.value
      }
    }

    return solvedProblemsResource.run(normalizedProfileUserId, {
      resetDataOnError: true
    })
  }

  return {
    solvedProblems: solvedProblemsResource.data,
    isSolvedProblemsLoading: solvedProblemsResource.isLoading,
    solvedProblemsErrorMessage: solvedProblemsResource.errorMessage,
    resetSolvedProblems,
    loadSolvedProblems
  }
}
